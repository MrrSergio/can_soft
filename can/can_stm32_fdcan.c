#include "can_stm32_fdcan.h"
#include "can_manager.h"
#include <stdio.h>
#include <stddef.h>

#define GET_CTX(h) ((FDCAN_Context *)((char *)(h) - offsetof(FDCAN_Context, hfdcan)))

/* Simple FDCAN driver based on STM32 HAL */

typedef struct {
    CAN_DriverContext_t base;
    FDCAN_HandleTypeDef hfdcan;
    ICANDriver         *driver;
} FDCAN_Context;

static CAN_Result_t fd_set_filter(ICANDriver *drv, uint32_t id, uint32_t mask);
static CAN_Result_t fd_set_mode(ICANDriver *drv, CAN_Mode_t mode);
static void         fd_config_bitrate(FDCAN_Context *ctx, uint32_t bitrate);

static CAN_Result_t fd_init(ICANDriver *drv, const CAN_Config_t *cfg)
{
    FDCAN_Context *ctx = (FDCAN_Context *)drv->ctx;
    if (!ctx)
        return CAN_ERROR;

    fd_config_bitrate(ctx, cfg ? cfg->bitrate : 500000);
    if (HAL_FDCAN_Start(&ctx->hfdcan) != HAL_OK)
        return CAN_ERROR;

    if (cfg) {
        fd_set_filter(drv, cfg->filter_id, cfg->filter_mask);
        fd_set_mode(drv, cfg->mode);
    }
    return CAN_OK;
}

static CAN_Result_t fd_send(ICANDriver *drv, const CAN_Message_t *msg, uint32_t timeout)
{
    (void)timeout;
    if (!msg)
        return CAN_ERROR;
    FDCAN_TxHeaderTypeDef hdr = {
        .Identifier = msg->id,
        .IdType = msg->extended ? FDCAN_EXTENDED_ID : FDCAN_STANDARD_ID,
        .TxFrameType = FDCAN_DATA_FRAME,
        .DataLength = msg->dlc << 16,
        .ErrorStateIndicator = FDCAN_ESI_ACTIVE,
        .BitRateSwitch = FDCAN_BRS_OFF,
        .FDFormat = FDCAN_CLASSIC_CAN,
        .TxEventFifoControl = FDCAN_NO_TX_EVENTS,
        .MessageMarker = 0
    };
    FDCAN_Context *ctx = (FDCAN_Context *)drv->ctx;
    if (HAL_FDCAN_AddMessageToTxFifoQ(&ctx->hfdcan, &hdr, msg->data) != HAL_OK)
        return CAN_ERROR;
    CAN_Manager_TriggerEvent(ctx->base.inst_id, CAN_EVENT_TX_COMPLETE, (void *)msg);
    return CAN_OK;
}

static CAN_Result_t fd_receive(ICANDriver *drv, CAN_Message_t *msg)
{
    FDCAN_RxHeaderTypeDef hdr;
    FDCAN_Context *ctx = (FDCAN_Context *)drv->ctx;
    if (!msg)
        return CAN_ERROR;
    if (HAL_FDCAN_GetRxMessage(&ctx->hfdcan, FDCAN_RX_FIFO0, &hdr, msg->data) == HAL_OK) {
        msg->id = hdr.Identifier;
        msg->extended = (hdr.IdType == FDCAN_EXTENDED_ID) ? 1 : 0;
        msg->dlc = hdr.DataLength >> 16;
        CAN_Manager_TriggerEvent(ctx->base.inst_id, CAN_EVENT_RX, msg);
        return CAN_OK;
    }
    return CAN_ERROR;
}

static CAN_Result_t fd_set_filter(ICANDriver *drv, uint32_t id, uint32_t mask)
{
    FDCAN_Context *ctx = (FDCAN_Context *)drv->ctx;
    FDCAN_FilterTypeDef f = {
        .IdType = FDCAN_EXTENDED_ID,
        .FilterIndex = 0,
        .FilterType = FDCAN_FILTER_MASK,
        .FilterConfig = FDCAN_FILTER_TO_RXFIFO0,
        .FilterID1 = id,
        .FilterID2 = mask
    };
    HAL_FDCAN_ConfigFilter(&ctx->hfdcan, &f);
    return CAN_OK;
}

static CAN_Result_t fd_set_mode(ICANDriver *drv, CAN_Mode_t mode)
{
    FDCAN_Context *ctx = (FDCAN_Context *)drv->ctx;
    uint32_t opmode = FDCAN_OPERATION_MODE_NORMAL;
    switch (mode) {
    case CAN_MODE_LOOPBACK:
        opmode = FDCAN_OPERATION_MODE_INTERNAL_LOOPBACK;
        break;
    case CAN_MODE_SILENT:
    case CAN_MODE_AUTOBAUD:
        opmode = FDCAN_OPERATION_MODE_BUS_MONITORING;
        break;
    case CAN_MODE_NORMAL:
    default:
        opmode = FDCAN_OPERATION_MODE_NORMAL;
        break;
    }
    HAL_FDCAN_Stop(&ctx->hfdcan);
    ctx->hfdcan.Init.Mode = opmode;
    HAL_FDCAN_Init(&ctx->hfdcan);
    return HAL_FDCAN_Start(&ctx->hfdcan) == HAL_OK ? CAN_OK : CAN_ERROR;
}

static void fd_config_bitrate(FDCAN_Context *ctx, uint32_t bitrate)
{
    const uint32_t pclk = 48000000U;
    const uint32_t tq = 16U;
    ctx->hfdcan.Init.NominalPrescaler = pclk / (bitrate * tq);
    ctx->hfdcan.Init.NominalSyncJumpWidth = 1;
    ctx->hfdcan.Init.NominalTimeSeg1 = 13;
    ctx->hfdcan.Init.NominalTimeSeg2 = 2;
    HAL_FDCAN_Init(&ctx->hfdcan);
}

static uint32_t fd_get_error(ICANDriver *drv)
{
    FDCAN_Context *ctx = (FDCAN_Context *)drv->ctx;
    return HAL_FDCAN_GetError(&ctx->hfdcan);
}

static CAN_Result_t fd_autobaud(ICANDriver *drv, const uint32_t *rates, uint8_t num)
{
    if (!drv || !rates || num == 0)
        return CAN_ERROR;

    FDCAN_Context *ctx = (FDCAN_Context *)drv->ctx;
    CAN_Message_t msg;

    for (uint8_t i = 0; i < num; ++i) {
        uint32_t br = rates[i];
        if (br == 0)
            continue;

        HAL_FDCAN_Stop(&ctx->hfdcan);
        ctx->hfdcan.Init.Mode = FDCAN_OPERATION_MODE_BUS_MONITORING;
        fd_config_bitrate(ctx, br);
        HAL_FDCAN_Start(&ctx->hfdcan);
        printf("FDCAN autobaud try %lu\n", (unsigned long)br);

        for (int attempt = 0; attempt < 100; ++attempt) {
            if (fd_receive(drv, &msg) == CAN_OK) {
                printf("FDCAN autobaud detected %lu\n", (unsigned long)br);
                HAL_FDCAN_Stop(&ctx->hfdcan);
                ctx->hfdcan.Init.Mode = FDCAN_OPERATION_MODE_NORMAL;
                fd_config_bitrate(ctx, br);
                HAL_FDCAN_Start(&ctx->hfdcan);
                return CAN_OK;
            }
        }
    }

    return CAN_ERROR;
}

static void fd_enable_interrupts(ICANDriver *drv)
{
    FDCAN_Context *ctx = (FDCAN_Context *)drv->ctx;
    HAL_FDCAN_ActivateNotification(&ctx->hfdcan,
                                   FDCAN_IT_RX_FIFO0_NEW_MESSAGE |
                                   FDCAN_IT_TX_FIFO_EMPTY |
                                   FDCAN_IT_ERROR_WARNING);
}

static void fd_disable_interrupts(ICANDriver *drv)
{
    FDCAN_Context *ctx = (FDCAN_Context *)drv->ctx;
    HAL_FDCAN_DeactivateNotification(&ctx->hfdcan,
                                     FDCAN_IT_RX_FIFO0_NEW_MESSAGE |
                                     FDCAN_IT_TX_FIFO_EMPTY |
                                     FDCAN_IT_ERROR_WARNING);
}

static void fd_irq_handler(ICANDriver *drv)
{
    FDCAN_Context *ctx = (FDCAN_Context *)drv->ctx;
    HAL_FDCAN_IRQHandler(&ctx->hfdcan);
}

void HAL_FDCAN_RxFifo0Callback(FDCAN_HandleTypeDef *hfdcan, uint32_t RxFifo0ITs)
{
    (void)RxFifo0ITs;
    FDCAN_Context *ctx = GET_CTX(hfdcan);
    FDCAN_RxHeaderTypeDef hdr;
    CAN_Message_t msg;
    if (HAL_FDCAN_GetRxMessage(hfdcan, FDCAN_RX_FIFO0, &hdr, msg.data) == HAL_OK) {
        msg.id = hdr.Identifier;
        msg.extended = (hdr.IdType == FDCAN_EXTENDED_ID) ? 1 : 0;
        msg.dlc = hdr.DataLength >> 16;
        CAN_Manager_TriggerEvent(ctx->base.inst_id, CAN_EVENT_RX, &msg);
    }
}

void HAL_FDCAN_TxFifoEmptyCallback(FDCAN_HandleTypeDef *hfdcan)
{
    FDCAN_Context *ctx = GET_CTX(hfdcan);
    CAN_Manager_TriggerEvent(ctx->base.inst_id, CAN_EVENT_TX_COMPLETE, NULL);
}

void HAL_FDCAN_ErrorCallback(FDCAN_HandleTypeDef *hfdcan)
{
    FDCAN_Context *ctx = GET_CTX(hfdcan);
    CAN_Manager_TriggerEvent(ctx->base.inst_id, CAN_EVENT_ERROR, NULL);
}

static ICANDriver fd_template = {
    .init            = fd_init,
    .send            = fd_send,
    .receive         = fd_receive,
    .set_filter      = fd_set_filter,
    .set_mode        = fd_set_mode,
    .get_error_state = fd_get_error,
    .auto_baud_detect = fd_autobaud,
    .enable_interrupts = fd_enable_interrupts,
    .disable_interrupts = fd_disable_interrupts,
    .irq_handler     = fd_irq_handler,
    .ctx             = NULL
};

void FDCAN_SetupDriver(ICANDriver *drv, FDCAN_Context *ctx, FDCAN_GlobalTypeDef *inst)
{
    if (!drv || !ctx)
        return;

    *drv = fd_template;
    ctx->hfdcan.Instance = inst;
    ctx->driver = drv;
    drv->ctx = ctx;
}


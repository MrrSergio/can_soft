#include "can_stm32_bxcan.h"
#include "can_manager.h"
#include <stdio.h>

/*
 * This driver demonstrates how the portable CAN layer can be connected to the
 * STM32 HAL bxCAN API. The previous revision of this file contained minimal
 * stubs so the example could compile without the HAL.  The implementation
 * below expects the real HAL headers to be available and calls into the HAL
 * directly.
 */

/* ----- Driver implementation -------------------------------------------- */

typedef struct {
    CAN_DriverContext_t base;
    CAN_HandleTypeDef   hcan;
} BxCAN_Context;

/* Forward declarations for helpers used in init */
static CAN_Result_t bx_set_filter(ICANDriver *drv, uint32_t id, uint32_t mask);
static CAN_Result_t bx_set_mode(ICANDriver *drv, CAN_Mode_t mode);
static void         bx_config_bitrate(BxCAN_Context *ctx, uint32_t bitrate);

static CAN_Result_t bx_init(ICANDriver *drv, const CAN_Config_t *cfg)
{
    BxCAN_Context *ctx = (BxCAN_Context *)drv->ctx;
    if (!ctx)
        return CAN_ERROR;

    bx_config_bitrate(ctx, cfg ? cfg->bitrate : 500000);
    if (HAL_CAN_Start(&ctx->hcan) != HAL_OK)
        return CAN_ERROR;

    if (cfg) {
        bx_set_filter(drv, cfg->filter_id, cfg->filter_mask);
        bx_set_mode(drv, cfg->mode);
    }
    return CAN_OK;
}

static CAN_Result_t bx_send(ICANDriver *drv, const CAN_Message_t *msg, uint32_t timeout)
{
    (void)timeout; (void)drv;
    if (!msg)
        return CAN_ERROR;
    CAN_TxHeaderTypeDef hdr = {
        .StdId = msg->id,
        .ExtId = msg->id,
        .IDE   = msg->extended ? 1 : 0,
        .RTR   = 0,
        .DLC   = msg->dlc
    };
    BxCAN_Context *ctx = (BxCAN_Context *)drv->ctx;
    HAL_CAN_AddTxMessage(&ctx->hcan, &hdr, (uint8_t *)msg->data, NULL);
    CAN_Manager_TriggerEvent(ctx->base.inst_id,
                             CAN_EVENT_TX_COMPLETE, (void *)msg);
    return CAN_OK;
}

static CAN_Result_t bx_receive(ICANDriver *drv, CAN_Message_t *msg)
{
    CAN_RxHeaderTypeDef hdr;
    BxCAN_Context *ctx = (BxCAN_Context *)drv->ctx;
    if (!msg)
        return CAN_ERROR;
    if (HAL_CAN_GetRxMessage(&ctx->hcan, 0, &hdr, msg->data) == HAL_OK) {
        msg->id       = hdr.IDE ? hdr.ExtId : hdr.StdId;
        msg->extended = hdr.IDE ? 1 : 0;
        msg->dlc      = hdr.DLC;
        CAN_Manager_TriggerEvent(ctx->base.inst_id,
                                 CAN_EVENT_RX, msg);
        return CAN_OK;
    }
    return CAN_ERROR;
}

static CAN_Result_t bx_set_filter(ICANDriver *drv, uint32_t id, uint32_t mask)
{
    BxCAN_Context *ctx = (BxCAN_Context *)drv->ctx;
    CAN_FilterTypeDef f;
    f.FilterBank = 0;
    f.FilterMode = CAN_FILTERMODE_IDMASK;
    f.FilterScale = CAN_FILTERSCALE_32BIT;
    f.FilterIdHigh = id << 5;
    f.FilterIdLow = 0;
    f.FilterMaskIdHigh = mask << 5;
    f.FilterMaskIdLow = 0;
    f.FilterFIFOAssignment = CAN_FILTER_FIFO0;
    f.FilterActivation = ENABLE;
    f.SlaveStartFilterBank = 0;
    HAL_CAN_ConfigFilter(&ctx->hcan, &f);
    return CAN_OK;
}

static CAN_Result_t bx_set_mode(ICANDriver *drv, CAN_Mode_t mode)
{
    BxCAN_Context *ctx = (BxCAN_Context *)drv->ctx;
    uint32_t hal_mode = CAN_MODE_NORMAL;
    switch (mode) {
    case CAN_MODE_LOOPBACK:
        hal_mode = CAN_MODE_LOOPBACK;
        break;
    case CAN_MODE_SILENT:
    case CAN_MODE_AUTOBAUD:
        hal_mode = CAN_MODE_SILENT;
        break;
    case CAN_MODE_NORMAL:
    default:
        hal_mode = CAN_MODE_NORMAL;
        break;
    }
    HAL_CAN_Stop(&ctx->hcan);
    ctx->hcan.Init.Mode = hal_mode;
    HAL_CAN_Init(&ctx->hcan);
    return HAL_CAN_Start(&ctx->hcan) == HAL_OK ? CAN_OK : CAN_ERROR;
}

static void bx_config_bitrate(BxCAN_Context *ctx, uint32_t bitrate)
{
    const uint32_t pclk = 36000000U; /* simulated peripheral clock */
    const uint32_t tq   = 16U;       /* number of time quanta */

    ctx->hcan.Init.Prescaler = pclk / (bitrate * tq);
    ctx->hcan.Init.SyncJumpWidth = CAN_SJW_1TQ;
    ctx->hcan.Init.TimeSeg1      = CAN_BS1_13TQ;
    ctx->hcan.Init.TimeSeg2      = CAN_BS2_2TQ;
    ctx->hcan.Init.TimeTriggeredMode   = DISABLE;
    ctx->hcan.Init.AutoBusOff          = DISABLE;
    ctx->hcan.Init.AutoWakeUp          = DISABLE;
    ctx->hcan.Init.AutoRetransmission  = DISABLE;
    ctx->hcan.Init.ReceiveFifoLocked   = DISABLE;
    ctx->hcan.Init.TransmitFifoPriority = DISABLE;
    HAL_CAN_Init(&ctx->hcan);
}

static uint32_t bx_get_error(ICANDriver *drv)
{
    BxCAN_Context *ctx = (BxCAN_Context *)drv->ctx;
    return HAL_CAN_GetError(&ctx->hcan);
}

static CAN_Result_t bx_autobaud(ICANDriver *drv, const uint32_t *rates, uint8_t num)
{
    if (!drv || !rates || num == 0)
        return CAN_ERROR;

    BxCAN_Context *ctx = (BxCAN_Context *)drv->ctx;
    CAN_Message_t msg;

    for (uint8_t i = 0; i < num; ++i) {
        uint32_t br = rates[i];
        if (br == 0)
            continue;

        HAL_CAN_Stop(&ctx->hcan);
        ctx->hcan.Init.Mode = CAN_MODE_SILENT;
        bx_config_bitrate(ctx, br);
        HAL_CAN_Start(&ctx->hcan);
        printf("bxCAN autobaud try %lu\n", (unsigned long)br);

        for (int attempt = 0; attempt < 100; ++attempt) {
            if (bx_receive(drv, &msg) == CAN_OK) {
                printf("bxCAN autobaud detected %lu\n", (unsigned long)br);
                HAL_CAN_Stop(&ctx->hcan);
                ctx->hcan.Init.Mode = CAN_MODE_NORMAL;
                bx_config_bitrate(ctx, br);
                HAL_CAN_Start(&ctx->hcan);
                return CAN_OK;
            }
        }
    }

    return CAN_ERROR;
}

static ICANDriver bx_template = {
    .init            = bx_init,
    .send            = bx_send,
    .receive         = bx_receive,
    .set_filter      = bx_set_filter,
    .set_mode        = bx_set_mode,
    .get_error_state = bx_get_error,
    .auto_baud_detect = bx_autobaud,
    .ctx             = NULL
};

void BxCAN_SetupDriver(ICANDriver *drv, BxCAN_Context *ctx, CAN_TypeDef *inst)
{
    if (!drv || !ctx)
        return;

    *drv = bx_template;
    ctx->hcan.Instance = inst;
    drv->ctx = ctx;
}

#include "can_stm32_bxcan.h"
#include "can_manager.h"
#include <stdio.h>

/*
 * This driver demonstrates how the portable CAN layer could be connected to
 * the STM32 HAL bxCAN API. The STM32 HAL is not available in this build
 * environment, therefore minimal stubs of the required types and functions are
 * provided so the code compiles and behaves in a predictable manner for the
 * example program.
 */

/* ----- Minimal HAL subset ------------------------------------------------ */

typedef struct {
    uint32_t StdId;
    uint32_t ExtId;
    uint32_t IDE;
    uint32_t RTR;
    uint32_t DLC;
} CAN_TxHeaderTypeDef;

typedef struct {
    uint32_t StdId;
    uint32_t ExtId;
    uint32_t IDE;
    uint32_t RTR;
    uint32_t DLC;
} CAN_RxHeaderTypeDef;

typedef struct {
    uint32_t FilterIdHigh;
    uint32_t FilterMaskIdHigh;
} CAN_FilterTypeDef;

typedef struct {
    struct {
        uint32_t Prescaler;
        uint32_t Mode;
        uint32_t SJW;
        uint32_t BS1;
        uint32_t BS2;
    } Init;
} CAN_HandleTypeDef;

typedef enum { HAL_OK = 0, HAL_ERROR } HAL_StatusTypeDef;

static HAL_StatusTypeDef HAL_CAN_Init(CAN_HandleTypeDef *h) { (void)h; return HAL_OK; }
static HAL_StatusTypeDef HAL_CAN_Start(CAN_HandleTypeDef *h) { (void)h; return HAL_OK; }
static HAL_StatusTypeDef HAL_CAN_Stop(CAN_HandleTypeDef *h) { (void)h; return HAL_OK; }
static HAL_StatusTypeDef HAL_CAN_ConfigFilter(CAN_HandleTypeDef *h, const CAN_FilterTypeDef *f)
{ (void)h; (void)f; return HAL_OK; }
static HAL_StatusTypeDef HAL_CAN_AddTxMessage(CAN_HandleTypeDef *h, const CAN_TxHeaderTypeDef *hdr,
                                             uint8_t *data, uint32_t *mailbox)
{ (void)h; (void)hdr; (void)data; if (mailbox) *mailbox = 0; return HAL_OK; }
static HAL_StatusTypeDef HAL_CAN_GetRxMessage(CAN_HandleTypeDef *h, uint32_t fifo, CAN_RxHeaderTypeDef *hdr,
                                              uint8_t *data)
{ (void)h; (void)fifo; (void)hdr; (void)data; return HAL_ERROR; }
static uint32_t HAL_CAN_GetError(CAN_HandleTypeDef *h) { (void)h; return 0; }

/* ----- Driver implementation -------------------------------------------- */

typedef struct {
    CAN_DriverContext_t base;
    CAN_HandleTypeDef hcan;
    int dummy; /* used to simulate bitrate register for autobaud */
} BxCAN_Context;

static BxCAN_Context bx_ctx;
/* Forward declarations for helpers used in init */
static CAN_Result_t bx_set_filter(ICANDriver *drv, uint32_t id, uint32_t mask);
static CAN_Result_t bx_set_mode(ICANDriver *drv, CAN_Mode_t mode);
static void         bx_config_bitrate(BxCAN_Context *ctx, uint32_t bitrate);

static CAN_Result_t bx_init(ICANDriver *drv, const CAN_Config_t *cfg)
{
    BxCAN_Context *ctx = &bx_ctx;
    bx_config_bitrate(ctx, cfg ? cfg->bitrate : 500000);
    (void)HAL_CAN_Start(&ctx->hcan);
    drv->ctx = ctx;
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
    CAN_FilterTypeDef f = { .FilterIdHigh = id, .FilterMaskIdHigh = mask };
    HAL_CAN_ConfigFilter(&ctx->hcan, &f);
    return CAN_OK;
}

static CAN_Result_t bx_set_mode(ICANDriver *drv, CAN_Mode_t mode)
{
    BxCAN_Context *ctx = (BxCAN_Context *)drv->ctx;
    ctx->hcan.Init.Mode = mode;
    return HAL_CAN_Start(&ctx->hcan) == HAL_OK ? CAN_OK : CAN_ERROR;
}

static void bx_config_bitrate(BxCAN_Context *ctx, uint32_t bitrate)
{
    const uint32_t pclk = 36000000U; /* simulated peripheral clock */
    const uint32_t tq   = 16U;       /* number of time quanta */

    ctx->hcan.Init.Prescaler = pclk / (bitrate * tq);
    ctx->hcan.Init.SJW       = 1;
    ctx->hcan.Init.BS1       = 13;
    ctx->hcan.Init.BS2       = 2;
    ctx->dummy               = (int)bitrate;
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

ICANDriver stm32_can1_driver = {
    .init = bx_init,
    .send = bx_send,
    .receive = bx_receive,
    .set_filter = bx_set_filter,
    .set_mode = bx_set_mode,
    .get_error_state = bx_get_error,
    .auto_baud_detect = bx_autobaud,
    .ctx = NULL
};

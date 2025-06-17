#include "can_mcp2515.h"
#include "can_manager.h"
#include <stdio.h>

typedef struct {
    CAN_DriverContext_t base;
    int dummy;
    CAN_Message_t echo_msg;
    int echo_pending;
} MCP2515_Context;

static MCP2515_Context mcp_ctx;

static CAN_Result_t mcp_init(ICANDriver *drv, const CAN_Config_t *cfg)
{
    (void)cfg;
    drv->ctx = &mcp_ctx;
    mcp_ctx.echo_pending = 0;
    return CAN_OK;
}

static CAN_Result_t mcp_send(ICANDriver *drv, const CAN_Message_t *msg, uint32_t timeout)
{
    (void)timeout;
    MCP2515_Context *ctx = (MCP2515_Context *)drv->ctx;
    if (msg && ctx) {
        printf("MCP2515 send id: 0x%lx\n", (unsigned long)msg->id);
        ctx->echo_msg = *msg;
        ctx->echo_pending = 1;
        CAN_Manager_TriggerEvent(ctx->base.inst_id,
                                 CAN_EVENT_TX_COMPLETE, (void *)msg);
    }
    return CAN_OK;
}

static CAN_Result_t mcp_receive(ICANDriver *drv, CAN_Message_t *msg)
{
    MCP2515_Context *ctx = (MCP2515_Context *)drv->ctx;
    if (ctx && ctx->echo_pending && msg) {
        *msg = ctx->echo_msg;
        ctx->echo_pending = 0;
        CAN_Manager_TriggerEvent(ctx->base.inst_id,
                                 CAN_EVENT_RX, msg);
        return CAN_OK;
    }
    return CAN_ERROR;
}

static CAN_Result_t mcp_set_filter(ICANDriver *drv, uint32_t id, uint32_t mask)
{
    (void)drv; (void)id; (void)mask;
    return CAN_OK;
}

static CAN_Result_t mcp_set_mode(ICANDriver *drv, CAN_Mode_t mode)
{
    (void)drv; (void)mode;
    return CAN_OK;
}

static uint32_t mcp_get_error(ICANDriver *drv)
{
    (void)drv;
    return 0;
}

static CAN_Result_t mcp_autobaud(ICANDriver *drv, const uint32_t *rates, uint8_t num)
{
    (void)drv; (void)rates; (void)num;
    return CAN_OK;
}

ICANDriver mcp2515_driver = {
    .init = mcp_init,
    .send = mcp_send,
    .receive = mcp_receive,
    .set_filter = mcp_set_filter,
    .set_mode = mcp_set_mode,
    .get_error_state = mcp_get_error,
    .auto_baud_detect = mcp_autobaud,
    .ctx = NULL
};

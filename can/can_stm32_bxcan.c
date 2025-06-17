#include "can_stm32_bxcan.h"
#include <stdio.h>

static CAN_Result_t bx_init(ICANDriver *drv, const CAN_Config_t *cfg)
{
    (void)cfg; (void)drv;
    return CAN_OK;
}

static CAN_Result_t bx_send(ICANDriver *drv, const CAN_Message_t *msg, uint32_t timeout)
{
    (void)drv; (void)timeout;
    if (msg)
        printf("STM32 bxCAN send id: 0x%lx\n", (unsigned long)msg->id);
    return CAN_OK;
}

static CAN_Result_t bx_receive(ICANDriver *drv, CAN_Message_t *msg)
{
    (void)drv; (void)msg;
    return CAN_ERROR;
}

static CAN_Result_t bx_set_filter(ICANDriver *drv, uint32_t id, uint32_t mask)
{
    (void)drv; (void)id; (void)mask;
    return CAN_OK;
}

static CAN_Result_t bx_set_mode(ICANDriver *drv, CAN_Mode_t mode)
{
    (void)drv; (void)mode;
    return CAN_OK;
}

static uint32_t bx_get_error(ICANDriver *drv)
{
    (void)drv;
    return 0;
}

static CAN_Result_t bx_autobaud(ICANDriver *drv, const uint32_t *rates, uint8_t num)
{
    (void)drv; (void)rates; (void)num;
    return CAN_OK;
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

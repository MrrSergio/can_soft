#include "can_manager.h"
#include "can_config.h"
#include <string.h>

#define MAX_CAN_INTERFACES 4

typedef struct {
    CAN_Message_t tx_queue[CAN_TX_QUEUE_LEN];
    uint16_t tx_head, tx_tail;
    CAN_Message_t rx_queue[CAN_RX_QUEUE_LEN];
    uint16_t rx_head, rx_tail;
} CAN_Buffer_t;

typedef struct {
    ICANDriver *driver;
    void *driver_ctx;
    CAN_Callback_t callbacks[3];
    CAN_Buffer_t buffers;
    uint32_t filter_id;
    uint32_t filter_mask;
} CAN_Instance_t;

static CAN_Instance_t can_instances[MAX_CAN_INTERFACES];
static uint8_t can_instances_count = 0;

/* Exported so drivers can notify about asynchronous events */
void CAN_Manager_TriggerEvent(uint8_t inst_id, CAN_Event_t event, void *arg);

void CAN_Manager_Init(void)
{
    memset(can_instances, 0, sizeof(can_instances));
    can_instances_count = 0;
}

int CAN_Manager_AddInterface(ICANDriver *driver, const CAN_Config_t *config)
{
    if (can_instances_count >= MAX_CAN_INTERFACES || !driver || !driver->init) {
        return -1;
    }
    if (driver->init(driver, config) != CAN_OK) {
        return -1;
    }
    can_instances[can_instances_count].driver = driver;
    can_instances[can_instances_count].driver_ctx = driver->ctx;
    if (config) {
        can_instances[can_instances_count].filter_id = config->filter_id;
        can_instances[can_instances_count].filter_mask = config->filter_mask;
    } else {
        can_instances[can_instances_count].filter_id = 0;
        can_instances[can_instances_count].filter_mask = 0;
    }
    memset(can_instances[can_instances_count].callbacks, 0, sizeof(CAN_Callback_t) * 3);
    CAN_Buffer_t *buf = &can_instances[can_instances_count].buffers;
    buf->tx_head = buf->tx_tail = 0;
    buf->rx_head = buf->rx_tail = 0;
    /* store instance id in driver context if available */
    if (driver->ctx) {
        CAN_DriverContext_t *ctx = (CAN_DriverContext_t *)driver->ctx;
        ctx->inst_id = can_instances_count;
    }
    return can_instances_count++;
}

CAN_Result_t CAN_SendMessage(uint8_t inst_id, const CAN_Message_t *msg)
{
    if (inst_id >= can_instances_count) {
        return CAN_ERROR;
    }
    CAN_Buffer_t *buf = &can_instances[inst_id].buffers;
    uint16_t next = (buf->tx_head + 1) % CAN_TX_QUEUE_LEN;
    if (next == buf->tx_tail)
        return CAN_ERROR; /* full */
    buf->tx_queue[buf->tx_head] = *msg;
    buf->tx_head = next;
    return CAN_OK;
}

void CAN_RegisterCallback(uint8_t inst_id, CAN_Event_t event, CAN_Callback_t cb)
{
    if (inst_id >= can_instances_count || event > CAN_EVENT_ERROR)
        return;
    can_instances[inst_id].callbacks[event] = cb;
}

CAN_Result_t CAN_SetFilter(uint8_t inst_id, uint32_t id, uint32_t mask)
{
    if (inst_id >= can_instances_count)
        return CAN_ERROR;
    ICANDriver *drv = can_instances[inst_id].driver;
    if (!drv || !drv->set_filter)
        return CAN_ERROR;
    can_instances[inst_id].filter_id = id;
    can_instances[inst_id].filter_mask = mask;
    return drv->set_filter(drv, id, mask);
}

int CAN_GetMessage(uint8_t inst_id, CAN_Message_t *msg)
{
    if (inst_id >= can_instances_count || !msg)
        return -1;
    CAN_Buffer_t *buf = &can_instances[inst_id].buffers;
    if (buf->rx_head == buf->rx_tail)
        return -1;
    *msg = buf->rx_queue[buf->rx_tail];
    buf->rx_tail = (buf->rx_tail + 1) % CAN_RX_QUEUE_LEN;
    return 0;
}

CAN_Result_t CAN_StartAutoBaud(uint8_t inst_id, const uint32_t *rates, uint8_t num)
{
    if (inst_id >= can_instances_count)
        return CAN_ERROR;
    ICANDriver *drv = can_instances[inst_id].driver;
    if (drv && drv->auto_baud_detect) {
        return drv->auto_baud_detect(drv, rates, num);
    }
    return CAN_ERROR;
}

void CAN_Manager_Process(void)
{
    for (uint8_t i = 0; i < can_instances_count; ++i) {
        CAN_Instance_t *inst = &can_instances[i];
        ICANDriver *drv = inst->driver;
        CAN_Buffer_t *buf = &inst->buffers;
        if (!drv)
            continue;

        if (buf->tx_head != buf->tx_tail && drv->send) {
            CAN_Message_t *msg = &buf->tx_queue[buf->tx_tail];
            if (drv->send(drv, msg, 0) == CAN_OK) {
                buf->tx_tail = (buf->tx_tail + 1) % CAN_TX_QUEUE_LEN;
            }
        }

        if (drv->receive) {
            CAN_Message_t rx;
            while (drv->receive(drv, &rx) == CAN_OK) {
                uint16_t next = (buf->rx_head + 1) % CAN_RX_QUEUE_LEN;
                if (next == buf->rx_tail)
                    break; /* drop if full */
                buf->rx_queue[buf->rx_head] = rx;
                buf->rx_head = next;
            }
        }
    }
}

/* Called by drivers or internal processing to dispatch events to registered
 * callbacks. */
void CAN_Manager_TriggerEvent(uint8_t inst_id, CAN_Event_t event, void *arg)
{
    if (inst_id >= can_instances_count)
        return;
    CAN_Callback_t cb = can_instances[inst_id].callbacks[event];
    if (cb)
        cb(inst_id, event, arg);
}

#include "can_manager.h"

void CAN_Driver_OnTxComplete(uint32_t mailbox)
{
    CAN_Manager_TriggerEvent(CAN_EVENT_TX_COMPLETE, mailbox);
}

void CAN_Driver_OnRx(uint32_t msg_id)
{
    CAN_Manager_TriggerEvent(CAN_EVENT_RX_MSG, msg_id);
}

void CAN_Driver_OnError(uint32_t error)
{
    CAN_Manager_TriggerEvent(CAN_EVENT_ERROR, error);
}

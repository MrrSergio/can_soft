#include "can_manager.h"
#include <stdio.h>

void CAN_Manager_Init(void)
{
    /* Initialization code */
}

void CAN_Manager_TriggerEvent(CAN_Event_t event, uint32_t param)
{
    /* In a real implementation this would notify the manager */
    /* For now simply print the event for demonstration */
    switch(event) {
        case CAN_EVENT_TX_COMPLETE:
            printf("TX complete: %u\n", param);
            break;
        case CAN_EVENT_RX_MSG:
            printf("RX message: %u\n", param);
            break;
        case CAN_EVENT_ERROR:
            printf("Error: %u\n", param);
            break;
        default:
            break;
    }
}

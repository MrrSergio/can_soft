#ifndef CAN_MANAGER_H
#define CAN_MANAGER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

typedef enum {
    CAN_EVENT_TX_COMPLETE,
    CAN_EVENT_RX_MSG,
    CAN_EVENT_ERROR
} CAN_Event_t;

void CAN_Manager_Init(void);

void CAN_Manager_TriggerEvent(CAN_Event_t event, uint32_t param);

#ifdef __cplusplus
}
#endif

#endif /* CAN_MANAGER_H */

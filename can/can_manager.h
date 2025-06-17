#ifndef CAN_MANAGER_H
#define CAN_MANAGER_H

#include "can_interface.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    CAN_EVENT_RX,
    CAN_EVENT_TX_COMPLETE,
    CAN_EVENT_ERROR
} CAN_Event_t;

typedef void (*CAN_Callback_t)(uint8_t inst_id, CAN_Event_t event, void *arg);

void CAN_Manager_Init(void);
int  CAN_Manager_AddInterface(ICANDriver *driver, const CAN_Config_t *config);
CAN_Result_t CAN_SendMessage(uint8_t inst_id, const CAN_Message_t *msg);
int CAN_GetMessage(uint8_t inst_id, CAN_Message_t *msg);
void CAN_RegisterCallback(uint8_t inst_id, CAN_Event_t event, CAN_Callback_t cb);
CAN_Result_t CAN_StartAutoBaud(uint8_t inst_id, const uint32_t *rates, uint8_t num);
void CAN_Manager_Process(void);

#ifdef __cplusplus
}
#endif

#endif /* CAN_MANAGER_H */

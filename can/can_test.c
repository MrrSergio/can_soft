#include "can_manager.h"
#include "can_mcp2515.h"
#include "can_stm32_bxcan.h"
#include "can_autobaud.h"
#include "can_config.h"
#include <stdio.h>

static void on_rx(uint8_t id, CAN_Event_t ev, void *arg)
{
    (void)id; (void)ev; (void)arg;
    printf("Message received callback\n");
}

int main(void)
{
    CAN_Config_t config = {
        .mode = CAN_MODE_AUTOBAUD,
        .bitrate = 0,
        .filter_id = 0x100,
        .filter_mask = 0x700
    };

    CAN_Manager_Init();
    int id0 = CAN_Manager_AddInterface(&mcp2515_driver, &config);
    int id1 = CAN_Manager_AddInterface(&stm32_can1_driver, &config);
    printf("Interfaces %d %d added\n", id0, id1);

    CAN_RegisterCallback(id0, CAN_EVENT_RX, on_rx);
    CAN_Message_t msg = { .id = 0x123, .dlc = 1, .data = {0x55}, .extended = 0 };
    CAN_SendMessage(id0, &msg);
    CAN_Manager_Process();
    CAN_StartAutoBaud(id0, default_bitrates, CAN_MAX_BITRATES);

    return 0;
}

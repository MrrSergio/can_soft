#include "can_manager.h"
#include "can_mcp2515.h"
#include "can_stm32_bxcan.h"
#include "can_stm32_fdcan.h"
#include "can_autobaud.h"
#include "can_config.h"
#include <stdio.h>

static void on_rx(uint8_t id, CAN_Event_t ev, void *arg)
{
    CAN_Message_t *m = (CAN_Message_t *)arg;
    printf("RX iface %u id 0x%lx\n", id, (unsigned long)m->id);
}

static void on_tx(uint8_t id, CAN_Event_t ev, void *arg)
{
    CAN_Message_t *m = (CAN_Message_t *)arg;
    printf("TX complete iface %u id 0x%lx\n", id, (unsigned long)m->id);
}

static void on_err(uint8_t id, CAN_Event_t ev, void *arg)
{
    (void)ev; (void)arg;
    printf("Error on iface %u\n", id);
}

int main(void)
{
    CAN_Config_t cfg0 = {
        .mode = CAN_MODE_AUTOBAUD,
        .bitrate = 0,
        .filter_id = 0x100,
        .filter_mask = 0x700
    };
    CAN_Config_t cfg1 = {
        .mode = CAN_MODE_NORMAL,
        .bitrate = 500000,
        .filter_id = 0,
        .filter_mask = 0
    };

    CAN_Manager_Init();
    int id0 = CAN_Manager_AddInterface(&mcp2515_driver, &cfg0);
    BxCAN_Context bx1, bx2;
    FDCAN_Context fd1;
    ICANDriver bx_drv1, bx_drv2, fd_drv;
    BxCAN_SetupDriver(&bx_drv1, &bx1, CAN1);
    BxCAN_SetupDriver(&bx_drv2, &bx2, CAN2);
    FDCAN_SetupDriver(&fd_drv, &fd1, FDCAN1);
    int id1 = CAN_Manager_AddInterface(&bx_drv1, &cfg1);
    int id2 = CAN_Manager_AddInterface(&bx_drv2, &cfg1);
    int id3 = CAN_Manager_AddInterface(&fd_drv, &cfg1);
    printf("Interfaces %d %d %d %d added\n", id0, id1, id2, id3);

    CAN_RegisterCallback(id0, CAN_EVENT_RX, on_rx);
    CAN_RegisterCallback(id0, CAN_EVENT_TX_COMPLETE, on_tx);
    CAN_RegisterCallback(id0, CAN_EVENT_ERROR, on_err);
    CAN_RegisterCallback(id1, CAN_EVENT_RX, on_rx);
    CAN_RegisterCallback(id1, CAN_EVENT_TX_COMPLETE, on_tx);
    CAN_RegisterCallback(id1, CAN_EVENT_ERROR, on_err);
    CAN_RegisterCallback(id2, CAN_EVENT_RX, on_rx);
    CAN_RegisterCallback(id2, CAN_EVENT_TX_COMPLETE, on_tx);
    CAN_RegisterCallback(id2, CAN_EVENT_ERROR, on_err);
    CAN_RegisterCallback(id3, CAN_EVENT_RX, on_rx);
    CAN_RegisterCallback(id3, CAN_EVENT_TX_COMPLETE, on_tx);
    CAN_RegisterCallback(id3, CAN_EVENT_ERROR, on_err);

    /* Autobaud on MCP2515 and FDCAN interfaces */
    CAN_StartAutoBaud(id0, default_bitrates, CAN_MAX_BITRATES);
    CAN_StartAutoBaud(id3, default_bitrates, CAN_MAX_BITRATES);

    /* Adjust filter and loopback mode directly via driver */
    mcp2515_driver.set_filter(&mcp2515_driver, 0x200, 0x7FF);
    mcp2515_driver.set_mode(&mcp2515_driver, CAN_MODE_LOOPBACK);

    CAN_Message_t msg = { .id = 0x123, .dlc = 2, .data = {0x55, 0xAA}, .extended = 0 };
    CAN_SendMessage(id0, &msg);
    CAN_SendMessage(id1, &msg);
    CAN_SendMessage(id2, &msg);
    CAN_SendMessage(id3, &msg);

    for (int i = 0; i < 3; ++i) {
        CAN_Manager_Process();
    }

    CAN_Message_t rx;
    while (CAN_GetMessage(id0, &rx) == 0) {
        printf("Polled RX from %d id 0x%lx\n", id0, (unsigned long)rx.id);
    }
    while (CAN_GetMessage(id1, &rx) == 0) {
        printf("Polled RX from %d id 0x%lx\n", id1, (unsigned long)rx.id);
    }
    while (CAN_GetMessage(id2, &rx) == 0) {
        printf("Polled RX from %d id 0x%lx\n", id2, (unsigned long)rx.id);
    }
    while (CAN_GetMessage(id3, &rx) == 0) {
        printf("Polled RX from %d id 0x%lx\n", id3, (unsigned long)rx.id);
    }

    /* Query error state via driver */
    printf("Error state iface %d: %lu\n", id0,
           (unsigned long)mcp2515_driver.get_error_state(&mcp2515_driver));
    printf("Error state iface %d: %lu\n", id1,
           (unsigned long)bx_drv1.get_error_state(&bx_drv1));
    printf("Error state iface %d: %lu\n", id2,
           (unsigned long)bx_drv2.get_error_state(&bx_drv2));
    printf("Error state iface %d: %lu\n", id3,
           (unsigned long)fd_drv.get_error_state(&fd_drv));

    return 0;
}

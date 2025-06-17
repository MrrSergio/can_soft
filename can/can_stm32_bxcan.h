#ifndef CAN_STM32_BXCAN_H
#define CAN_STM32_BXCAN_H

#include <stdint.h>
#include "stm32f1xx_hal_can.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Basic CAN frame structure */
struct can_frame {
    uint32_t id;      /* 11 or 29 bit identifier */
    uint8_t  dlc;     /* Data length code 0..8 */
    uint8_t  data[8]; /* Payload */
};

/* Driver handle */
struct bxcan_driver {
    CAN_HandleTypeDef *hcan;
};

int bx_init(struct bxcan_driver *drv, uint32_t bitrate);
int bx_send(struct bxcan_driver *drv, const struct can_frame *frm);
int bx_receive(struct bxcan_driver *drv, struct can_frame *frm);
int bx_set_filter(struct bxcan_driver *drv, int idx, uint32_t id, uint32_t mask);
int bx_set_mode(struct bxcan_driver *drv, int loopback, int silent);
uint32_t bx_get_error(struct bxcan_driver *drv);
int bx_autobaud(struct bxcan_driver *drv);

#ifdef __cplusplus
}
#endif

#endif /* CAN_STM32_BXCAN_H */

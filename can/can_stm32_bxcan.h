#ifndef CAN_STM32_BXCAN_H
#define CAN_STM32_BXCAN_H


#include "can_interface.h"
#include "stm32f1xx_hal.h"

typedef struct {
    CAN_DriverContext_t base;
    CAN_HandleTypeDef   hcan;
} BxCAN_Context;

void BxCAN_SetupDriver(ICANDriver *driver, BxCAN_Context *ctx, CAN_TypeDef *inst);

#endif /* CAN_STM32_BXCAN_H */

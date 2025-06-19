#ifndef CAN_STM32_FDCAN_H
#define CAN_STM32_FDCAN_H

#include "can_interface.h"

#if defined(STM32H7xx)
#include "stm32h7xx_hal.h"
#include "stm32h7xx_hal_fdcan.h"
#else
#error "Unsupported STM32 family for FDCAN"
#endif

typedef struct {
    CAN_DriverContext_t base;
    FDCAN_HandleTypeDef hfdcan;
} FDCAN_Context;

void FDCAN_SetupDriver(ICANDriver *driver, FDCAN_Context *ctx, FDCAN_GlobalTypeDef *inst);

#endif /* CAN_STM32_FDCAN_H */

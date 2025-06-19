#ifndef CAN_STM32_BXCAN_H
#define CAN_STM32_BXCAN_H


#include "can_interface.h"

#if defined(STM32F1xx)
#  include "stm32f1xx_hal.h"
#  include "stm32f1xx_hal_can.h"
#elif defined(STM32F2xx)
#  include "stm32f2xx_hal.h"
#  include "stm32f2xx_hal_can.h"
#elif defined(STM32F4xx)
#  include "stm32f4xx_hal.h"
#  include "stm32f4xx_hal_can.h"
#else
#  error "Unsupported STM32 family for bxCAN"
#endif

typedef struct {
    CAN_DriverContext_t base;
    CAN_HandleTypeDef   hcan;
} BxCAN_Context;

void BxCAN_SetupDriver(ICANDriver *driver, BxCAN_Context *ctx, CAN_TypeDef *inst);

#endif /* CAN_STM32_BXCAN_H */

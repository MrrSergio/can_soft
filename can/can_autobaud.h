#ifndef CAN_AUTOBAUD_H
#define CAN_AUTOBAUD_H

#include <stdint.h>
#include "can_interface.h"

#ifdef __cplusplus
extern "C" {
#endif

CAN_Result_t CAN_Autobaud_Detect(ICANDriver *driver, const uint32_t *rates, uint8_t num);

#ifdef __cplusplus
}
#endif

#endif /* CAN_AUTOBAUD_H */

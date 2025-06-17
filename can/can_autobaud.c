#include "can_autobaud.h"
#include <stddef.h>

CAN_Result_t CAN_Autobaud_Detect(ICANDriver *driver, const uint32_t *rates, uint8_t num)
{
    if (!driver || !rates || !driver->set_mode || !driver->auto_baud_detect)
        return CAN_ERROR;
    /* Put driver in listen-only mode if supported */
    driver->set_mode(driver, CAN_MODE_SILENT);
    CAN_Result_t res = driver->auto_baud_detect(driver, rates, num);
    driver->set_mode(driver, CAN_MODE_NORMAL);
    return res;
}

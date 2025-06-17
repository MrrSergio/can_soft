#ifndef CAN_CONFIG_H
#define CAN_CONFIG_H

#include <stdint.h>

#define CAN_MAX_BITRATES 4

static const uint32_t default_bitrates[CAN_MAX_BITRATES] = {125000, 250000, 500000, 1000000};

#ifndef CAN_TX_QUEUE_LEN
#define CAN_TX_QUEUE_LEN 16
#endif

#ifndef CAN_RX_QUEUE_LEN
#define CAN_RX_QUEUE_LEN 16
#endif

#endif /* CAN_CONFIG_H */

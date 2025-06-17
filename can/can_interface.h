#ifndef CAN_INTERFACE_H
#define CAN_INTERFACE_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    CAN_MODE_NORMAL,
    CAN_MODE_SILENT,
    CAN_MODE_LOOPBACK,
    CAN_MODE_AUTOBAUD
} CAN_Mode_t;

typedef struct {
    uint32_t bitrate;
    uint32_t filter_id;
    uint32_t filter_mask;
    CAN_Mode_t mode;
} CAN_Config_t;

typedef enum {
    CAN_OK = 0,
    CAN_ERROR
} CAN_Result_t;

typedef struct ICANDriver ICANDriver;

typedef struct {
    uint32_t id;
    uint8_t dlc;
    uint8_t data[8];
    uint8_t extended;
} CAN_Message_t;

struct ICANDriver {
    CAN_Result_t (*init)(ICANDriver *driver, const CAN_Config_t *config);
    CAN_Result_t (*send)(ICANDriver *driver, const CAN_Message_t *msg, uint32_t timeout_ms);
    CAN_Result_t (*receive)(ICANDriver *driver, CAN_Message_t *msg);
    CAN_Result_t (*set_filter)(ICANDriver *driver, uint32_t id, uint32_t mask);
    CAN_Result_t (*set_mode)(ICANDriver *driver, CAN_Mode_t mode);
    uint32_t     (*get_error_state)(ICANDriver *driver);
    CAN_Result_t (*auto_baud_detect)(ICANDriver *driver, const uint32_t *bitrates, uint8_t num);
    void *ctx; /* driver specific context */
};

#ifdef __cplusplus
}
#endif

#endif /* CAN_INTERFACE_H */

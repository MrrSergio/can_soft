#ifndef CAN_MCP2515_H
#define CAN_MCP2515_H

#include <stdint.h>
#include <stddef.h>
#include <linux/spi/spidev.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Context for MCP2515 driver */
typedef struct {
    const char *spi_device;  /**< Path to SPI device (/dev/spidevX.Y) */
    int spi_fd;              /**< File descriptor for opened SPI device */
    uint8_t spi_mode;        /**< SPI mode configuration */
    uint32_t spi_speed_hz;   /**< SPI bus speed */
    uint8_t cs_pin;          /**< Chip select GPIO pin if managed manually */
    uint8_t int_pin;         /**< Interrupt GPIO pin */
} MCP2515_Context;

int mcp_init(MCP2515_Context *ctx);
int mcp_send(MCP2515_Context *ctx, const uint8_t *data, size_t len);
int mcp_receive(MCP2515_Context *ctx, uint8_t *data, size_t len);
int mcp_set_filter(MCP2515_Context *ctx, uint32_t mask, uint32_t filter);
int mcp_set_mode(MCP2515_Context *ctx, uint8_t mode);
int mcp_get_error(MCP2515_Context *ctx);
int mcp_autobaud(MCP2515_Context *ctx, const uint8_t *speeds, size_t count);

#ifdef __cplusplus
}
#endif

#endif /* CAN_MCP2515_H */

#include "can_mcp2515.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <errno.h>

static int spi_transfer(MCP2515_Context *ctx, const uint8_t *tx, uint8_t *rx, size_t len)
{
    struct spi_ioc_transfer tr = {
        .tx_buf = (unsigned long)tx,
        .rx_buf = (unsigned long)rx,
        .len = len,
        .delay_usecs = 0,
        .speed_hz = ctx->spi_speed_hz,
        .bits_per_word = 8,
    };
    return ioctl(ctx->spi_fd, SPI_IOC_MESSAGE(1), &tr);
}

int mcp_init(MCP2515_Context *ctx)
{
    if (!ctx || !ctx->spi_device)
        return -1;

    ctx->spi_fd = open(ctx->spi_device, O_RDWR);
    if (ctx->spi_fd < 0) {
        perror("open spi");
        return -1;
    }

    if (ioctl(ctx->spi_fd, SPI_IOC_WR_MODE, &ctx->spi_mode) < 0) {
        perror("SPI_IOC_WR_MODE");
        return -1;
    }

    if (ioctl(ctx->spi_fd, SPI_IOC_WR_MAX_SPEED_HZ, &ctx->spi_speed_hz) < 0) {
        perror("SPI_IOC_WR_MAX_SPEED_HZ");
        return -1;
    }

    /* Reset MCP2515 */
    uint8_t reset_cmd = 0xC0;
    if (spi_transfer(ctx, &reset_cmd, NULL, 1) < 0)
        return -1;
    usleep(10000); /* Wait for reset */

    return 0;
}

int mcp_send(MCP2515_Context *ctx, const uint8_t *data, size_t len)
{
    if (!ctx || ctx->spi_fd < 0 || !data)
        return -1;

    /* Example: Write to TX buffer 0 */
    uint8_t buffer[14];
    if (len > 8)
        len = 8;

    buffer[0] = 0x40; /* Load TX buffer 0 command */
    memcpy(&buffer[1], data, len);

    if (spi_transfer(ctx, buffer, NULL, len + 1) < 0)
        return -1;

    uint8_t send_cmd = 0x81; /* RTS for TXB0 */
    if (spi_transfer(ctx, &send_cmd, NULL, 1) < 0)
        return -1;

    return 0;
}

int mcp_receive(MCP2515_Context *ctx, uint8_t *data, size_t len)
{
    if (!ctx || ctx->spi_fd < 0 || !data)
        return -1;

    uint8_t cmd = 0x90; /* Read RX buffer 0 */
    uint8_t rx[13] = {0};

    if (len > 13)
        len = 13;

    if (spi_transfer(ctx, &cmd, rx, len + 1) < 0)
        return -1;

    memcpy(data, &rx[1], len);
    return 0;
}

int mcp_set_filter(MCP2515_Context *ctx, uint32_t mask, uint32_t filter)
{
    /* This is a simplified implementation writing to RXF0 and RXM0 */
    if (!ctx || ctx->spi_fd < 0)
        return -1;

    uint8_t buf[5];

    buf[0] = 0x02; /* Write instruction */

    /* Mask register address for RXM0 */
    buf[1] = 0x20; /* RXM0SIDH */
    buf[2] = (mask >> 3) & 0xFF;
    buf[3] = ((mask & 0x07) << 5);
    buf[4] = 0; /* EID8 */
    if (spi_transfer(ctx, buf, NULL, 5) < 0)
        return -1;

    buf[1] = 0x00; /* RXF0SIDH */
    buf[2] = (filter >> 3) & 0xFF;
    buf[3] = ((filter & 0x07) << 5);
    buf[4] = 0; /* EID8 */
    if (spi_transfer(ctx, buf, NULL, 5) < 0)
        return -1;

    return 0;
}

int mcp_set_mode(MCP2515_Context *ctx, uint8_t mode)
{
    if (!ctx || ctx->spi_fd < 0)
        return -1;

    uint8_t buf[3];
    buf[0] = 0x02; /* Write */
    buf[1] = 0x0F; /* CANCTRL */
    buf[2] = mode;
    if (spi_transfer(ctx, buf, NULL, 3) < 0)
        return -1;
    return 0;
}

int mcp_get_error(MCP2515_Context *ctx)
{
    if (!ctx || ctx->spi_fd < 0)
        return -1;

    uint8_t cmd[2] = {0x03, 0x2D}; /* Read, EFLG */
    uint8_t res[2];
    if (spi_transfer(ctx, cmd, res, 2) < 0)
        return -1;
    return res[1];
}

int mcp_autobaud(MCP2515_Context *ctx, const uint8_t *speeds, size_t count)
{
    if (!ctx || ctx->spi_fd < 0 || !speeds)
        return -1;

    for (size_t i = 0; i < count; ++i) {
        if (mcp_set_mode(ctx, 0x80 /* Configuration mode */) < 0)
            continue;

        uint8_t cfg_cmd[4] = {0x02, 0x2A, speeds[i], 0x90};
        if (spi_transfer(ctx, cfg_cmd, NULL, 4) < 0)
            continue;

        if (mcp_set_mode(ctx, 0x00 /* Normal mode */) == 0)
            return 0;
    }
    return -1;
}

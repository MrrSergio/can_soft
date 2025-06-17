# can_soft

This repository provides a simple driver for the MCP2515 CAN controller using the Linux spidev interface.

```
make
```

Example usage requires specifying the SPI device and parameters in `MCP2515_Context` before calling `mcp_init`.

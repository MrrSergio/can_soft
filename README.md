# CAN Soft Module

This repository contains a portable CAN bus management module written in pure C. The module allows the use of multiple CAN controllers simultaneously via a common `ICANDriver` interface. Example drivers for MCP2515 and STM32 controllers are provided. The STM32 support now covers both the legacy **bxCAN** peripheral (used on F1/F2/F4 families) and the newer **FDCAN** peripheral found on the H7 series. All drivers rely on the corresponding STM32 HAL libraries.

## Directory structure

```
can/
├── can_autobaud.c/h    - auto baudrate detection helper
├── can_config.h        - default configuration values
├── can_interface.h     - abstract ICANDriver definition
├── can_manager.c/h     - manager for multiple CAN instances
├── can_mcp2515.c/h     - driver for MCP2515 controller
├── can_stm32_bxcan.c/h - STM32 bxCAN driver with helper to create CAN1/CAN2/CAN3 instances
├── can_stm32_fdcan.c/h - STM32 FDCAN driver for H7 series
└── can_test.c          - usage example
```

## Features

- Multiple CAN interfaces selectable at runtime
- Built-in transmit and receive queues with event callbacks
- Automatic bitrate detection helper
- Simple API for sending messages and polling receive buffers
- Direct access to driver functions for filters, modes and error queries

## Building example

`can_test.c` demonstrates adding four interfaces (MCP2515, two bxCAN instances for CAN1/CAN2 and one FDCAN interface). The example enables autobaud on the MCP2515 and FDCAN drivers, configures filters and loopback mode, sends messages and polls for reception.

```
cc can/*.c -o can_test
./can_test
```

To build the STM32 versions of the drivers make sure the appropriate HAL sources for your family (F1/F2/F4 for bxCAN or H7 for FDCAN) are in your include path and link the HAL libraries when compiling your firmware.

# CAN Soft Module

This repository contains a portable CAN bus management module written in pure C. The module allows the use of multiple CAN controllers simultaneously via a common `ICANDriver` interface. Example drivers for MCP2515 and STM32 bxCAN are provided as stubs.

## Directory structure

```
can/
├── can_autobaud.c/h    - auto baudrate detection helper
├── can_config.h        - default configuration values
├── can_interface.h     - abstract ICANDriver definition
├── can_manager.c/h     - manager for multiple CAN instances
├── can_mcp2515.c/h     - stub driver for MCP2515 controller
├── can_stm32_bxcan.c/h - stub driver for STM32 bxCAN controller
└── can_test.c          - usage example
```

## Features

- Multiple CAN interfaces selectable at runtime
- Built-in transmit and receive queues with event callbacks
- Automatic bitrate detection helper
- Simple API for sending messages and polling receive buffers
- Direct access to driver functions for filters, modes and error queries

## Building example

`can_test.c` demonstrates adding two interfaces, enabling autobaud, configuring filters and loopback mode, sending messages and polling for reception.

```
cc can/*.c -o can_test
./can_test
```

# can_soft

## CAN Manager API

This repository contains minimal examples of a CAN manager with an event trigger API.
Drivers should invoke `CAN_Manager_TriggerEvent` when a transmission completes,
a message is received or an error occurs.

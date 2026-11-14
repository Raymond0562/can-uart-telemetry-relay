# Real-Time CAN-to-UART Telemetry Relay

An interrupt-driven relay in embedded C that forwards every CAN frame it receives
out over UART, without ever blocking inside the interrupt.

## How it works

- `RxCan()` runs in the CAN receive interrupt. It only copies the frame into a
  32-slot circular buffer and advances the write index, so the ISR stays short.
- `Iter()` runs in the main loop. It drains the buffer, builds a UART frame for
  each message, and sends it.
- The read and write indexes are `volatile` because they are shared between the
  ISR and the main loop.
- If the buffer is full, the new frame is read and discarded so the bus keeps
  moving and existing data is never overwritten.
- An RGB LED shows activity: red while idle, green while frames are being sent.

## UART frame format

Each CAN message becomes a 14-byte UART frame:

| Bytes | Contents |
|---|---|
| 0-1 | Header `0xF5 0xAE` |
| 2-5 | CAN ID, converted to big-endian |
| 6-13 | The 8 CAN data bytes |

## Files

| File | Role |
|---|---|
| `formulaE_Design_project.c` | The relay: `Init()`, `Iter()`, and `RxCan()` |
| `HAL.h` | Hardware abstraction layer the relay is written against (UART send, LED, CAN read) |

The HAL functions are declared but not implemented here; they are provided by the
target board.

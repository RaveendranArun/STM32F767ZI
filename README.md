# STM32F7 DMA playground

Bare-metal experiments with the STM32F767 DMA controller (RM0410), on the
NUCLEO-F767ZI (Nucleo-144) board. No HAL — registers are accessed through the
CMSIS device header.

The project started on the STM32F4DISCOVERY (STM32F407); that code is kept in
`app/backup/` and `drivers/backup/` for reference.

## Board quick reference

| Item | NUCLEO-F767ZI |
|------|---------------|
| MCU | STM32F767ZI, Cortex-M7, 2 MB flash, 512 KB RAM |
| Virtual COM port | USART3 — PD8 (TX), PD9 (RX), AF7 |
| User button B1 | PC13 (high when pressed) → `EXTI15_10_IRQHandler` |
| LEDs | LD1 green PB0, LD2 blue PB7, LD3 red PB14 |
| Default clock | HSI 16 MHz (`SystemInit()` is not called) |

## Layout

```
app/
  inc/            application headers (main.h)
  src/            application code (main.c)
  backup/         previous STM32F407 application (not built)
drivers/
  inc/            peripheral driver headers (dma.h, ...)
  src/            peripheral driver sources — every .c here is built automatically
  backup/         previous STM32F407 interrupt handlers (not built)
platform/
  startup/        vector table (STM32F767, 110 IRQs) and Reset_Handler
  linker/         linker script (2 MB flash, 512 KB RAM)
  syscalls/       newlib syscall stubs (normal build only)
ThirdParty/CMSIS/ CMSIS core and STM32F7xx (and older STM32F4xx) device headers
tools/openocd/    OpenOCD board configuration
tools/renode/     Renode emulation script
docs/             notes and guides
```

## Build

Requires `arm-none-eabi-gcc` and OpenOCD. Renode is optional (emulation only).

| Command       | Result |
|---------------|--------|
| `make`        | `image/final.{elf,bin,hex}` (newlib-nano) |
| `make semi`   | `image/final_sh.{elf,bin,hex}` with semihosting `printf` |
| `make flash`  | Programs, verifies and resets the board (stop `make load` first) |
| `make load`   | Starts OpenOCD for the Nucleo board (GDB on port 3333) |
| `make renode` | Runs the firmware in the Renode emulator (GDB on port 3333) |
| `make clean`  | Removes `build/` and `image/` |

## Emulation

The firmware can run without hardware in Renode's NUCLEO-F767ZI model, which
covers USART3, the LEDs and the user button. See
[docs/renode_nucleo_f767zi.md](docs/renode_nucleo_f767zi.md) for installation,
monitor commands, GDB debugging and limitations.

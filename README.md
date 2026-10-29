# STM32F767ZI bare-metal drivers

Register-level peripheral drivers for the STM32F767ZI (RM0410), on the
NUCLEO-F767ZI (Nucleo-144) board. No HAL — registers are accessed through the
CMSIS device header.

The demo application sends `Hello World` over the ST-LINK virtual COM port
(USART3) using DMA (memory → peripheral), triggered by the user button.

## Board quick reference

| Item | NUCLEO-F767ZI |
|------|---------------|
| MCU | STM32F767ZI, Cortex-M7, 2 MB flash, 512 KB RAM |
| Virtual COM port | USART3 — PD8 (TX), PD9 (RX), AF7, 115200 8N1 |
| USART3_TX DMA | DMA1 Stream 3, Channel 4 |
| User button B1 | PC13 (high when pressed) → `EXTI15_10_IRQHandler` |
| LEDs | LD1 green PB0, LD2 blue PB7, LD3 red PB14 |
| Default clock | HSI 16 MHz (`SystemInit()` is not called) |

## Layout

The code is split into three layers. Each layer only uses the ones below it:

```
app      what the firmware does      (main.c, interrupt handlers, DMA callbacks)
  ↓
bsp      which pins / instances      (board.h: PC13, USART3, DMA1 Stream 3, ...)
  ↓
drivers  how a peripheral works      (gpio, exti, usart, dma — no board knowledge)
  ↓
CMSIS    register definitions
```

Porting to another board means changing `bsp/` only.

```
app/
  inc/            application headers (main.h)
  src/            main.c — demo setup, message buffer, DMA callbacks
bsp/
  inc/            board.h — Nucleo-F767ZI pin and peripheral assignments
  src/            board.c — board_button_init(), board_vcp_init()
drivers/
  inc/            gpio.h, exti.h, usart.h, dma.h
  src/            driver sources + stm32f7xx_it.c (interrupt handlers)
platform/
  startup/        vector table (STM32F767, 110 IRQs) and Reset_Handler
  linker/         linker script (2 MB flash, 512 KB RAM)
  syscalls/       newlib syscall stubs (normal build only)
ThirdParty/CMSIS/ CMSIS core and STM32F7xx (and older STM32F4xx) device headers
tools/openocd/    OpenOCD board configuration
tools/renode/     Renode emulation script
docs/             notes and guides
```

Every `.c` file in `app/src`, `bsp/src` and `drivers/src` is built
automatically — adding a file needs no Makefile change.

## Drivers

All drivers take the peripheral instance as a parameter (`GPIOC`, `USART3`,
`DMA1_Stream3`, ...), so the same code works for any port, USART or stream.

| Driver | Functions |
|--------|-----------|
| `gpio`  | `gpio_clock_enable`, `gpio_set_mode`, `gpio_set_pull`, `gpio_set_af` |
| `exti`  | `exti_config` (port, pin, rising / falling / both), `exti_is_pending`, `exti_clear_pending` |
| `usart` | `usart_clock_enable`, `usart_init` (TX, 8N1), `usart_send` (blocking), `usart_dma_tx_enable` / `_disable` |
| `dma`   | `dma_stream_config` (`dma_config_t`: channel, direction, width, increment, FIFO, addresses, length), `dma_stream_enable_irq`, `dma_stream_enable` / `_disable`, `dma_stream_set_length`, `dma_get_flag` / `dma_clear_flag` |

The DMA driver works out the controller (DMA1 / DMA2) and stream number from
the stream pointer, so callers never deal with `LISR` / `HISR` bit positions.

## How the demo works

1. `board_button_init()` — PC13 as input, EXTI line 13 on the falling edge.
2. `board_vcp_init()` — PD8/PD9 in AF7, USART3 at 115200 baud, TX enabled.
3. `vcp_tx_dma_init()` — DMA1 Stream 3 set up memory → `USART3->TDR` and
   enabled; it waits for a DMA request from the USART.
4. Button press → `EXTI15_10_IRQHandler` sets `USART3 CR3.DMAT`, so the
   USART starts requesting bytes and the message is sent.
5. Transfer complete → `FT_Complete_callback()` turns `DMAT` off, reloads the
   length and re-arms the stream for the next press.

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

To see the output on the board, open a serial terminal on the ST-LINK virtual
COM port at 115200 8N1 and press the blue user button.

## Emulation

The firmware can run without hardware in Renode's NUCLEO-F767ZI model, which
covers USART3, the LEDs and the user button. In the Renode monitor, type
`click` to press the button; USART3 output appears in the analyzer and on TCP
port 3456 (`nc localhost 3456`). See
[docs/renode_nucleo_f767zi.md](docs/renode_nucleo_f767zi.md) for installation,
monitor commands, GDB debugging and limitations.

## Roadmap

- DMA receive (peripheral → memory): USART3_RX on DMA1 Stream 1, Channel 4,
  circular buffer + USART idle-line detection
- Generic DMA interrupt handler with per-stream weak callbacks
- Move interrupt handlers into `app/`

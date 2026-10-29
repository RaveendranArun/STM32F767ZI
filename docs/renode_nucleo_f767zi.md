# Renode emulation: NUCLEO-F767ZI

How to run and debug this firmware without hardware, using
[Renode](https://renode.io)'s model of the Nucleo-144 STM32F767ZI board.

Last verified: 2026-10-10 with Renode 1.17.0 on macOS (Apple Silicon).

## Why Renode, not QEMU

QEMU has no STM32F7 machine. Its closest STM32 board (`netduinoplus2`, an
STM32F405) uses the F4 USART registers (`SR`/`DR`) and does not model GPIO
or RCC. Renode ships a board file for this exact board,
`platforms/boards/nucleo_f767zi.repl`.

## What the model provides

| Feature | Model | Notes |
|---|---|---|
| CPU | Cortex-M7 | SysTick at 216 MHz, FPU interrupt 81 |
| Flash | 2 MB @ `0x08000000` | ELF is loaded here |
| RAM | SRAM @ `0x20000000` | Region is larger than the real 512 KB, so overflowing RAM won't fault in Renode |
| USART1/2/3/6 | `STM32F7_USART` | F7 registers (`ISR`, `TDR`, `RDR`). USART3 (PD8/PD9) is the board's virtual COM port |
| UART4/5/7/8 | `STM32_UART` | Older SR/DR-style model, not F7 |
| GPIO A–K, EXTI, SYSCFG | Yes | EXTI lines 10–15 go to IRQ 40 (`EXTI15_10`) |
| LEDs | `GreenLED` PB0, `BlueLED` PB7, `RedLED` PB14 | |
| User button B1 | `UserButton1` on PC13 | Pressing it gives a **rising edge**, as on the real board |
| Also modelled | CAN1–3, ADC1–3, CRC, Ethernet + PHY | Not tried yet |

## Installation (one-time)

Homebrew has no Renode package. Install it from the GitHub release:

```sh
curl -LO https://github.com/renode/renode/releases/download/v1.17.0/renode-1.17.0.osx-arm64-portable.dmg
hdiutil attach renode-1.17.0.osx-arm64-portable.dmg
cp -R /Volumes/Renode*/Renode.app /Applications/
hdiutil detach /Volumes/Renode*
xattr -dr com.apple.quarantine /Applications/Renode.app   # avoids the Gatekeeper block
```

Check that it runs:

```sh
/Applications/Renode.app/Contents/MacOS/renode --version
```

For a newer version, take the `osx-arm64-portable.dmg` asset from
<https://github.com/renode/renode/releases>.

The first run downloads the STM32F7 SVD register file from `dl.antmicro.com`,
so that run needs internet access.

## Project files

| File | Purpose |
|---|---|
| [tools/renode/nucleo_f767zi.resc](../tools/renode/nucleo_f767zi.resc) | Renode script: creates the board, loads `build/final.elf`, logs USART3 output, opens a USART3 serial terminal on TCP `:3456`, starts a GDB server on `:3333` |
| [Makefile](../Makefile) `renode` target | Builds the firmware, then runs the script headless in the terminal |
| `RENODE` variable in the Makefile | Path to the Renode binary (`/Applications/Renode.app/Contents/MacOS/renode`) |

## Running

```sh
make renode
```

What happens:

- The firmware is built (same as `make`).
- Renode starts in console mode and loads `build/final.elf`. You should see
  `Setting initial values: PC = ..., SP = 0x20080000`.
- USART3 output appears as log lines, for example
  `[INFO] usart3: [...] Hello World`.
- The terminal shows the `(nucleo_f767zi)` monitor prompt, where you type
  Renode commands.
- Type `quit` to exit.

### Serial terminal (USART3 over TCP)

The script also exposes USART3 as a raw serial stream on TCP port **3456**,
like CoolTerm or `screen` on the real board's virtual COM port. In a second
terminal:

```sh
nc localhost 3456
```

- Characters the firmware sends appear here as plain text, without the log
  prefix.
- What you type should be sent to USART3's RX line, for testing reception
  (not yet tested).
- Output sent before `nc` connects is not replayed. To catch the first
  messages, connect, then reset the board from the monitor with
  `machine Reset`.
- Exit `nc` with `Ctrl-C`; the emulation keeps running.

### Useful monitor commands

| Command | Effect |
|---|---|
| `gpioPortC.UserButton1 PressAndRelease` | Click the user button |
| `gpioPortC.UserButton1 Press` / `Release` | Hold or release the button |
| `gpioPortB.GreenLED State` | `True` if the LED is on (same for `BlueLED`, `RedLED`) |
| `sysbus ReadDoubleWord 0x40004800` | Read a peripheral register (`0x40004800` is USART3_CR1) |
| `cpu PC` | Current program counter |
| `pause` / `start` | Stop or resume the emulation |
| `machine Reset` | Reset the board; the ELF is reloaded by the script's `reset` macro |
| `sysbus LogPeripheralAccess usart3` | Log every register access to a peripheral |
| `quit` | Exit Renode |

### Running a different ELF

The script uses `$bin ?= ...`, so you can set `$bin` before loading it:

```sh
/Applications/Renode.app/Contents/MacOS/renode --disable-gui --console \
  -e '$bin=@/absolute/path/to/app.elf; i @tools/renode/nucleo_f767zi.resc; start'
```

### GUI mode

Without `--disable-gui`, Renode opens a monitor window and a separate
terminal window for USART3, like a serial terminal (not tried in this project):

```sh
/Applications/Renode.app/Contents/MacOS/renode tools/renode/nucleo_f767zi.resc
```

Then type `start` in the monitor window.

## Debugging with GDB

The script starts a GDB server on port **3333**, the same port OpenOCD uses.
**Stop OpenOCD (`make load`) before running Renode**, or the port will be
busy.

### Command line (tested)

Start Renode **without** `start`, so the CPU waits at the reset vector:

```sh
/Applications/Renode.app/Contents/MacOS/renode --disable-gui --console \
  -e 'i @tools/renode/nucleo_f767zi.resc'
```

In a second terminal:

```sh
arm-none-eabi-gdb build/final.elf
(gdb) target remote :3333
(gdb) break main
(gdb) monitor start                  # Renode commands work via "monitor"
(gdb) continue
(gdb) monitor gpioPortC.UserButton1 PressAndRelease
```

Breakpoints in interrupt handlers work. A breakpoint in
`EXTI15_10_IRQHandler` stopped when the button was pressed from GDB.

GDB may print `warning: Invalid state, unable to determine sp alias,
assuming msp.` This is harmless.

### VS Code (cortex-debug), not yet tested

The existing **"Attach to running OpenOCD (make load)"** configuration sends
`monitor reset halt`, which is an OpenOCD command, so it is not expected to
work with Renode. A separate configuration that attaches without resetting
should work:

```json
{
    "name": "Attach to Renode (make renode)",
    "type": "cortex-debug",
    "request": "attach",
    "servertype": "external",
    "gdbTarget": "127.0.0.1:3333",
    "cwd": "${workspaceFolder}",
    "executable": "${workspaceFolder}/build/final.elf",
    "armToolchainPath": "/opt/homebrew/bin"
}
```

Run `make renode` first, then start this configuration.

## Limitations

- **Clock and baud rate:** Renode doesn't check `BRR`, the PLL or flash wait
  states. A wrong baud rate still prints correctly in Renode.
- **RCC clock enables:** peripherals may work even if their `RCC->xxxENR`
  bit is never set. Forgetting a clock enable shows up only on hardware.
- **RAM size:** the SRAM region is larger than 512 KB, so stack or heap
  overflow past `0x20080000` won't fault in Renode.
- **Timing:** time is virtual. Busy-wait delays won't match real time.
- **Not modelled the same:** DMA, caches and the D-cache/DMA coherency
  issues won't behave exactly as on hardware.

Always confirm on the real board with `make flash`.

## Troubleshooting

| Symptom | Cause / fix |
|---|---|
| "Renode.app is damaged / can't be opened" | Gatekeeper quarantine: `xattr -dr com.apple.quarantine /Applications/Renode.app` |
| `GDB server ... port 3333` error | OpenOCD (or another Renode) is still running: `pkill openocd`, or `quit` the other Renode. The script stops at this error, so the firmware isn't loaded |
| Port 3456 error, or `nc` shows nothing | Another Renode still holds the terminal port; `lsof -iTCP:3456` shows which process |
| No USART3 output | Check `USART3->CR1` has `TE` and `UE` set, and the code waits for `USART_ISR_TXE` (F7 registers, not F4 `SR`/`DR`) |
| CPU stuck in `Default_Handler` | An interrupt is enabled without a handler, or the handler name doesn't match the vector table in `platform/startup/stm32_startup.c` |
| Hangs at first run | It is downloading the SVD file; needs internet once |

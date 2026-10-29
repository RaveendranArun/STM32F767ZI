# DMA notes: STM32F767ZI

Notes on the STM32F7 DMA controller, and how this project uses it to send
`"Hello World\r\n"` over USART3 when the user button is pressed.

Status: draft. Reference: RM0410 (STM32F76xxx/77xxx reference manual), DMA
chapter, and AN4031 (Using the STM32F2/F4/F7 DMA controller).

## Overview

The F767 has two general-purpose DMA controllers, **DMA1** and **DMA2**.
Each one has:

| Item | Count / size |
|---|---|
| Streams | 8 (Stream 0–7), each one an independent transfer engine |
| Channels per stream | Up to 16 request inputs (`CHSEL` is 4 bits on F76x/F77x; F4 and F74x have 3 bits, 8 channels) |
| FIFO per stream | 4 words (16 bytes) |
| Max items per transfer | 65535 (`NDTR` is 16 bits) |
| Ports | A memory port and a peripheral port, both AHB masters |

**DMA1 vs DMA2**

- DMA1's peripheral port is connected only to the APB1 peripherals, so
  **DMA1 cannot do memory-to-memory transfers**. Only DMA2 can.
- USART3 is on APB1, so its requests go to DMA1.

## Streams and channels

A **stream** is the hardware that moves the data. A **channel** is a
request line from a peripheral. Each stream has a multiplexer, and `CHSEL`
picks which of its channels (peripherals) is allowed to trigger it. Only one
channel can be active on a stream at a time.

The "DMA1 request mapping" table in RM0410 lists which peripheral is on
which stream/channel pair. For USART3:

| Request | Stream | Channel |
|---|---|---|
| USART3_TX | DMA1 Stream 3 | Channel 4 (**used here**) |
| USART3_TX | DMA1 Stream 4 | Channel 7 (alternative) |
| USART3_RX | DMA1 Stream 1 | Channel 4 |

A stream's registers are named `DMA1_Stream3->CR`, `->NDTR`, `->PAR`,
`->M0AR`, `->M1AR`, `->FCR` in CMSIS.

## Arbiter

When several streams of the **same** controller have requests pending, the
arbiter picks one in two steps:

1. **Software priority:** the `PL` bits in `DMA_SxCR` (17:16):
   `00` low, `01` medium, `10` high, `11` very high. The highest wins.
2. **Hardware priority:** if two streams have the same `PL`, the **lower
   stream number** wins (Stream 0 beats Stream 1, and so on).

DMA1 and DMA2 have separate arbiters. When both controllers, or a
controller and the CPU, want the same bus, the bus matrix decides.

This project uses only one stream, so `PL` is left at reset (low).

## Types of transfer

### Direction (`DIR`, bits 7:6)

| `DIR` | Transfer | Source | Destination |
|---|---|---|---|
| `00` | Peripheral-to-memory (P2M) | `PAR` | `M0AR` |
| `01` | Memory-to-peripheral (M2P) | `M0AR` | `PAR` (**used here**) |
| `10` | Memory-to-memory (M2M) | `PAR` | `M0AR` (DMA2 only, FIFO mode only) |

### Modes

| Mode | Bits | Behaviour |
|---|---|---|
| Normal | `CIRC = 0` | Stops when `NDTR` reaches 0. Hardware clears `EN`. To send again, reload `NDTR` and set `EN` (**used here**) |
| Circular | `CIRC = 1` | `NDTR` and the addresses reload automatically and the transfer repeats. Used for continuous streams such as ADC sampling or UART receive |
| Double buffer | `DBM = 1` | Alternates between `M0AR` and `M1AR`. `CT` shows which buffer is in use, so software can fill one while DMA uses the other. Implies circular |

### Flow controller (`PFCTRL`, bit 5)

- `0`: the DMA is the flow controller and the length comes from `NDTR`
  (**used here**).
- `1`: the peripheral decides when the transfer ends. Only SDMMC supports this.

### Data width and address increment

- `PSIZE` (12:11) and `MSIZE` (14:13): `00` byte, `01` half-word, `10` word.
  Here both are byte, because `USART3->TDR` takes one character at a time.
- `MINC` (bit 10) / `PINC` (bit 9): increment the memory or peripheral
  address after each item. For M2P to a USART, set `MINC` and leave `PINC`
  clear, so `TDR` stays fixed while the pointer walks through the string.
- `NDTR` counts **items of `PSIZE`**, not bytes.

### Direct mode vs FIFO mode

| | Direct mode (`DMDIS = 0`) | FIFO mode (`DMDIS = 1`) |
|---|---|---|
| Buffering | None, each request moves one item straight through | Data goes through the 4-word FIFO |
| Widths | `MSIZE` is forced equal to `PSIZE` | `PSIZE` and `MSIZE` can differ (packing/unpacking) |
| Burst | Not allowed | Allowed |
| M2M | Not allowed | Required |
| Threshold | n/a | `FTH` (1:0): 1/4, 1/2, 3/4 or full FIFO |

This project uses FIFO mode with a full threshold. For a byte-for-byte
transfer to a USART, direct mode would work equally well.

### Single vs burst transfer

- `MBURST` (24:23) / `PBURST` (22:21): `00` single, `01` INC4, `10` INC8,
  `11` INC16.
- Burst needs FIFO mode, and the burst size must fit the FIFO threshold.
- Single transfer is used here.

## Interrupts and status flags

Each stream has five events. The flags are in `DMA_LISR` (Streams 0–3) or
`DMA_HISR` (Streams 4–7), and are cleared by writing 1 to the same bit in
`DMA_LIFCR` / `DMA_HIFCR`.

| Event | Enable bit | Stream 3 flag (`LISR`) |
|---|---|---|
| FIFO error (over/underrun) | `FCR.FEIE` (bit 7) | `FEIF3` bit 22 |
| Direct mode error | `CR.DMEIE` (bit 1) | `DMEIF3` bit 24 |
| Transfer error (bus error) | `CR.TEIE` (bit 2) | `TEIF3` bit 25 |
| Half transfer | `CR.HTIE` (bit 3) | `HTIF3` bit 26 |
| Transfer complete | `CR.TCIE` (bit 4) | `TCIF3` bit 27 |

The flag bit positions for each stream in `LISR`/`HISR`: Stream 0 uses bits
0–5, Stream 1 bits 6–11, Stream 2 bits 16–21, and Stream 3 bits 22–27.
Streams 4–7 repeat the same pattern in `HISR`.

All five events share one NVIC line per stream (`DMA1_Stream3_IRQn`), so the
handler must check the flags to see what happened, and must clear them or
the interrupt fires again forever.

On a transfer error, hardware clears `EN` and stops the stream.

## Enabling sequence

From RM0410, "Stream configuration procedure", with the USART side from
"Transmission using DMA".

1. **Enable the DMA clock:** `RCC->AHB1ENR |= RCC_AHB1ENR_DMA1EN`. Without it,
   every write to `DMA1` registers is ignored.
2. **Make sure the stream is off:** clear `EN`, then read it back until it
   is 0. Configuration registers can only be written while `EN = 0`.
3. **Clear the stream's old flags** in `LIFCR`/`HIFCR`. `EN` will not start a
   new transfer while flags from the last one are set.
4. **Peripheral address:** `PAR = (uint32_t)&USART3->TDR`.
5. **Memory address:** `M0AR = (uint32_t)msg` (and `M1AR` for double buffer).
6. **Item count:** `NDTR`.
7. **Channel:** `CHSEL`.
8. **Flow controller:** `PFCTRL` (leave 0 unless the peripheral controls it).
9. **Priority:** `PL`.
10. **FIFO:** `FCR` (direct or FIFO mode, threshold, `FEIE`).
11. **Rest of `CR`:** `DIR`, `MINC`/`PINC`, `PSIZE`/`MSIZE`, burst,
    `CIRC`/`DBM`, interrupt enables.
12. **NVIC:** `NVIC_EnableIRQ(DMA1_Stream3_IRQn)` if interrupts are used.
13. **Enable the stream:** set `EN`.
14. **Let the peripheral make requests:** `USART3->CR3 |= USART_CR3_DMAT`.
    USART3's `TXE` is already 1 when idle, so the first request comes
    immediately.

## How this project uses it

Files: [app/src/main.c](../app/src/main.c),
[drivers/src/stm32f7xx_it.c](../drivers/src/stm32f7xx_it.c).

### Configuration

| Setting | Value |
|---|---|
| Controller / stream / channel | DMA1 / Stream 3 / Channel 4 |
| Direction | M2P, `msg` → `USART3->TDR` |
| `NDTR` | `sizeof(msg) - 1` (13, the trailing `'\0'` is not sent) |
| Widths | Byte / byte |
| Increment | `MINC = 1`, `PINC = 0` |
| Mode | Normal (not circular), DMA flow control |
| FIFO | FIFO mode, full threshold, single transfer |
| Priority | Low (reset value) |
| Interrupts | HT, TC, TE, DME, FE |

### Startup order in `main()`

```c
button_init();                    // PC13 -> EXTI13, falling edge
usart3_init();                    // PD8/PD9 AF7, 115200 baud (16 MHz HSI)
dma1_init();                      // clock + stream config
dma1_interrupt_configuration();   // HT/TC/TE/DME/FE + NVIC
enable_dma1_stream3();            // EN = 1, stream waits for requests
```

The stream is armed but idle, because USART3 isn't sending DMA requests yet.

### Run-time flow

1. Button press → `EXTI15_10_IRQHandler` sets `DMAT` in `USART3->CR3`.
2. USART3 asks for data each time `TXE` is set. DMA1 Stream 3 copies one
   byte from `msg` to `TDR` for each request.
3. After half the bytes → `HTIF3` → `HT_Complete_callback()` (empty).
4. After the last byte → `TCIF3`, and hardware clears `EN`.
5. `FT_Complete_callback()` re-arms for the next press:
   - reloads `NDTR`
   - clears `DMAT`, so the USART stops requesting
   - sets `EN` again

   The stream is then waiting, as at startup.
6. Errors (TE, DME, FE) go to callbacks that stop in `while (1)` so they can
   be inspected in the debugger.

## Things to watch

- **Configure only with `EN = 0`.** Writes while the stream is running are
  ignored or cause errors.
- **Clear all flags before setting `EN`.** The handler clears only the flag
  it handled, so a stale flag (for example HT) can still be set when the
  stream is re-enabled.
- **`LIFCR`/`HIFCR` are write-1-to-clear.** A plain `=` is the usual form.
  `|=` works only because these registers read as 0.
- **DMA "transfer complete" ≠ USART "transmission complete".** When `TCIF3`
  sets, the last byte is still in the USART shift register. Wait for
  `USART_ISR_TC` before disabling the USART or entering low power.
- **Button edge.** B1 on the Nucleo-144 is active-high, so a falling-edge
  trigger sends on release, not press. The button is not debounced.
- **D-cache (F7 only).** If D-cache is enabled and the buffer is in SRAM,
  clean the cache before an M2P transfer (`SCB_CleanDCache_by_Addr`) and
  invalidate it after a P2M transfer, or the DMA and CPU see different data.
  Not an issue today: the cache is off and `msg` is `const` in flash. See
  AN4839.

## Register quick reference

`DMA_SxCR`

| Bits | Field | Meaning |
|---|---|---|
| 0 | `EN` | Stream enable |
| 1 | `DMEIE` | Direct mode error interrupt enable |
| 2 | `TEIE` | Transfer error interrupt enable |
| 3 | `HTIE` | Half transfer interrupt enable |
| 4 | `TCIE` | Transfer complete interrupt enable |
| 5 | `PFCTRL` | Peripheral flow controller |
| 7:6 | `DIR` | Direction |
| 8 | `CIRC` | Circular mode |
| 9 | `PINC` | Peripheral increment |
| 10 | `MINC` | Memory increment |
| 12:11 | `PSIZE` | Peripheral data size |
| 14:13 | `MSIZE` | Memory data size |
| 15 | `PINCOS` | Peripheral increment offset size |
| 17:16 | `PL` | Priority level |
| 18 | `DBM` | Double buffer mode |
| 19 | `CT` | Current target (`M0AR`/`M1AR`) |
| 22:21 | `PBURST` | Peripheral burst |
| 24:23 | `MBURST` | Memory burst |
| 28:25 | `CHSEL` | Channel select |

`DMA_SxFCR`

| Bits | Field | Meaning |
|---|---|---|
| 1:0 | `FTH` | FIFO threshold |
| 2 | `DMDIS` | 1 = FIFO mode, 0 = direct mode |
| 5:3 | `FS` | FIFO status (read-only) |
| 7 | `FEIE` | FIFO error interrupt enable |

#ifndef __DMA_H__
#define __DMA_H__

#include <stdint.h>
#include "stm32f767xx.h"

/* Transfer direction (CR.DIR field values) */
typedef enum
{
    DMA_DIR_P2M = 0U,
    DMA_DIR_M2P = 1U,
    DMA_DIR_M2M = 2U
} dma_dir_t;

/* Data width, used for both peripheral and memory side (CR.PSIZE / CR.MSIZE) */
typedef enum
{
    DMA_WIDTH_BYTE = 0U,
    DMA_WIDTH_HALF = 1U,
    DMA_WIDTH_WORD = 2U
} dma_width_t;

/* FIFO mode: direct mode, or FIFO with a threshold (FCR.FTH field values + 1) */
typedef enum
{
    DMA_FIFO_DIRECT  = 0U,
    DMA_FIFO_1_4     = 1U,
    DMA_FIFO_HALF    = 2U,
    DMA_FIFO_3_4     = 3U,
    DMA_FIFO_FULL    = 4U
} dma_fifo_t;

typedef struct
{
    uint8_t     channel;        // 0..7, request mapping from the reference manual
    dma_dir_t   dir;
    dma_width_t width;
    uint8_t     mem_inc;        // 1 = increment memory address after each item
    dma_fifo_t  fifo;
    uint32_t    periph_addr;
    uint32_t    mem_addr;
    uint16_t    len;            // number of data items
} dma_config_t;

/* Stream event flags. Same bit layout as the stream's field in LISR/HISR,
   so they can be OR'ed together for the interrupt enable mask too */
typedef enum
{
    DMA_FLAG_FE  = (1U << 0),   // FIFO overrun/underrun error
    DMA_FLAG_DME = (1U << 2),   // Direct mode error
    DMA_FLAG_TE  = (1U << 3),   // Transfer error
    DMA_FLAG_HT  = (1U << 4),   // Half transfer
    DMA_FLAG_TC  = (1U << 5)    // Transfer complete
} dma_flag_t;

void dma_clock_enable(DMA_Stream_TypeDef *stream);
void dma_stream_config(DMA_Stream_TypeDef *stream, const dma_config_t *cfg);
void dma_stream_enable_irq(DMA_Stream_TypeDef *stream, uint32_t flags);
void dma_stream_set_length(DMA_Stream_TypeDef *stream, uint16_t len);
void dma_stream_enable(DMA_Stream_TypeDef *stream);
void dma_stream_disable(DMA_Stream_TypeDef *stream);
uint8_t dma_get_flag(DMA_Stream_TypeDef *stream, dma_flag_t flag);
void dma_clear_flag(DMA_Stream_TypeDef *stream, dma_flag_t flag);

#endif // __DMA_H__

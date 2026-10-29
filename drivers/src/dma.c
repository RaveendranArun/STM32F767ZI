#include "dma.h"

/* Streams sit at <controller base> + 0x10 + 0x18 * n, n = 0..7 */
static inline DMA_TypeDef *dma_controller(DMA_Stream_TypeDef *stream)
{
    return (DMA_TypeDef *)((uint32_t)stream & ~0xFFU);
}

static inline uint32_t dma_stream_index(DMA_Stream_TypeDef *stream)
{
    return (((uint32_t)stream & 0xFFU) - 0x10U) / 0x18U;
}

/* Bit offset of the stream's 6-bit flag field inside LISR/HISR (or LIFCR/HIFCR) */
static inline uint32_t dma_flag_shift(uint32_t idx)
{
    static const uint8_t shift[4] = { 0U, 6U, 16U, 22U };
    return shift[idx & 0x3U];
}

void dma_clock_enable(DMA_Stream_TypeDef *stream)
{
    if (dma_controller(stream) == DMA1) RCC->AHB1ENR |= RCC_AHB1ENR_DMA1EN;
    else                                RCC->AHB1ENR |= RCC_AHB1ENR_DMA2EN;
}

void dma_stream_config(DMA_Stream_TypeDef *stream, const dma_config_t *cfg)
{
    uint32_t cr = 0U;

    //1. Enable the peripheral clock for the DMA controller
    dma_clock_enable(stream);

    //2. The stream must be disabled before it can be configured
    dma_stream_disable(stream);

    //3. Channel select, direction, data width and memory increment
    cr |= ((uint32_t)cfg->channel << DMA_SxCR_CHSEL_Pos);
    cr |= ((uint32_t)cfg->dir     << DMA_SxCR_DIR_Pos);
    cr |= ((uint32_t)cfg->width   << DMA_SxCR_PSIZE_Pos);
    cr |= ((uint32_t)cfg->width   << DMA_SxCR_MSIZE_Pos);
    if (cfg->mem_inc) cr |= DMA_SxCR_MINC;
    stream->CR = cr;

    //4. Program the source / destination addresses and number of data items
    stream->PAR  = cfg->periph_addr;
    stream->M0AR = cfg->mem_addr;
    stream->NDTR = cfg->len;

    //5. Select direct or FIFO mode, and the FIFO threshold
    if (cfg->fifo == DMA_FIFO_DIRECT)
    {
        stream->FCR &= ~(DMA_SxFCR_DMDIS | DMA_SxFCR_FTH);
    }
    else
    {
        stream->FCR &= ~DMA_SxFCR_FTH;
        stream->FCR |= DMA_SxFCR_DMDIS | ((uint32_t)(cfg->fifo - 1U) << DMA_SxFCR_FTH_Pos);
    }
}

/* 'flags' is an OR of DMA_FLAG_xx; enables the matching interrupt for each */
void dma_stream_enable_irq(DMA_Stream_TypeDef *stream, uint32_t flags)
{
    if (flags & DMA_FLAG_HT)  stream->CR  |= DMA_SxCR_HTIE;
    if (flags & DMA_FLAG_TC)  stream->CR  |= DMA_SxCR_TCIE;
    if (flags & DMA_FLAG_TE)  stream->CR  |= DMA_SxCR_TEIE;
    if (flags & DMA_FLAG_DME) stream->CR  |= DMA_SxCR_DMEIE;
    if (flags & DMA_FLAG_FE)  stream->FCR |= DMA_SxFCR_FEIE;
}

/* Only valid while the stream is disabled */
void dma_stream_set_length(DMA_Stream_TypeDef *stream, uint16_t len)
{
    stream->NDTR = len;
}

void dma_stream_enable(DMA_Stream_TypeDef *stream)
{
    stream->CR |= DMA_SxCR_EN;
}

void dma_stream_disable(DMA_Stream_TypeDef *stream)
{
    stream->CR &= ~DMA_SxCR_EN;

    // EN reads back 1 until the current transfer has finished
    while (stream->CR & DMA_SxCR_EN);
}

uint8_t dma_get_flag(DMA_Stream_TypeDef *stream, dma_flag_t flag)
{
    DMA_TypeDef *dma = dma_controller(stream);
    uint32_t idx     = dma_stream_index(stream);
    uint32_t isr     = (idx < 4U) ? dma->LISR : dma->HISR;

    return (isr & ((uint32_t)flag << dma_flag_shift(idx))) ? 1U : 0U;
}

void dma_clear_flag(DMA_Stream_TypeDef *stream, dma_flag_t flag)
{
    DMA_TypeDef *dma = dma_controller(stream);
    uint32_t idx     = dma_stream_index(stream);
    uint32_t mask    = ((uint32_t)flag << dma_flag_shift(idx));

    // LIFCR/HIFCR are write-1-to-clear
    if (idx < 4U) dma->LIFCR = mask;
    else          dma->HIFCR = mask;
}

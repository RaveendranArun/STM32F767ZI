#include "stm32f767xx.h"
#include "board.h"
#include "exti.h"
#include "usart.h"
#include "dma.h"

volatile uint32_t button_press_count = 0;

extern void HT_Complete_callback(void);
extern void FT_Complete_callback(void);
extern void TE_error_callback(void);
extern void DME_error_callback(void);
extern void FE_error_callback(void);

void EXTI15_10_IRQHandler(void)
{
    if (exti_is_pending(BOARD_BUTTON_PIN))
    {
        exti_clear_pending(BOARD_BUTTON_PIN);

        // Send USART3_TX DMA request to DMA1 controller
        usart_dma_tx_enable(BOARD_VCP_USART);
    }
}

// IRQ handler for DMA1 stream 3 global interrupt
void DMA1_Stream3_IRQHandler(void)
{
    DMA_Stream_TypeDef *stream = BOARD_VCP_TX_DMA_STREAM;

    if (dma_get_flag(stream, DMA_FLAG_HT))
    {
        dma_clear_flag(stream, DMA_FLAG_HT);
        HT_Complete_callback();
    }
    else if (dma_get_flag(stream, DMA_FLAG_TC))
    {
        dma_clear_flag(stream, DMA_FLAG_TC);
        FT_Complete_callback();
    }
    else if (dma_get_flag(stream, DMA_FLAG_TE))
    {
        dma_clear_flag(stream, DMA_FLAG_TE);
        TE_error_callback();
    }
    else if (dma_get_flag(stream, DMA_FLAG_FE))
    {
        dma_clear_flag(stream, DMA_FLAG_FE);
        FE_error_callback();
    }
    else if (dma_get_flag(stream, DMA_FLAG_DME))
    {
        dma_clear_flag(stream, DMA_FLAG_DME);
        DME_error_callback();
    }

}

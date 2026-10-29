#include <stdint.h>
#include "main.h"

#ifdef SEMIHOSTING
#include <stdio.h>
extern void initialise_monitor_handles(void);
#endif

static const char msg[] = "Hello World\r\n";
#define MSG_LEN     ((uint16_t)(sizeof(msg) - 1U))

/* Memory -> USART3 TDR, one byte at a time, armed but waiting for the USART's DMA request */
static void vcp_tx_dma_init(void)
{
    const dma_config_t cfg =
    {
        .channel     = BOARD_VCP_TX_DMA_CHANNEL,
        .dir         = DMA_DIR_M2P,
        .width       = DMA_WIDTH_BYTE,
        .mem_inc     = 1U,
        .fifo        = DMA_FIFO_FULL,
        .periph_addr = (uint32_t)&BOARD_VCP_USART->TDR,
        .mem_addr    = (uint32_t)msg,
        .len         = MSG_LEN
    };

    dma_stream_config(BOARD_VCP_TX_DMA_STREAM, &cfg);
    dma_stream_enable_irq(BOARD_VCP_TX_DMA_STREAM,
                          DMA_FLAG_HT | DMA_FLAG_TC | DMA_FLAG_TE | DMA_FLAG_DME | DMA_FLAG_FE);
    NVIC_EnableIRQ(BOARD_VCP_TX_DMA_IRQn);

    dma_stream_enable(BOARD_VCP_TX_DMA_STREAM);
}

//function prototypes of the callbacks
void HT_Complete_callback(void)
{

}

void FT_Complete_callback(void)
{
    
	dma_stream_set_length(BOARD_VCP_TX_DMA_STREAM, MSG_LEN); // Program number of data items to send
	usart_dma_tx_disable(BOARD_VCP_USART);                   // Disable USART3_TX DMA request to DMA1 controller

	dma_stream_enable(BOARD_VCP_TX_DMA_STREAM);
}

void TE_error_callback(void)
{
    while (1);
}

void DME_error_callback(void)
{
    while (1);
}

void FE_error_callback(void)
{
    while(1);
}

int main(void)
{
#ifdef SEMIHOSTING
    initialise_monitor_handles();
    printf("Hello from STM32\n");
#endif

    board_button_init();
    board_vcp_init();
    vcp_tx_dma_init();
    for (;;);
}
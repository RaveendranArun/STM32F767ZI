#include "usart.h"

void usart_clock_enable(USART_TypeDef *usart)
{
    // USART1/6 sit on APB2, the rest on APB1
    if      (usart == USART1) RCC->APB2ENR |= RCC_APB2ENR_USART1EN;
    else if (usart == USART6) RCC->APB2ENR |= RCC_APB2ENR_USART6EN;
    else if (usart == USART2) RCC->APB1ENR |= RCC_APB1ENR_USART2EN;
    else if (usart == USART3) RCC->APB1ENR |= RCC_APB1ENR_USART3EN;
    else if (usart == UART4)  RCC->APB1ENR |= RCC_APB1ENR_UART4EN;
    else if (usart == UART5)  RCC->APB1ENR |= RCC_APB1ENR_UART5EN;
    else if (usart == UART7)  RCC->APB1ENR |= RCC_APB1ENR_UART7EN;
    else if (usart == UART8)  RCC->APB1ENR |= RCC_APB1ENR_UART8EN;
}

/* 8N1, TX only. 'brr' = f_ck / baud (oversampling by 16) */
void usart_init(USART_TypeDef *usart, uint32_t brr)
{
    //1. Enable the peripheral clock for the USART peripheral
    usart_clock_enable(usart);

    //2. Configure the baud rate
    usart->BRR = brr;

    //3. Configure the data width, no of stop bits
    // <No configuration is required, we will use the default values>

    //4. Enable the TX engine of the USART peripheral
    usart->CR1 |= USART_CR1_TE;

    //5. Enable the USART peripheral
    usart->CR1 |= USART_CR1_UE;
}

void usart_send(USART_TypeDef *usart, const uint8_t *buf, uint32_t len)
{
    for (uint32_t i = 0; i < len; ++i)
    {
        // Waiting for the TXE to become 1
        while (!(usart->ISR & USART_ISR_TXE));
        usart->TDR = buf[i];
    }

    // Waiting for the TC to become 1 (last byte fully shifted out)
    while (!(usart->ISR & USART_ISR_TC));
}

// Let the USART raise TX DMA requests to the DMA controller
void usart_dma_tx_enable(USART_TypeDef *usart)
{
    usart->CR3 |= USART_CR3_DMAT;
}

void usart_dma_tx_disable(USART_TypeDef *usart)
{
    usart->CR3 &= ~USART_CR3_DMAT;
}

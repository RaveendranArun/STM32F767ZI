#ifndef __USART_H__
#define __USART_H__

#include <stdint.h>
#include "stm32f767xx.h"

void usart_clock_enable(USART_TypeDef *usart);
void usart_init(USART_TypeDef *usart, uint32_t brr);
void usart_send(USART_TypeDef *usart, const uint8_t *buf, uint32_t len);
void usart_dma_tx_enable(USART_TypeDef *usart);
void usart_dma_tx_disable(USART_TypeDef *usart);

#endif // __USART_H__

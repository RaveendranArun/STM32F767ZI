#ifndef __BOARD_H__
#define __BOARD_H__

#include "stm32f767xx.h"

/* Nucleo-144 STM32F767ZI pin assignments */

// User button B1
#define BOARD_BUTTON_PORT       GPIOC
#define BOARD_BUTTON_PIN        13U
#define BOARD_BUTTON_IRQn       EXTI15_10_IRQn

// ST-LINK virtual COM port: USART3, PD8 = TX, PD9 = RX
#define BOARD_VCP_USART         USART3
#define BOARD_VCP_PORT          GPIOD
#define BOARD_VCP_TX_PIN        8U
#define BOARD_VCP_RX_PIN        9U
#define BOARD_VCP_AF            7U
#define BOARD_VCP_BRR           0x8BU   // 16 MHz HSI / 115200 baud

// USART3_TX DMA request: DMA1 stream 3, channel 4 (RM0410 DMA1 request mapping)
#define BOARD_VCP_TX_DMA_STREAM     DMA1_Stream3
#define BOARD_VCP_TX_DMA_CHANNEL    4U
#define BOARD_VCP_TX_DMA_IRQn       DMA1_Stream3_IRQn

void board_button_init(void);
void board_vcp_init(void);

#endif // __BOARD_H__

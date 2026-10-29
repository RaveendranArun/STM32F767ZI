#ifndef __GPIO_H__
#define __GPIO_H__

#include <stdint.h>
#include "stm32f767xx.h"

/* Port index from base address: GPIOA = 0, GPIOB = 1, ... GPIOK = 10 */
#define GPIO_PORT_INDEX(GPIOx)  ((uint8_t)(((uint32_t)(GPIOx) - GPIOA_BASE) / 0x400U))

/* MODER field values */
typedef enum
{
    GPIO_MODE_INPUT  = 0U,
    GPIO_MODE_OUTPUT = 1U,
    GPIO_MODE_AF     = 2U,
    GPIO_MODE_ANALOG = 3U
} gpio_mode_t;

/* PUPDR field values */
typedef enum
{
    GPIO_PULL_NONE = 0U,
    GPIO_PULL_UP   = 1U,
    GPIO_PULL_DOWN = 2U
} gpio_pull_t;

void gpio_clock_enable(GPIO_TypeDef *port);
void gpio_set_mode(GPIO_TypeDef *port, uint8_t pin, gpio_mode_t mode);
void gpio_set_pull(GPIO_TypeDef *port, uint8_t pin, gpio_pull_t pull);
void gpio_set_af(GPIO_TypeDef *port, uint8_t pin, uint8_t af);

#endif // __GPIO_H__

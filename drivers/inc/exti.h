#ifndef __EXTI_H__
#define __EXTI_H__

#include <stdint.h>
#include "stm32f767xx.h"

/* Edge(s) that set the pending flag */
typedef enum
{
    EXTI_TRIGGER_RISING  = 1U,
    EXTI_TRIGGER_FALLING = 2U,
    EXTI_TRIGGER_BOTH    = 3U
} exti_trigger_t;

void exti_config(GPIO_TypeDef *port, uint8_t pin, exti_trigger_t trigger);
uint8_t exti_is_pending(uint8_t pin);
void exti_clear_pending(uint8_t pin);

#endif // __EXTI_H__

#include "exti.h"
#include "gpio.h"

/* Route EXTI line 'pin' to GPIO port 'port' (0 = A, 1 = B, 2 = C, ...) */
static inline void syscfg_exti_config(uint8_t port, uint8_t pin)
{
    uint32_t idx   = pin >> 2U;           /* pin / 4  -> EXTICR[0..3]   */
    uint32_t shift = (pin & 0x3U) * 4U;   /* (pin % 4) * 4 -> field pos */

    SYSCFG->EXTICR[idx] &= ~(0xFU << shift);
    SYSCFG->EXTICR[idx] |=  ((uint32_t)port << shift);
}

void exti_config(GPIO_TypeDef *port, uint8_t pin, exti_trigger_t trigger)
{
    uint32_t mask = (1U << pin);

    //1. Enable clock for SYS_CFG
    RCC->APB2ENR |= RCC_APB2ENR_SYSCFGEN;

    //2. Connect EXTI line 'pin' to the given GPIO port
    syscfg_exti_config(GPIO_PORT_INDEX(port), pin);

    //3. Configure the edge detection
    if (trigger & EXTI_TRIGGER_RISING)  EXTI->RTSR |= mask; else EXTI->RTSR &= ~mask;
    if (trigger & EXTI_TRIGGER_FALLING) EXTI->FTSR |= mask; else EXTI->FTSR &= ~mask;

    //4. Unmask the interrupt on that line (done last so a half-configured line can't fire)
    EXTI->IMR |= mask;
}

uint8_t exti_is_pending(uint8_t pin)
{
    return (EXTI->PR & (1U << pin)) ? 1U : 0U;
}

void exti_clear_pending(uint8_t pin)
{
    // PR is write-1-to-clear
    EXTI->PR = (1U << pin);
}

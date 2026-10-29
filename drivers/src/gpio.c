#include "gpio.h"

void gpio_clock_enable(GPIO_TypeDef *port)
{
    // GPIOxEN bits in AHB1ENR follow the port order: GPIOAEN = bit 0, GPIOBEN = bit 1, ...
    RCC->AHB1ENR |= (1U << GPIO_PORT_INDEX(port));
}

void gpio_set_mode(GPIO_TypeDef *port, uint8_t pin, gpio_mode_t mode)
{
    uint32_t shift = pin * 2U;

    port->MODER &= ~(0x3U << shift);
    port->MODER |=  ((uint32_t)mode << shift);
}

void gpio_set_pull(GPIO_TypeDef *port, uint8_t pin, gpio_pull_t pull)
{
    uint32_t shift = pin * 2U;

    port->PUPDR &= ~(0x3U << shift);
    port->PUPDR |=  ((uint32_t)pull << shift);
}

void gpio_set_af(GPIO_TypeDef *port, uint8_t pin, uint8_t af)
{
    uint32_t idx   = pin >> 3U;           /* pin / 8  -> AFR[0] (AFRL) or AFR[1] (AFRH) */
    uint32_t shift = (pin & 0x7U) * 4U;   /* (pin % 8) * 4 -> field pos */

    port->AFR[idx] &= ~(0xFU << shift);
    port->AFR[idx] |=  ((uint32_t)(af & 0xFU) << shift);
}

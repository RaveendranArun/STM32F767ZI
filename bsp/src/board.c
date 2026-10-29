#include "board.h"
#include "gpio.h"
#include "exti.h"
#include "usart.h"

void board_button_init(void)
{
    //1. Enable the peripheral clock for the button port
    gpio_clock_enable(BOARD_BUTTON_PORT);

    //2. Keep the pin in input mode (B1 has an external pull-down on the Nucleo)
    gpio_set_mode(BOARD_BUTTON_PORT, BOARD_BUTTON_PIN, GPIO_MODE_INPUT);

    //3. Route the pin to its EXTI line and trigger on the falling edge (button release)
    exti_config(BOARD_BUTTON_PORT, BOARD_BUTTON_PIN, EXTI_TRIGGER_FALLING);

    //4. Enable the IRQ related to that EXTI line in NVIC of the processor.
    NVIC_EnableIRQ(BOARD_BUTTON_IRQn);
}

void board_vcp_init(void)
{
    //1. Enable the clock for the VCP GPIO port
    gpio_clock_enable(BOARD_VCP_PORT);

    //2. PD8 as USART3_TX: alternate function mode with pull-up
    gpio_set_mode(BOARD_VCP_PORT, BOARD_VCP_TX_PIN, GPIO_MODE_AF);
    gpio_set_af(BOARD_VCP_PORT, BOARD_VCP_TX_PIN, BOARD_VCP_AF);
    gpio_set_pull(BOARD_VCP_PORT, BOARD_VCP_TX_PIN, GPIO_PULL_UP);

    //3. PD9 as USART3_RX: alternate function mode with pull-up
    gpio_set_mode(BOARD_VCP_PORT, BOARD_VCP_RX_PIN, GPIO_MODE_AF);
    gpio_set_af(BOARD_VCP_PORT, BOARD_VCP_RX_PIN, BOARD_VCP_AF);
    gpio_set_pull(BOARD_VCP_PORT, BOARD_VCP_RX_PIN, GPIO_PULL_UP);

    //4. Bring up the USART itself
    usart_init(BOARD_VCP_USART, BOARD_VCP_BRR);
}

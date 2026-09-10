#include "Lighting.hpp"

#include <stm32f030x6.h>

void configure_lighting_peripheral() {
    constexpr uint32_t af1 = 0b0001;
    constexpr uint32_t af2 = 0b0010;
    // Feed clock to GPIOA, GPIOB
    RCC->AHBENR |= RCC_AHBENR_GPIOAEN | RCC_AHBENR_GPIOBEN;
    // Feed clock to TIM1
    RCC->APB2ENR |= RCC_APB2ENR_TIM1EN;
    // Feed clock to TIM3
    RCC->APB1ENR |= RCC_APB1ENR_TIM3EN;

    // Select AF1 for PA7 (TIM3_CH2)
    GPIOA->AFR[0] |= (af1 << GPIO_AFRL_AFSEL7_Pos);
    // Select AF2 for PA8 (TIM1_CH1), PA9 (TIM1_CH2), PA10 (TIM1_CH3), PA11 (TIM1_CH4)
    GPIOA->AFR[1] |= (af2 << GPIO_AFRH_AFSEL8_Pos) | (af2 << GPIO_AFRH_AFSEL9_Pos) | (af2 << GPIO_AFRH_AFSEL10_Pos) |
                     (af2 << GPIO_AFRH_AFSEL11_Pos);

    // Select AF1 for PB0 (TIM3_CH3), PB1 (TIM3_CH4)
    GPIOB->AFR[0] |= (af1 << GPIO_AFRL_AFSEL0_Pos) | (af1 << GPIO_AFRL_AFSEL1_Pos);
}

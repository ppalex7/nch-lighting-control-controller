#include "stm32f030x6.h"

#include <io_expander.hpp>
#include <uart_logger.hpp>
#include <i2c_protocol.h>

volatile uint16_t g_expander_input;

void request_input_state() {
    // disable channel
    DMA1_Channel3->CCR &= (~DMA_CCR_EN_Msk);

    // set number of data to transfer
    DMA1_Channel3->CNDTR = sizeof(g_expander_input);

    // enable channel
    DMA1_Channel3->CCR |= DMA_CCR_EN;

    I2C1->CR2 = I2C_CR2_AUTOEND | (sizeof(g_expander_input) << I2C_CR2_NBYTES_Pos) | I2C_CR2_START | I2C_CR2_RD_WRN
            | (IO_EXPANDER_I2C_ADDRESS << I2C_CR2_SADD_Pos);
    uart_log("I2C configured to fetch input state from 0x%02X\n", IO_EXPANDER_I2C_ADDRESS);
}

void configure_peripheral_for_io_expander() {
    // Feed clock to GPIOB, GPIOA, DMA1
    RCC->AHBENR |= RCC_AHBENR_GPIOBEN | RCC_AHBENR_GPIOAEN | RCC_AHBENR_DMAEN;

    // Feed clock to system configuration controller
    RCC->APB2ENR |= RCC_APB2ENR_SYSCFGCOMPEN;

    // configure communication input pins: PA0
    // select source input
    SYSCFG->EXTICR[0] = SYSCFG_EXTICR1_EXTI0_PA;
    // enable rising trigger
    EXTI->RTSR = EXTI_RTSR_TR0;
    // set interrupt mask on lines
    EXTI->IMR |= EXTI_IMR_MR0;

    // Configure PB6, PB7 to alternate function
    GPIOB->MODER |= GPIO_MODER_MODER6_1 | GPIO_MODER_MODER7_1;
    // Select AF1 (I2C1_SCL) for PB6, AF1 (I2C1_SDA) for PB7
    GPIOB->AFR[0] |= (0b0001 << GPIO_AFRL_AFSEL7_Pos) | (0b0001 << GPIO_AFRL_AFSEL6_Pos);

    // Feed clock to I2C
    RCC->APB1ENR |= RCC_APB1ENR_I2C1EN;

    // Timing register value is computed with STM32CubeIDE:
    // standard mode @100kHz with I2CCLK = 12Mhz, rise time = 632ns (+20ns gap), fall time = 110ns (+20ns gap)
    const uint32_t timing = 0x00A02D3Au;
    I2C1->TIMINGR = timing;

    I2C1->CR1 |= I2C_CR1_RXDMAEN | I2C_CR1_PE;

    // set source peripheral address
    DMA1_Channel3->CPAR = reinterpret_cast<uint32_t>(&(I2C1->RXDR));

    // Configure the memory address
    DMA1_Channel3->CMAR = reinterpret_cast<uint32_t>(&g_expander_input);

    // priority level: high
    // memory increment mode
    // data transfer direction: read from peripheral
    // transfer complete interrupt enable
    DMA1_Channel3->CCR = DMA_CCR_PL_1 | DMA_CCR_MINC | DMA_CCR_TCIE;

    // enable interrupts
    NVIC_EnableIRQ(EXTI0_1_IRQn);
    NVIC_EnableIRQ(DMA1_Channel2_3_IRQn);
}

void on_dma_io_expander_transfer_complete() {
    if (DMA1->ISR & DMA_ISR_TCIF3) {
        uart_log("DMA TCIF3: fetched input state is 0x%04X\n", g_expander_input);
        DMA1->IFCR = DMA_IFCR_CTCIF3;
    }
}

void on_exti_io_expander_request() {
    if (EXTI->PR & EXTI_PR_PR0) {
        uart_log("IR from I/O-expander\n");

        request_input_state();

        EXTI->PR = EXTI_PR_PR0;
    }
}

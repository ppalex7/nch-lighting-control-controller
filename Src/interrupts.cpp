#include "stm32f030x6.h"

#include <interrupts.hpp>
#include <uart_logger.hpp>

void DMA1_CH2_3_IRQHandler() {
    on_dma_log_transfer_complete();
}

void EXTI0_1_IRQHandler() {
    if (EXTI->PR & EXTI_PR_PR0) {
        uart_log("IR from I/O-expander\n");
        EXTI->PR = EXTI_PR_PR0;
    }
}

#include "interrupts.hpp"

#include <stm32f030x6.h>
#include <uart_logger.hpp>

#include "io_expander.hpp"

void DMA1_CH2_3_IRQHandler() {
    on_dma_log_transfer_complete();
    on_dma_io_expander_transfer_complete();
}

void EXTI0_1_IRQHandler() {
    on_exti_io_expander_request();
}

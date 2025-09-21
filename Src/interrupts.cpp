#include "stm32f030x6.h"

#include <interrupts.hpp>
#include <uart_logger.hpp>

void DMA1_CH2_3_IRQHandler() {
    on_dma_log_transfer_complete();
}

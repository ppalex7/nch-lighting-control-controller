#include "stm32f030x6.h"

#include <main.hpp>
#include <uart_logger.hpp>

#if !defined(__SOFT_FP__) && defined(__ARM_FP)
  #warning "FPU is not initialized, but the project is compiling for an FPU. Please initialize the FPU before use."
#endif

static void configure_sysclock();

int main() {

    configure_sysclock();

    const unsigned int baud_rate = 115200;
    const unsigned int f_cpu = 12000000;
    // Set baud rate with oversampling by 16, so:
    configure_logger_peripheral(f_cpu / baud_rate);

    uart_log("device configured\n");

    while (1) {
        process_buffered_logs();
    }
}

/**
 * Configure clock to 12 MHz fed from HSE
 */
inline static void configure_sysclock() {
    // Enable High Speed External Clock
    RCC->CR |= RCC_CR_HSEON;

    // wait until HSE clock ready
    while (!(RCC->CR & RCC_CR_HSERDY))
        ;

    // switch to HSE clock
    RCC->CFGR = (RCC->CFGR & (~RCC_CFGR_SW)) | RCC_CFGR_SW_HSE;

    // wait until clock switched
    while ((RCC->CFGR & RCC_CFGR_SWS) != RCC_CFGR_SWS_HSE)
        ;
}

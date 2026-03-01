#ifndef HSE_VALUE
#error "HSE_VALUE is not defined, but required"
#elif (HSE_VALUE != 12000000U)
#warning "Unexpected HSE_VALUE"
#endif

#include "stm32f030x6.h"

#include <main.hpp>
#include <systick.hpp>
#include <uart_logger.hpp>
#include <io_expander.hpp>

#if !defined(__SOFT_FP__) && defined(__ARM_FP)
  #warning "FPU is not initialized, but the project is compiling for an FPU. Please initialize the FPU before use."
#endif

static void configure_sysclock();

int main() {
    configure_sysclock();
    SysTick_Config(HSE_VALUE / SYS_TICK_RATE_HZ);
    NVIC_EnableIRQ(SysTick_IRQn);

    const unsigned int baud_rate = 115200;
    // Set baud rate with oversampling by 16, so:
    configure_logger_peripheral(HSE_VALUE / baud_rate);


    configure_peripheral_for_io_expander();
    uart_log("I/O expander initialized\n");

    uart_log("device configured\n");

    request_input_state();

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



#include "stm32f030x6.h"

#include <main.hpp>

#if !defined(__SOFT_FP__) && defined(__ARM_FP)
  #warning "FPU is not initialized, but the project is compiling for an FPU. Please initialize the FPU before use."
#endif

int main() {

    configure_sysclock();

    while (1) {
    }
}

/**
 * Configure clock to 12 MHz fed from HSE
 */
static void configure_sysclock() {
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

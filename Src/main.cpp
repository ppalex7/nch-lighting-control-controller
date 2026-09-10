#ifndef HSE_VALUE
  #error "HSE_VALUE is not defined, but required"
#elif (HSE_VALUE != 12000000U)
  #warning "Unexpected HSE_VALUE"
#endif

#include "main.hpp"

#include <tuple>

#include <stm32f030x6.h>

#include "Lighting.hpp"
#include "io_expander.hpp"
#include "systick.hpp"
#include "uart_logger.hpp"

#if !defined(__SOFT_FP__) && defined(__ARM_FP)
  #warning "FPU is not initialized, but the project is compiling for an FPU. Please initialize the FPU before use."
#endif

#define SYS_TICK_RATE_HZ 1000U

static void configure_sysclock();

template <typename... Ls> static void tick_all(std::tuple<Ls...> &lightings) {
    std::apply([](auto &...l) { (l.tick(), ...); }, lightings);
}

using L1 = Lighting<PinDriver<GPIOB_BASE, 1>, 0b010000000>;
using L2 = Lighting<PinDriver<GPIOB_BASE, 0>, 0b001000000>;
using L3 = Lighting<PinDriver<GPIOA_BASE, 7>, 0b000100000>;
using L4 = Lighting<PinDriver<GPIOA_BASE, 9>, 0b000010000>;
using L5 = Lighting<PinDriver<GPIOA_BASE, 8>, 0b000001000>;
using L6 = Lighting<PinDriver<GPIOA_BASE, 11>, 0b000000100>;
using L7 = Lighting<PinDriver<GPIOA_BASE, 10>, 0b000000001>;

static std::tuple<L1, L2, L3, L4, L5, L6, L7> lightings;

int main() {
    configure_sysclock();
    SysTick_Config(HSE_VALUE / SYS_TICK_RATE_HZ);
    NVIC_EnableIRQ(SysTick_IRQn);

    const unsigned int baud_rate = 115200;
    // Set baud rate with oversampling by 16, so:
    configure_logger_peripheral(HSE_VALUE / baud_rate);

    configure_lighting_peripheral();

    configure_peripheral_for_io_expander();
    uart_log("I/O expander initialized\n");

    uart_log("device configured\n");

    request_input_state();

    while (1) {
        process_buffered_logs();
        tick_all(lightings);
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



#ifndef SYSTICK_HPP_
#define SYSTICK_HPP_

#ifndef SYS_TICK_RATE_HZ
#warning "SYS_TICK_RATE_HZ is not defined, set default value 1000"
#define SYS_TICK_RATE_HZ 1000U
#endif

#include <stdint.h>

extern volatile uint32_t system_ticks;

extern "C" void SysTick_Handler();

#define MS_TO_TICKS(ms)  (((uint32_t)(ms) * (SYS_TICK_RATE_HZ) + 500U) / 1000U)

#endif /* SYSTICK_HPP_ */

#ifndef SYSTICK_HPP_
#define SYSTICK_HPP_

#include <cstdint>

extern volatile uint32_t system_ticks;

extern "C" void SysTick_Handler();

#endif /* SYSTICK_HPP_ */

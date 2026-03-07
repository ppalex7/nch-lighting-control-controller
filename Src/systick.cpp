#include <systick.hpp>

volatile uint32_t system_ticks = 0;

void SysTick_Handler() {
    system_ticks++;
}

uint32_t EB_uptime() {
    return system_ticks;
}

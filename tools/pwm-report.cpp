#include <iomanip>
#include <iostream>

#include "PwmConfig.hpp"

int main() {
    constexpr double tickUs = 1e6 / pwm::counterClockHz;
    constexpr double periodUs = pwm::periodTicks * tickUs;

    std::cout << std::fixed << std::setprecision(6)
              << "PWM configuration (compile-time values, not hardware measurements)\n"
              << "Clock source: HSE, PLL bypassed, AHB/APB dividers = 1\n"
              << "HSE / CPU / SYSCLK / HCLK / PCLK: " << HSE_VALUE << " Hz\n"
              << "TIM1 / TIM3 input clock: " << pwm::timerClockHz << " Hz\n"
              << "PSC register: " << pwm::prescaler << " (divide by " << pwm::prescaler + 1u << ")\n"
              << "Counter clock: " << pwm::counterClockHz << " Hz\n"
              << "Counter tick: " << tickUs << " us\n"
              << "Counter mode: center-aligned mode 1 (up/down, period = 2 * ARR)\n"
              << "ARR register: " << pwm::autoReloadValue << '\n'
              << "Full period: " << pwm::periodTicks << " ticks / " << periodUs << " us\n"
              << "Requested PWM frequency: " << pwm::pwmFrequencyHz << " Hz\n"
              << "Actual PWM frequency: " << static_cast<double>(pwm::counterClockHz) / pwm::periodTicks << " Hz\n"
              << "TIM1 repetition counter (RCR): " << pwm::repetitionCounter << '\n'
              << "TIM1 update event frequency (overflow + underflow, divided by RCR + 1): "
              << static_cast<double>(pwm::counterClockHz) / pwm::autoReloadValue / (pwm::repetitionCounter + 1u)
              << " Hz\n"
              << "TIM3 update event frequency (overflow + underflow): "
              << static_cast<double>(pwm::counterClockHz) / pwm::autoReloadValue << " Hz\n"
              << "TIM1 initial CNT: 0\n"
              << "TIM3 initial CNT / phase offset: " << pwm::phaseShiftTicks << " ticks / "
              << pwm::phaseShiftTicks * tickUs << " us / "
              << 360.0 * pwm::phaseShiftTicks / pwm::periodTicks << " deg\n"
              << "Brightness levels: " << pwm::brightnessLevelCount << " (0.." << static_cast<unsigned>(pwm::maxLevel)
              << "), linear duty, rounded to the nearest compare tick\n"
              << "Duty resolution: " << 100.0 / pwm::autoReloadValue << " % per compare tick\n"
              << "Nominal brightness step: " << 100.0 / pwm::maxLevel << " %\n\n"
              << "Outputs are active high. CCR preload is enabled.\n"
              << "Duty ticks are half the active ticks in a full up/down period.\n"
              << "At level 0 / max the output is forced inactive / active; CCR is not written.\n"
              << "CCR columns show the equivalent PWM compare values, including the endpoints.\n\n"
              << "Level  DutyTicks  CCR(PWM1)  CCR(PWM2)  ActiveTicks     Duty(%)   OnTime(us)  Output\n";

    for (uint32_t level = 0u; level < pwm::brightnessLevelCount; ++level) {
        const auto dutyTicks = pwm::kDutyTable.ticks[level];
        const auto activeTicks = 2u * dutyTicks;
        const char *output = level == 0u ? "forced inactive" : level == pwm::maxLevel ? "forced active" : "PWM";
        std::cout << std::setw(5) << level << std::setw(11) << dutyTicks << std::setw(11) << dutyTicks
                  << std::setw(11) << pwm::autoReloadValue - dutyTicks << std::setw(13) << activeTicks
                  << std::setw(12) << 100.0 * activeTicks / pwm::periodTicks
                  << std::setw(13) << activeTicks * tickUs << "  " << output << '\n';
    }
}

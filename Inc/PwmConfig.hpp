#ifndef PWM_CONFIG_HPP_
#define PWM_CONFIG_HPP_

#include <cstdint>

#ifndef HSE_VALUE
#error "HSE_VALUE must be supplied by the build configuration"
#endif

namespace pwm {

inline constexpr uint32_t pwmFrequencyHz = 10000u;
inline constexpr uint32_t prescaler = 0u;
inline constexpr uint32_t repetitionCounter = 0u;
inline constexpr uint32_t gammaSampleCount = 64u;

inline constexpr uint32_t timerClockHz = HSE_VALUE;
inline constexpr uint32_t counterClockHz = timerClockHz / (prescaler + 1u);
inline constexpr uint32_t autoReloadValue = counterClockHz / (2u * pwmFrequencyHz);
inline constexpr uint32_t periodTicks = 2u * autoReloadValue;
inline constexpr uint32_t phaseShiftTicks = autoReloadValue / 2u;

// OCxM
inline constexpr uint32_t forceInactive = 0b100u;
inline constexpr uint32_t forceActive = 0b101u;
inline constexpr uint32_t kOcmPwm1 = 0x6u;
inline constexpr uint32_t kOcmPwm2 = 0x7u;

static_assert(repetitionCounter <= 0xFFu, "RCR does not fit into 8 bits");
static_assert(prescaler <= 0xFFFFu, "PSC does not fit into 16 bits");
static_assert(timerClockHz % (prescaler + 1u) == 0u, "timer clock is not divisible by the prescaler");
static_assert(
    counterClockHz % (2u * pwmFrequencyHz) == 0u, "PWM frequency is not exactly representable at this counter clock");
static_assert(autoReloadValue >= 1u && autoReloadValue <= 0xFFFFu, "ARR does not fit into 16 bits");
static_assert(autoReloadValue % 2u == 0u, "ARR must be even, otherwise the quarter period shift is not exact");

constexpr double apply_gamma_2_2(double brightness) {
    // x^2.2 = x^2 * fifth_root(x). Bisect the root in [0, 1] at compile time.
    double lower = 0.0;
    double upper = 1.0;
    for (uint32_t iteration = 0u; iteration < 40u; ++iteration) {
        const double middle = (lower + upper) * 0.5;
        const double square = middle * middle;
        if (square * square * middle < brightness) {
            lower = middle;
        } else {
            upper = middle;
        }
    }
    return brightness * brightness * (lower + upper) * 0.5;
}

struct GammaDutyTable {
    uint16_t ticks[gammaSampleCount];
    uint32_t count;
};

constexpr GammaDutyTable make_gamma_duty_table() {
    GammaDutyTable table{{0u}, 1u};
    for (uint32_t sample = 1u; sample < gammaSampleCount; ++sample) {
        const double brightness = static_cast<double>(sample) / (gammaSampleCount - 1u);
        const auto ticks = static_cast<uint16_t>(autoReloadValue * apply_gamma_2_2(brightness) + 0.5);
        if (ticks > table.ticks[table.count - 1u]) {
            table.ticks[table.count++] = ticks;
        }
    }
    return table;
}

inline constexpr GammaDutyTable gammaDutyTable = make_gamma_duty_table();
inline constexpr uint32_t brightnessLevelCount = gammaDutyTable.count;
inline constexpr uint8_t maxLevel = static_cast<uint8_t>(brightnessLevelCount - 1u);

static_assert(brightnessLevelCount >= 2u && brightnessLevelCount <= 256u,
    "level count must fit into uint8_t and provide at least two steps");
static_assert(autoReloadValue >= maxLevel, "ARR is too small to resolve all brightness levels");

struct DutyTable {
    uint16_t ticks[brightnessLevelCount];
};

constexpr DutyTable make_duty_table() {
    DutyTable table{};
    for (uint32_t level = 0u; level < brightnessLevelCount; ++level) {
        table.ticks[level] = gammaDutyTable.ticks[level];
    }
    return table;
}

inline constexpr DutyTable kDutyTable = make_duty_table();

static_assert(kDutyTable.ticks[0] == 0u, "level 0 must map to zero duty");
static_assert(kDutyTable.ticks[maxLevel] == autoReloadValue, "top level must map to full duty");

} // namespace pwm

#endif /* PWM_CONFIG_HPP_ */

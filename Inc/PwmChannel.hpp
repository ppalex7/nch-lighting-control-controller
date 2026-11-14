#ifndef PWM_CHANNEL_HPP_
#define PWM_CHANNEL_HPP_

#include <cstdint>

#include <stm32f030x6.h>

#include "PwmConfig.hpp"

namespace pwm {

template <uint32_t TimerBase, uint32_t TimerChannel, uint32_t PwmMode> class PwmChannel {
  public:
    static_assert(TimerBase == TIM1_BASE || TimerBase == TIM3_BASE, "only TIM1 and TIM3 are used for lighting");
    static_assert(TimerChannel >= 1u && TimerChannel <= 4u, "timer channel must be 1..4");
    static_assert(PwmMode == kOcmPwm1 || PwmMode == kOcmPwm2, "channel must run in PWM mode 1 or 2");

    static constexpr uint32_t timer_base = TimerBase;
    static constexpr uint32_t timer_channel = TimerChannel;
    static constexpr uint32_t pwm_mode = PwmMode;
    static constexpr uint8_t max_level = maxLevel;

    static constexpr uint32_t start_count = (TimerBase == TIM1_BASE) ? 0u : phaseShiftTicks;

    static constexpr uint32_t phase_ticks = (PwmMode == kOcmPwm1)
                                                ? ((periodTicks - start_count) % periodTicks)
                                                : ((autoReloadValue + periodTicks - start_count) % periodTicks);

    static constexpr uint32_t phase_deg = phase_ticks * 360u / periodTicks;

    static void set_level(uint8_t level) {
        if (level == 0u) {
            write_mode(forceInactive);
            return;
        }

        if (level >= max_level) {
            write_mode(forceActive);
            return;
        }

        ccr() = ccr_ticks(level);
        write_mode(PwmMode);
    }

  private:
    static constexpr uint32_t ocm_pos = (TimerChannel == 1u)   ? TIM_CCMR1_OC1M_Pos
                                        : (TimerChannel == 2u) ? TIM_CCMR1_OC2M_Pos
                                        : (TimerChannel == 3u) ? TIM_CCMR2_OC3M_Pos
                                                               : TIM_CCMR2_OC4M_Pos;

    static constexpr uint32_t ocm_msk = (TimerChannel == 1u)   ? TIM_CCMR1_OC1M_Msk
                                        : (TimerChannel == 2u) ? TIM_CCMR1_OC2M_Msk
                                        : (TimerChannel == 3u) ? TIM_CCMR2_OC3M_Msk
                                                               : TIM_CCMR2_OC4M_Msk;

    inline static TIM_TypeDef *timer() { return reinterpret_cast<TIM_TypeDef *>(TimerBase); }

    inline static volatile uint32_t &ccmr() {
        if constexpr (TimerChannel <= 2u) {
            return timer()->CCMR1;
        } else {
            return timer()->CCMR2;
        }
    }

    inline static volatile uint32_t &ccr() {
        if constexpr (TimerChannel == 1u) {
            return timer()->CCR1;
        } else if constexpr (TimerChannel == 2u) {
            return timer()->CCR2;
        } else if constexpr (TimerChannel == 3u) {
            return timer()->CCR3;
        } else {
            return timer()->CCR4;
        }
    }

    inline static uint32_t ccr_ticks(uint8_t level) {
        if constexpr (PwmMode == kOcmPwm1) {
            return kDutyTable.ticks[level];
        } else {
            return autoReloadValue - kDutyTable.ticks[level];
        }
    }

    inline static void write_mode(uint32_t mode) { ccmr() = (ccmr() & ~ocm_msk) | (mode << ocm_pos); }
};

using Ch1 = PwmChannel<TIM3_BASE, 4u, kOcmPwm1>;
using Ch2 = PwmChannel<TIM3_BASE, 3u, kOcmPwm2>;
using Ch3 = PwmChannel<TIM3_BASE, 2u, kOcmPwm1>;
using Ch4 = PwmChannel<TIM1_BASE, 1u, kOcmPwm2>;
using Ch5 = PwmChannel<TIM1_BASE, 2u, kOcmPwm1>;
using Ch6 = PwmChannel<TIM1_BASE, 4u, kOcmPwm2>;
using Ch7 = PwmChannel<TIM1_BASE, 3u, kOcmPwm1>;

static_assert((Ch2::phase_deg + 180u) % 360u == Ch3::phase_deg, "Q2 and Q3 must be in antiphase");
static_assert(Ch1::phase_deg != Ch4::phase_deg, "Q4 must not share a phase with Q1");
static_assert(Ch4::phase_deg != Ch5::phase_deg, "Q5 should not share a phase with Q4");
static_assert(Ch4::phase_deg != Ch7::phase_deg, "Q7 should not share a phase with Q4");
static_assert(Ch5::phase_deg != Ch6::phase_deg, "Q5 and Q6 should not share a phase");
static_assert(Ch6::phase_deg != Ch7::phase_deg, "Q6 and Q7 should not share a phase");

} // namespace pwm

#endif /* PWM_CHANNEL_HPP_ */

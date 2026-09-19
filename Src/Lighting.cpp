#include "Lighting.hpp"

#include <tuple>

#include <stm32f030x6.h>

#include "PwmChannel.hpp"

namespace {

// Q1: TIM3_CH4, PB1, AF1, PWM1, 270°
using L1 = Lighting<pwm::Ch1, 0b010000000, 1>;
// Q2: TIM3_CH3, PB0, AF1, PWM2, 90°
using L2 = Lighting<pwm::Ch2, 0b001000000, 2>;
// Q3: TIM3_CH2, PA7, AF1, PWM1, 270°
using L3 = Lighting<pwm::Ch3, 0b000100000, 3>;
// Q4: TIM1_CH2, PA9, AF2, PWM2, 180°
using L4 = Lighting<pwm::Ch4, 0b000010000, 4>;
// Q5: TIM1_CH1, PA8, AF2, PWM1, 0°
using L5 = Lighting<pwm::Ch5, 0b000001000, 5>;
// Q6: TIM1_CH4, PA11, AF2, PWM2, 180°
using L6 = Lighting<pwm::Ch6, 0b000000100, 6>;
// Q7: TIM1_CH3, PA10, AF2, PWM1, 0°
using L7 = Lighting<pwm::Ch7, 0b000000010, 7>;

std::tuple<L1, L2, L3, L4, L5, L6, L7> lightings;

} // namespace

void tick_lighting() {
    std::apply([](auto &...l) { (l.tick(), ...); }, lightings);
}

void configure_lighting_peripheral() {
    constexpr uint32_t moderAlternate = 0b10u;
    constexpr uint32_t af1 = 0b0001u;
    constexpr uint32_t af2 = 0b0010u;
    // Feed clock to GPIOA, GPIOB
    RCC->AHBENR |= RCC_AHBENR_GPIOAEN | RCC_AHBENR_GPIOBEN;
    // Feed clock to TIM1
    RCC->APB2ENR |= RCC_APB2ENR_TIM1EN;
    // Feed clock to TIM3
    RCC->APB1ENR |= RCC_APB1ENR_TIM3EN;
    // Dummy read of the last RCC write; the enable takes a few clocks to propagate and the first peripheral access
    // after it may otherwise be dropped.
    (void)RCC->APB1ENR;

    // Set Center-aligned mode 1, enable the ARR preload.
    // CEN stays 0 here: TIM1 must start last, since its start event is what releases TIM3.
    TIM1->CR1 = TIM_CR1_CMS_0 | TIM_CR1_ARPE;
    // Select 'enable' master mode makes TRGO follow the CEN bit.
    // This is the master half of the synchronisation; TRGO rises the moment TIM1 is started.
    TIM1->CR2 = TIM_CR2_MMS_0;
    // Set prescaler; counter clock = timer clock / (PSC + 1).
    TIM1->PSC = pwm::prescaler;
    // Set auto-reload value; in center-aligned mode one full period is 2 * ARR counter clocks.
    TIM1->ARR = pwm::autoReloadValue;
    // Repetition counter; 0 means every counter overflow/underflow raises an update event.
    TIM1->RCR = 0u;
    // Force OCxREF inactive on channels 1 and 2, and enable the CCR preload.
    // Force mode holds the outputs low regardless of CNT, which is what keeps the lamps initial off.
    TIM1->CCMR1 = (pwm::forceInactive << TIM_CCMR1_OC1M_Pos) | TIM_CCMR1_OC1PE |
                  (pwm::forceInactive << TIM_CCMR1_OC2M_Pos) | TIM_CCMR1_OC2PE;
    // same force-inactive plus preload setup for channels 3 and 4.
    TIM1->CCMR2 = (pwm::forceInactive << TIM_CCMR2_OC3M_Pos) | TIM_CCMR2_OC3PE |
                  (pwm::forceInactive << TIM_CCMR2_OC4M_Pos) | TIM_CCMR2_OC4PE;
    // compare value for channel 1 (PWM mode 1); 0 is its zero-duty value.
    TIM1->CCR1 = 0u;
    // compare value for channel 2 (PWM mode 2); ARR - not 0 - is the zero-duty value.
    // (because the output is active while CNT > CCR, and OCxPE defers the first CCR write to the next update event)
    TIM1->CCR2 = pwm::autoReloadValue;
    // compare value for channel 3 (PWM mode 1); zero duty.
    TIM1->CCR3 = 0u;
    // compare value for channel 4 (PWM mode 2); zero duty.
    TIM1->CCR4 = pwm::autoReloadValue;
    // CCxE=1 routes OCxREF to the pin; CCxP is left 0, so all outputs are active high.
    TIM1->CCER = TIM_CCER_CC1E | TIM_CCER_CC2E | TIM_CCER_CC3E | TIM_CCER_CC4E;
    // Master output enable.
    TIM1->BDTR = TIM_BDTR_MOE;
    // Generate an update event: transferring PSC, ARR and the CCRs into their shadow registers, clearing the counter.
    TIM1->EGR = TIM_EGR_UG;
    // Clear the UIF flag that UG has just raised.
    TIM1->SR = 0u;

    // Same center-aligned mode 1 and ARR preload as TIM1; CEN stays 0 because the slave-mode controller will set it on
    // the trigger.
    TIM3->CR1 = TIM_CR1_CMS_0 | TIM_CR1_ARPE;
    // identical prescaler, otherwise the two timers would drift apart.
    TIM3->PSC = pwm::prescaler;
    // identical period, for the same reason.
    TIM3->ARR = pwm::autoReloadValue;
    // channel 2 forced inactive with CCR preload enabled; channel 1 is unused.
    TIM3->CCMR1 = (pwm::forceInactive << TIM_CCMR1_OC2M_Pos) | TIM_CCMR1_OC2PE;
    // channels 3 and 4 forced inactive with CCR preload enabled.
    TIM3->CCMR2 = (pwm::forceInactive << TIM_CCMR2_OC3M_Pos) | TIM_CCMR2_OC3PE |
                  (pwm::forceInactive << TIM_CCMR2_OC4M_Pos) | TIM_CCMR2_OC4PE;
    // channel 2 is PWM mode 1, zero duty.
    TIM3->CCR2 = 0u;
    // channel 3 is PWM mode 2, so ARR is its zero-duty value.
    TIM3->CCR3 = pwm::autoReloadValue;
    // channel 4 is PWM mode 1, zero duty.
    TIM3->CCR4 = 0u;
    // enable outputs for channels 2..4, active high; channel 1 is unused.
    TIM3->CCER = TIM_CCER_CC2E | TIM_CCER_CC3E | TIM_CCER_CC4E;
    // UG loads the shadow registers, as on TIM1.
    TIM3->EGR = TIM_EGR_UG;
    // clear the UIF raised by UG.
    TIM3->SR = 0u;
    // preload the counter with a quarter of the full period (ARR/2 out of 2*ARR).
    // This is the whole phase shift mechanism, and it must come after EGR.UG, which would
    // otherwise clear the counter back to zero.
    TIM3->CNT = pwm::phaseShiftTicks;
    // SMS=110 is trigger mode - the trigger sets CEN without resetting CNT, so the preloaded offset survives the start.
    // TS=000 selects ITR0, which maps to TIM1_TRGO.
    TIM3->SMCR = TIM_SMCR_SMS_2 | TIM_SMCR_SMS_1;

    // start TIM1; TRGO rises with it and releases TIM3 from its preloaded count, leaving the two timers permanently 90
    // degrees apart.
    TIM1->CR1 |= TIM_CR1_CEN;

    // Select AF1 for PA7 (TIM3_CH2)
    GPIOA->AFR[0] |= (af1 << GPIO_AFRL_AFSEL7_Pos);
    // Select AF2 for PA8 (TIM1_CH1), PA9 (TIM1_CH2), PA10 (TIM1_CH3), PA11 (TIM1_CH4)
    GPIOA->AFR[1] |= (af2 << GPIO_AFRH_AFSEL8_Pos) | (af2 << GPIO_AFRH_AFSEL9_Pos) | (af2 << GPIO_AFRH_AFSEL10_Pos) |
                     (af2 << GPIO_AFRH_AFSEL11_Pos);

    // Select AF1 for PB0 (TIM3_CH3), PB1 (TIM3_CH4)
    GPIOB->AFR[0] |= (af1 << GPIO_AFRL_AFSEL0_Pos) | (af1 << GPIO_AFRL_AFSEL1_Pos);

    // Select Alternate function mode for PA7, PA8, PA9, PA10, PA11
    GPIOA->MODER |= (moderAlternate << GPIO_MODER_MODER7_Pos) | (moderAlternate << GPIO_MODER_MODER8_Pos) |
                    (moderAlternate << GPIO_MODER_MODER9_Pos) | (moderAlternate << GPIO_MODER_MODER10_Pos) |
                    (moderAlternate << GPIO_MODER_MODER11_Pos);

    // Select Alternate function mode for PB0, PB1
    GPIOB->MODER |= (moderAlternate << GPIO_MODER_MODER0_Pos) | (moderAlternate << GPIO_MODER_MODER1_Pos);
}

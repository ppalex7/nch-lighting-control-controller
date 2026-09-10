#ifndef LIGHTING_HPP_
#define LIGHTING_HPP_

#include <cstdint>

#include <stm32f030x6.h>

#define EB_NO_CALLBACK
#define EB_NO_FOR
#define EB_DEB_TIME 0
#include "VirtButton.h"

#include "io_expander.hpp"
#include "systick.hpp"
#include "uart_logger.hpp"

void configure_lighting_peripheral();

// TODO add: timer and channel
template<uint32_t PortBase, uint8_t PinNum>
class PinDriver {
private:
    static constexpr uint32_t moder_output = 0b01;
    static constexpr uint32_t moder_af = 0b10;
    static constexpr uint32_t moder_mask = 0b11;

    inline static GPIO_TypeDef* gpio() {
        return reinterpret_cast<GPIO_TypeDef*>(PortBase);
    }

    inline static void set_pin_mode(const uint32_t moder) {
        gpio()->MODER = (gpio()->MODER & ~(moder_mask << PinNum * 2)) | (moder << PinNum * 2);
    }

public:
    static void set_high() {
        set_pin_mode(moder_output);
        gpio()->BSRR = (0b1U << PinNum);

    }
    static void set_low() {
        set_pin_mode(moder_output);
        gpio()->BRR = (0b1U << PinNum);
    }

    static void set_pwm(uint16_t value) {
        set_pin_mode(moder_af);
        // TODO: configure timer
    }
};

template<typename PinDriver, uint16_t expander_input_mask>
class Lighting {
private:
    static constexpr uint8_t max_level = 63;
    static constexpr uint8_t fade_step_time = 8;

    bool dimming_direction_up;

    uint8_t current_level;
    uint8_t target_level;
    uint8_t saved_level;

    uint32_t last_fade_time;

    VirtButton button;

    void set_output_level(uint8_t level) {
        if (level == 0) {
            PinDriver::set_low();
        } else if (level >= max_level) {
            PinDriver::set_high();
        } else {
            PinDriver::set_pwm(level);
        }
    }

public:
    Lighting() :
            dimming_direction_up(true), current_level(0), target_level(0), saved_level(max_level), last_fade_time(0) {

    }

    void tick() {
        if (button.tick(g_expander_input & expander_input_mask)) {

            if (button.click()) {
                target_level = (current_level > 0) ? saved_level : 0;
                uart_log("Button click: set target_level=%d", target_level);
            }

            if (button.step()) {
                uart_log("Button step: with direction_up=%d, set ", dimming_direction_up);
                if (current_level == 0) {
                    dimming_direction_up = true;
                    current_level = 1;
                    uart_log("enabled=1, dimming_direction_up=1, ");
                }

                if (dimming_direction_up && target_level < max_level) {
                    target_level++;
                } else if (!dimming_direction_up && target_level > 1) {
                    target_level--;
                }
                uart_log("target_level=%d\n", target_level);

                saved_level = target_level;
            }

            if (button.releaseStep()) {
                dimming_direction_up = !dimming_direction_up;
                uart_log("Button releaseStep: set direction_up=%d\n", dimming_direction_up);
            }
        }

        if (current_level != target_level) {
            if (system_ticks - last_fade_time > fade_step_time) {
                if (current_level < target_level) {
                    current_level++;
                } else {
                    current_level--;
                }
                set_output_level(current_level);
                uart_log("fading, current_level=%d\n", current_level);
                last_fade_time = system_ticks;
            }
        }
    }
};

#endif /* LIGHTING_HPP_ */

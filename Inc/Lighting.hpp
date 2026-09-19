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
void tick_lighting();

template <typename Channel, uint16_t expander_input_mask, uint8_t log_channel> class Lighting {
  private:
    static constexpr uint8_t max_level = Channel::max_level;
    static constexpr uint8_t fade_step_time = 8;

    bool dimming_direction_up;

    uint8_t current_level;
    uint8_t target_level;
    uint8_t saved_level;

    uint32_t last_fade_time;

    VirtButton button;

  public:
    Lighting()
        : dimming_direction_up(true), current_level(0), target_level(0), saved_level(max_level), last_fade_time(0) {}

    void tick() {
        if (button.tick(g_expander_input & expander_input_mask)) {

            if (button.click()) {
                target_level = (current_level > 0) ? 0 : saved_level;
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
                // Fading crosses level 0 and max_level, so OCxM has to be
                // switched between force and PWM modes here as well:
                // set_level() must stay a full mode+CCR update, not a CCR-only write.
                Channel::set_level(current_level);
                last_fade_time = system_ticks;
                uart_log("fading, current_level=%d\n", current_level);
                last_fade_time = system_ticks;
            }
        }
    }
};

#endif /* LIGHTING_HPP_ */

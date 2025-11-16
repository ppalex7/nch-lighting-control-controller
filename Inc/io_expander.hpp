#ifndef IO_EXPANDER_HPP_
#define IO_EXPANDER_HPP_

// PORTD                 PORTB
// 0 0 0 0  0 0 0 SW9    SW1 SW2 SW3 SW4  SW5 SW6 SW7 SW8
extern volatile uint16_t g_expander_input;

void request_input_state();
void configure_peripheral_for_io_expander();
void on_dma_io_expander_transfer_complete();
void on_exti_io_expander_request();

#endif /* IO_EXPANDER_HPP_ */

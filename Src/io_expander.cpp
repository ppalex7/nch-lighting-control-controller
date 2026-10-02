#include "io_expander.hpp"

#include <stm32f030x6.h>

#include "i2c_protocol.h"
#include "systick.hpp"
#include "uart_logger.hpp"

volatile uint16_t g_expander_input;

namespace {

enum class State : uint8_t { Idle, Busy, Cooldown };

volatile State g_state = State::Idle;
volatile uint32_t g_phase_started = 0;
// Set by on_exti_io_expander_request(), consumed and cleared in tick_io_expander()'s
// Idle case; also resets g_consecutive_failures (see there for why).
volatile bool g_read_pending = false;
// Set by on_i2c_io_expander_error() (interrupt context), consumed in
// tick_io_expander()'s Cooldown case. note_failure() does a read-modify-write on
// g_consecutive_failures, so it must only ever run from one context; deferring the
// count to the main loop instead of doing it inside the ISR is what keeps that true.
volatile bool g_error_pending = false;
volatile uint8_t g_consecutive_failures = 0;

// Real transfer is ~300 us at 100 kHz (address + 2 data bytes). 20 ms gives wide margin
// over any plausible STM8 ISR latency and is the only backstop for a slave holding SDA
// with no clock activity at all (BUSY set forever, no flag ever fires for that case).
constexpr uint32_t transaction_timeout_ms = 20;
// Pace between an error/timeout and the next attempt. Errors are expected to be rare
// after the STM8 boot race; no need to hammer the bus faster than this.
constexpr uint32_t retry_delay_ms = 5;
// After this many consecutive failures the automat stops attempting on its own; only
// a fresh EXTI edge (a real new event, not just the same stale condition) re-arms it.
constexpr uint8_t max_consecutive_failures = 5;

// Shared cleanup for any transfer that will not complete: leaves DMA ch3 and I2C1
// flags in a clean state for the next attempt. Does not touch PE — a BUSY flag stuck
// by a slave physically holding SDA is a decision the caller (tick_io_expander) takes
// explicitly, not a side effect of cleanup.
void abort_active_transfer() {
    DMA1_Channel3->CCR &= ~DMA_CCR_EN_Msk;
    // CGIF3 clears TCIF3/HTIF3/TEIF3/GIF3 for channel 3 in one write.
    DMA1->IFCR = DMA_IFCR_CGIF3;
    I2C1->ICR = I2C_ICR_NACKCF | I2C_ICR_STOPCF | I2C_ICR_BERRCF | I2C_ICR_ARLOCF | I2C_ICR_OVRCF;
}

// Called only from tick_io_expander() (see g_error_pending above for why) — once for
// a software timeout, once for a deferred ISR-reported error. Logs once, exactly at
// the moment the cap is reached, not on every subsequent Idle check.
void note_failure() {
    if (g_consecutive_failures < max_consecutive_failures) {
        ++g_consecutive_failures;
        if (g_consecutive_failures == max_consecutive_failures) {
            uart_log("I2C giving up after %d consecutive failures\n", g_consecutive_failures);
        }
    }
}

} // namespace

void request_input_state() {
    // disable channel
    DMA1_Channel3->CCR &= (~DMA_CCR_EN_Msk);

    // set number of data to transfer
    DMA1_Channel3->CNDTR = sizeof(g_expander_input);

    // enable channel
    DMA1_Channel3->CCR |= DMA_CCR_EN;

    // Must be set before CR2/START: an address-phase NACK can be fast enough that the
    // I2C1 ISR runs before this function returns, and it needs to see Busy already.
    g_state = State::Busy;
    g_phase_started = system_ticks;

    I2C1->CR2 = I2C_CR2_AUTOEND | (sizeof(g_expander_input) << I2C_CR2_NBYTES_Pos) | I2C_CR2_START | I2C_CR2_RD_WRN |
                (IO_EXPANDER_I2C_ADDRESS << I2C_CR2_SADD_Pos);
    uart_log("I2C configured to fetch input state from 0x%02X\n", IO_EXPANDER_I2C_ADDRESS);
}

void configure_peripheral_for_io_expander() {
    // Feed clock to GPIOB, GPIOA, DMA1
    RCC->AHBENR |= RCC_AHBENR_GPIOBEN | RCC_AHBENR_GPIOAEN | RCC_AHBENR_DMAEN;

    // Feed clock to system configuration controller
    RCC->APB2ENR |= RCC_APB2ENR_SYSCFGCOMPEN;

    // configure communication input pins: PA0
    // select source input
    SYSCFG->EXTICR[0] = SYSCFG_EXTICR1_EXTI0_PA;
    // enable rising trigger
    EXTI->RTSR = EXTI_RTSR_TR0;
    // set interrupt mask on lines
    EXTI->IMR |= EXTI_IMR_MR0;

    // OTYPER: output type open-drain for PB6/PB7. I2C is a wired-AND bus; in
    // push-pull (the reset default) the master keeps driving SDA high through the
    // ACK bit slot and fights the slave's pull-down, so a real ACK can be sampled
    // as a NACK.
    GPIOB->OTYPER |= GPIO_OTYPER_OT_6 | GPIO_OTYPER_OT_7;
    // Select AF1 (I2C1_SCL) for PB6, AF1 (I2C1_SDA) for PB7
    GPIOB->AFR[0] |= (0b0001 << GPIO_AFRL_AFSEL7_Pos) | (0b0001 << GPIO_AFRL_AFSEL6_Pos);
    // Configure PB6, PB7 to alternate function
    GPIOB->MODER |= GPIO_MODER_MODER6_1 | GPIO_MODER_MODER7_1;

    // Feed clock to I2C
    RCC->APB1ENR |= RCC_APB1ENR_I2C1EN;

    // Select SYSCLK (HSE, 12 MHz) as the I2C1 kernel clock instead of the reset
    // default HSI (8 MHz), to match the TIMINGR value computed below for 12 MHz.
    RCC->CFGR3 |= RCC_CFGR3_I2C1SW_SYSCLK;

    // Timing register value is computed with STM32CubeIDE:
    // standard mode @100kHz with I2CCLK = 12Mhz, rise time = 632ns (+20ns gap), fall time = 110ns (+20ns gap)
    const uint32_t timing = 0x00A02D3Au;
    I2C1->TIMINGR = timing;

    // RXDMAEN routes RXNE to DMA ch3. NACKIE reports a NACK'ed address, ERRIE
    // covers BERR/ARLO/OVR. Without them a NACK was invisible: hardware sends
    // STOP on its own, RXNE never sets, DMA never gets a request, and the
    // transfer-complete interrupt the code was waiting for never fired.
    I2C1->CR1 |= I2C_CR1_RXDMAEN | I2C_CR1_NACKIE | I2C_CR1_ERRIE | I2C_CR1_PE;

    // set source peripheral address
    DMA1_Channel3->CPAR = reinterpret_cast<uint32_t>(&(I2C1->RXDR));

    // Configure the memory address
    DMA1_Channel3->CMAR = reinterpret_cast<uint32_t>(&g_expander_input);

    // priority level: high
    // memory increment mode
    // data transfer direction: read from peripheral
    // transfer complete interrupt enable
    DMA1_Channel3->CCR = DMA_CCR_PL_1 | DMA_CCR_MINC | DMA_CCR_TCIE;

    // enable interrupts
    NVIC_EnableIRQ(EXTI0_1_IRQn);
    NVIC_EnableIRQ(DMA1_Channel2_3_IRQn);
    NVIC_EnableIRQ(I2C1_IRQn);
}

void on_dma_io_expander_transfer_complete() {
    if (DMA1->ISR & DMA_ISR_TCIF3) {
        uart_log("DMA TCIF3: fetched input state is 0x%04X\n", g_expander_input);
        DMA1->IFCR = DMA_IFCR_CTCIF3;

        g_consecutive_failures = 0;
        // Idle re-checks the PA0 level itself; if the STM8 kept the request line
        // asserted because the input changed again mid-transfer (see
        // I2C1_SPI2_IRQHandler in the STM8 firmware), no new EXTI edge will ever
        // arrive, so returning to Idle unconditionally is what actually re-polls it.
        g_state = State::Idle;
    }
}

void on_i2c_io_expander_error() {
    // Snapshot before clearing: reading ISR after ICR/IFCR writes would see the
    // flags already cleared.
    const uint32_t status = I2C1->ISR;
    abort_active_transfer();
    // Does not call note_failure() itself — see g_error_pending's comment above for why.
    g_error_pending = true;
    g_state = State::Cooldown;
    g_phase_started = system_ticks;
    // NACKF is bit 4, BERR is bit 8, ARLO is bit 9, OVR is bit 10 of ISR.
    uart_log("I2C error, ISR=0x%04X\n", static_cast<uint16_t>(status));
}

void tick_io_expander() {
    switch (g_state) {
    case State::Busy:
        if (system_ticks - g_phase_started > transaction_timeout_ms) {
            const uint32_t status = I2C1->ISR;
            abort_active_transfer();
            // Discard a same-instant ISR report: this path already accounts the
            // failure itself below, note_failure() must run exactly once per event.
            g_error_pending = false;
            note_failure();
            g_state = State::Cooldown;
            g_phase_started = system_ticks;
            uart_log("I2C timeout, ISR=0x%04X\n", static_cast<uint16_t>(status));
        }
        return;

    case State::Cooldown:
        if (g_error_pending) {
            // The only place note_failure() is ever called from now: always main-loop
            // context, never interrupt context, so the increment has no RMW race.
            g_error_pending = false;
            note_failure();
        }
        if (system_ticks - g_phase_started <= retry_delay_ms) {
            return;
        }
        g_state = State::Idle;
        [[fallthrough]];

    case State::Idle:
        // BUSY is cleared by hardware only on a detected Stop or on PE=0. If a slave
        // physically holds SDA with no clock activity, it never clears here; nothing
        // further is attempted after the timeout logged above, on purpose — no
        // automatic bus recovery in this iteration.
        if (I2C1->ISR & I2C_ISR_BUSY) {
            return;
        }
        if (g_consecutive_failures >= max_consecutive_failures) {
            // Gave up; only on_exti_io_expander_request() resetting the counter on a
            // fresh edge re-arms this.
            return;
        }
        if (!g_read_pending && !(GPIOA->IDR & GPIO_IDR_0)) {
            return;
        }
        g_read_pending = false;
        request_input_state();
        return;
    }
}

void on_exti_io_expander_request() {
    if (EXTI->PR & EXTI_PR_PR0) {
        // Clear the pending flag before handling the request: a rising edge that
        // arrives while this handler is still running would otherwise be
        // overwritten by this same clear at the end and lost forever.
        EXTI->PR = EXTI_PR_PR0;

        g_read_pending = true;
        // A genuinely new event from the STM8 re-arms retries after the failure cap
        // in tick_io_expander() — it means something changed, not just the same
        // stuck condition.
        g_consecutive_failures = 0;
        uart_log("IR from I/O-expander\n");
    }
}

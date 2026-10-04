#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/adc.h"
#include "hardware/clocks.h"
#include "hardware/pio.h"
#include "PWM.pio.h"
#include "hardware/pll.h"

#pragma region INIT VALUES

// PLL
static uint32_t pll_freq;
#define REF_DIV 1
#define VCO_FREQ 1250000
#define POST_DIV_1 5
#define POST_DIV_2 1

// ADC
#define VOUTSENSE 26
static uint16_t vout_offset;

// PIO for PWM
static PIO PWM = pio0;
#define SM 0
#define LO 0
#define HI 1
#define PINDIRS 1
static uint32_t pinmask = 0b11;
static uint offset;
#define BASE 0

// Indicator light
#define ledpin 25

#pragma endregion

// Output and tuning
static uint freq = 300000; // for initial frequency
static float deadtime = 0.01f;
static uint64_t current_time;
#define VSET 200
#define KP 1000
#define DIVIDE_ROUND(a, b) (((a) + ((b) / 2)) / (b))
#define CONTROLLER_FREQ    1000U
#define LOOP_DT            DIVIDE_ROUND(1000000U, CONTROLLER_FREQ) // timer counts once per us
#define ADCPERVOLT 44.63
#define FREQPERVOLT 5000u

#pragma region DEFS

// PIO setup on pins 0 and 1
static inline void PWM_setup(PIO pio, uint sm, uint lo, uint hi, uint pindirs, uint32_t mask, uint offset, uint base){
    pio_gpio_init(pio, lo);
    pio_gpio_init(pio, hi);
    pio_sm_set_pindirs_with_mask(pio, sm, pindirs, mask);
    pio_sm_config c = PWM_program_get_default_config(offset);
    sm_config_set_sideset_pins(&c, base);
    sm_config_set_out_shift(&c, true, false, 16); // 16 is just pull_threshold
    sm_config_set_fifo_join(&c, PIO_FIFO_JOIN_TX);

    pio_sm_init(pio, sm, offset, &c);
    pio_sm_set_enabled(pio, sm, true);
}

static inline void PWM_set_freq(PIO pio, uint sm, uint32_t active_counter, uint32_t deadtime_counter){
    if (!pio_sm_is_tx_fifo_full(pio, sm)){
        pio_sm_put(pio, sm, (uint32_t)active_counter << 12 | deadtime_counter);
    }
}

typedef struct{
    uint32_t active;
    uint32_t deadtime;
} PWM_cycles;

PWM_cycles PWM_target_freq(uint freq, uint32_t pll_freq, float deadtime){
    // request some frequency, given pll frequency, and percentage of deadtime (0.01 = 1% deadtime)
    // minimum of 12 cycles, and an additional 2 cycles for each active and deadtime
    uint approx_cycles = (uint) (pll_freq / freq);
    uint approx_deadtime = (uint) (approx_cycles * deadtime);
    int deadtime_cycles = approx_deadtime - 5; // amount of required dead cycles
    if (deadtime_cycles < 0){
        deadtime_cycles = 0;
    }
    int active_cycles = approx_cycles -  deadtime_cycles;
    if (active_cycles < 0){
        active_cycles = 0;
    }
    return (PWM_cycles){active_cycles, deadtime_cycles};
}

static inline void disable_sm(PIO pio, uint sm, uint pin) {
    pio_sm_exec(pio, sm, pio_encode_set(pio_pins, 0));
    pio_sm_set_enabled(pio, sm, false);
}

static inline void enable_sm(PIO pio, uint sm){
    pio_sm_set_enabled(pio, sm true);
}

// Quick conversion methods

static double adc_to_vout(uint adc_values){
    return adc_values / ADCPERVOLT;
}

static double vout_to_freq(double vout){
    return vout * FREQPERVOUT;
}

#pragma endregion

int main()
{
    #pragma region INITS

    stdio_init_all();
    adc_init();

    sleep_ms(5000);

    // PLL STUFF
    pll_init (pll_sys, REF_DIV, VCO_FREQ, POST_DIV_1, POST_DIV_1); // oc to 250MHz
    pll_freq = clock_get_hz(clk_sys);

    // ADC STUFF
    adc_gpio_init(VOUTSENSE); // Pin 26
    adc_select_input(0); // 0-3 is 26-29 on board
    // not really sure whether its good to let the adc run or not, but configuring it to not run for now
    //adc_run(true);
    vout_offset = adc_read();
    // technically overclocking the adc up to 4MS/s is possible, but keeping it at base frequency means no glitches
    //clock_configure(clk_adc, CLK_DEST_SYS_CLOCKS, CLOCKS_CLK_ADC_CTRL_AUXSRC_VALUE_CLKSRC_PLL_SYS, pll_freq, pll_freq);

    //PWM STUFF
    offset = pio_add_program(PWM, &PWM_program);
    PWM_setup(PWM, SM, LO, HI, PINDIRS, pinmask, offset, BASE);

    // Indicator light
    gpio_set_function(ledpin, GPIO_FUNC_SIO);
    gpio_put(ledpin, true);

    #pragma endregion

    // Startup first
    uint elapsed;
    double vout;
    double error;
    sleep_ms(10000);

    for (int i = 0; i < 100; i++;){
        current_time = time_us_64(); // first pulse

        PWM_cycles target_freq = PWM_target_freq(freq, pll_freq, deadtime);
        PWM_set_freq(PWM, SM, target_freq.active, target_freq.deadtime);
        enable_sm(PWM, SM);

        elapsed = time_us_64() - current_time;
        while (elapsed < LOOP_DT/2){
            elapsed = time_us_64() - current_time;
        }

        current_time = time_us_64(); // next pulse

        disable_sm(PIO, SM, 0);
        freq += 5000;

        elapsed = time_us_64() - current_time;
        while (elapsed < LOOP_DT/2){
            elapsed = time_us_64() - current_time;
        }
    }
}

#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/adc.h"
#include "hardware/clocks.h"
#include "hardware/pio.h"
#include "PWM.pio.h"

// CONTROLLER VALUES
static uint freq = 300000;
static float deadtime = 0.01f;

// PLL
static uint32_t pll_freq;

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

// Output
#define VOUT 200

// PLL setup

// PIO setup on pins 0 and 1
static inline void PWM_setup(PIO pio, uint sm, uint lo, uint hi, uint pindirs, uint32_t mask, uint offset, uint base){
    pio_gpio_init(pio, lo);
    pio_gpio_init(pio, hi);
    pio_sm_set_pindirs_with_mask(pio, sm, pindirs, mask);
    pio_sm_config c = PWM_program_get_default_config(offset);
    sm_config_set_sideset_pins(&PWM_program, base);
    sm_config_set_out_shift(&PWM_program, true, false, 16); // 16 is just pull_threshold
    sm_config_set_fifo_join(&PWM_program, PIO_FIFO_JOIN_TX);

    pio_sm_init(pio, sm, offset, &PWM_program);
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


int main()
{

    stdio_init_all();
    adc_init();

    // PLL STUFF
    pll_freq = clock_get_hz(clk_sys);

    // ADC STUFF
    adc_gpio_init(VOUTSENSE); // Pin 26
    adc_select_input(0); // 0-3 is 26-29 on board
    vout_offset = adc_read();
    clock_configure(clk_adc, CLK_DEST_SYS_CLOCKS, CLOCKS_CLK_ADC_CTRL_AUXSRC_VALUE_CLKSRC_PLL_SYS, pll_freq, pll_freq);

    //PWM STUFF
    offset = pio_add_program(PWM, &PWM_program);
    PWM_setup(PWM, 0, LO, HI, PINDIRS, pinmask, offset, BASE);
    PWM_cycles pwm_cycles = PWM_target_freq(freq, pll_freq, deadtime);
    PWM_set_freq(PWM, 0, pwm_cycles.active, pwm_cycles.deadtime);


    while (true) {
        printf("Hello, world!\n");
        sleep_ms(1000);
    }
}

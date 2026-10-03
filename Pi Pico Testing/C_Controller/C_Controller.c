#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/adc.h"
#include "hardware/clocks.h"
#include "hardware/pio.h"
#include "PWM.pio.h"

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
    offset = pio_add_program(PWM, &PWM_program);
    pio_sm_config c = PWM_program_get_default_config(offset);
    sm_config_set_sideset_pins(&PWM_program, base);
    sm_config_set_out_shift(&PWM_program, true, false, 16); // 16 is just pull_threshold
    sm_config_set_fifo_join(&PWM_program, PIO_FIFO_JOIN_TX);

    pio_sm_init(pio, sm, offset, &PWM_program);
    pio_sm_set_enabled(pio, sm, true);
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

    while (true) {
        printf("Hello, world!\n");
        sleep_ms(1000);
    }
}

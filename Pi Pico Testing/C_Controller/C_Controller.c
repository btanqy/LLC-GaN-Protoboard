#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/adc.h"
#include "hardware/clocks.h"
#include "hardware/pio.h"
#include "PWM.pio.h"
#include "hardware/pll.h"
#include "hardware/vreg.h"
#include "math.h"
#include <stdlib.h>
#include "string.h"

#pragma region INIT VALUES

// PLL
static uint32_t pll_freq;

// ADC
#define VOUTSENSE 26
static int vout_offset;
#define num_samples 7
static int adc_meas[num_samples];

// PIO for PWM
static PIO PWM = pio0;
#define SM 0
#define LO 0
#define HI 1
#define PINDIRS 3 // binary 0b11
static uint32_t pinmask = 0b11;
static uint offset;
#define BASE 0

#pragma endregion

// Output and tuning
static uint freq = 400000; // for initial frequency
static float deadtime = 0.05f;
static uint64_t current_time;
#define VSET 210
#define KP 0.07
#define KI 50
static uint integral;
#define DIVIDE_ROUND(a, b) (((a) + ((b) / 2)) / (b))
#define CONTROLLER_FREQ    33000U
#define LOOP_DT            DIVIDE_ROUND(1000000U, CONTROLLER_FREQ) // timer counts once per us
#define ADCPERVOLT 4.55
#define FREQPERVOLT 2000u
#define HICLAMP 400000u
#define LOCLAMP 220000u

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
    //pio_sm_set_enabled(pio, sm, true); // enables are called later
}

static inline void PWM_set_freq(PIO pio, uint sm, uint32_t active_counter, uint32_t deadtime_counter){
    if (!pio_sm_is_tx_fifo_full(pio, sm)){
        pio_sm_put(pio, sm, (uint32_t)deadtime_counter << 20 | active_counter);
    }
}

typedef struct{
    uint32_t active;
    uint32_t deadtime;
} PWM_cycles;

PWM_cycles PWM_target_freq(uint freq, uint32_t pll_freq, float deadtime){
    // request some frequency, given pll frequency, and percentage of deadtime (0.01 = 1% deadtime)
    // minimum of 12 cycles, and an additional 2 cycles for each active and deadtime
    uint approx_cycles = (uint) (pll_freq / (2 * freq));
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

static double clamp(double value){
    if (value > HICLAMP){
        value = HICLAMP;
    }
    if (value < LOCLAMP){
        value = LOCLAMP;
    }
    return value;
}

// Pulsing

static inline void disable_sm(PIO pio, uint sm) {
    pio_sm_set_enabled(pio, sm, false);
    pio_sm_clear_fifos(pio, sm);
    pio_sm_restart(pio, sm);
    pio_sm_exec(pio, sm, pio_encode_jmp(offset));
    pio_sm_set_pins_with_mask(pio, sm, 0, pinmask);
}

static inline void enable_sm(PIO pio, uint sm, PWM_cycles counters){
    PWM_set_freq(pio, sm, counters.active, counters.deadtime); 
    pio_sm_set_enabled(pio, sm, true);
}

// Quick conversion methods

static double adc_to_vout(int adc_values){
    return adc_values / ADCPERVOLT;
}

static double vout_to_freq(double vout){
    return vout * FREQPERVOLT;
}

#pragma endregion

int main()
{
    #pragma region INITS

    vreg_set_voltage(VREG_VOLTAGE_1_15);
    sleep_ms(1000);

    // PLL STUFF
    set_sys_clock_pll(1500 * MHZ, 6, 1);
    pll_freq = clock_get_hz(clk_sys);

    // stdio_init_all(); causes this to hang without usb??
    adc_init();

    sleep_ms(10000);

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

    #pragma endregion

    // Startup first
    PWM_cycles target_freq = PWM_target_freq(HICLAMP, pll_freq, deadtime);
    // PWM_set_freq(PWM, SM, target_freq.active, target_freq.deadtime);
    // enable_sm(PWM, SM, target_freq);
    // sleep_us(20);
    uint elapsed;
    double vout;
    double error;
    integral = 0;
    int adc_mean;
    int variance;
    int adc_pruned;
    int adc_pruned_qty;

    while (true) {
        for (uint i = 0; i < 32000; i++){
            current_time = time_us_64(); // ensure accurate loop time

            // Control here

            //averaged vout
            adc_mean = 0;
            variance = 0;
            adc_pruned = 0;
            adc_pruned_qty = 0;
            memset(adc_meas, 0, num_samples * sizeof(adc_meas[0]));
            for (uint ii = 0; ii < num_samples; ii++){
                adc_meas[ii] = adc_read();
                adc_mean += adc_meas[ii];
            }
            adc_mean = (int) (adc_mean / num_samples);
            for (uint ii = 0; ii < num_samples; ii++){
                variance += ((adc_meas[ii] - adc_mean)*(adc_meas[ii] - adc_mean));
            }
            variance = (int) (variance / num_samples);
            for (uint ii = 0; ii < num_samples; ii++){
                if (abs(adc_meas[ii] - adc_mean)*abs(adc_meas[ii] - adc_mean) > 2*variance){
                    adc_meas[ii] = 0;
                    adc_pruned_qty += 1;
                }
                adc_pruned += adc_meas[ii];
            }
            if (adc_pruned_qty == num_samples){
                adc_pruned_qty = num_samples - 1;
            }
            adc_pruned = (int)(adc_pruned / (num_samples - adc_pruned_qty));

            vout = adc_to_vout(adc_pruned - vout_offset);
            error = vout_to_freq(vout - VSET);
            integral += KI * error * LOOP_DT / 1e6;
            freq = (uint) clamp(freq + (double)(KP * error) + (double)integral); // scale loop dt to seconds from microseconds
            target_freq = PWM_target_freq(freq, pll_freq, deadtime);
            PWM_set_freq(PWM, SM, target_freq.active, target_freq.deadtime);

            elapsed = time_us_64() - current_time;
            while (elapsed < LOOP_DT){
                elapsed = time_us_64() - current_time;
            }
        }
        disable_sm(PWM, SM);
        sleep_ms(5000);
        freq = 400000;
        target_freq = PWM_target_freq(freq, pll_freq, deadtime);
        enable_sm(PWM, SM, target_freq);
        sleep_us(100);
    }
}

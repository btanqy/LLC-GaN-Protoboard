#include <stdio.h>
#include <math.h>
// #include <hardware/pll.h>
#include <hardware/gpio.h>
#include <hardware/pwm.h>
#include <hardware/adc.h>

#include "pico/stdlib.h"

#define LOWSIDE 0
#define HIGHSIDE 2

static uint low_slice;
static uint low_channel;
static uint high_slice;
static uint high_channel;

int main()
{
    stdio_init_all();

    low_slice = pwm_gpio_to_slice_num (LOWSIDE);
    low_channel = pwm_gpio_to_channel (LOWSIDE);
    high_slice = pwm_gpio_to_slice_num (HIGHSIDE);
    high_channel = pwm_gpio_to_channel (HIGHSIDE);

    gpio_set_function(LOWSIDE, GPIO_FUNC_PWM);
    gpio_set_function(HIGHSIDE, GPIO_FUNC_PWM);

    pwm_set_enabled (LOWSIDE, true);	
    pwm_set_enabled (HIGHSIDE, true);


    while (true) {
        printf("Hello, world!\n");
        sleep_ms(1000);
    }
}

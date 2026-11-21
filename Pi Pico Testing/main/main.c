// Load-cell arrow-key keyboard for Raspberry Pi Pico (RP2040) - Pico SDK + TinyUSB
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/sync.h"
#include "tusb.h"

typedef struct { uint sck, out; int32_t value; } hx711_t;

// Same pins as the Python version
static hx711_t up_cell    = { .sck = 0,  .out = 1  };  // hx711_1
static hx711_t down_cell  = { .sck = 9,  .out = 8  };  // hx711_2
static hx711_t left_cell  = { .sck = 14, .out = 15 };  // hx711_3
static hx711_t right_cell = { .sck = 18, .out = 19 };  // hx711_4

// Thresholds from your spec (key is pressed when reading > threshold)
#define UP_THRESHOLD     (-1130000)
#define DOWN_THRESHOLD   (-1750000)
#define LEFT_THRESHOLD   (-880000)
#define RIGHT_THRESHOLD  (-20000)

static void hx711_init(hx711_t *h) {
    gpio_init(h->out); gpio_set_dir(h->out, GPIO_IN); gpio_pull_up(h->out);
    gpio_init(h->sck); gpio_set_dir(h->sck, GPIO_OUT); gpio_put(h->sck, 0);
}

// Non-blocking: returns true and updates h->value if a new sample was ready.
static bool hx711_read(hx711_t *h) {
    if (gpio_get(h->out)) return false;          // DOUT high = not ready

    uint32_t raw = 0;
    uint32_t irq = save_and_disable_interrupts(); // keep SCK pulses short (<60us high)
    for (int i = 0; i < 24; i++) {
        gpio_put(h->sck, 1); sleep_us(1);
        raw = (raw << 1) | gpio_get(h->out);
        gpio_put(h->sck, 0); sleep_us(1);
    }
    gpio_put(h->sck, 1); sleep_us(1);             // 25th pulse: channel A, gain 128
    gpio_put(h->sck, 0); sleep_us(1);
    restore_interrupts(irq);

    if (raw & 0x800000) raw |= 0xFF000000;        // sign-extend 24 -> 32 bit
    h->value = (int32_t)raw;
    return true;
}

int main(void) {
    hx711_init(&up_cell);   hx711_init(&down_cell);
    hx711_init(&left_cell); hx711_init(&right_cell);
    up_cell.value = down_cell.value = left_cell.value = right_cell.value = INT32_MIN;

    tusb_rhport_init_t dev_init = { .role = TUSB_ROLE_DEVICE, .speed = TUSB_SPEED_AUTO };
    tusb_init(0, &dev_init);

    uint8_t last_keys[6] = {0};
    bool first = true;

    while (true) {
        tud_task();

        hx711_read(&up_cell);
        hx711_read(&down_cell);
        hx711_read(&left_cell);
        hx711_read(&right_cell);

        // Each direction is independent, so all 4 can be held at once
        uint8_t keys[6] = {0};
        int n = 0;
        if (up_cell.value    > UP_THRESHOLD)    keys[n++] = HID_KEY_ARROW_UP;
        if (down_cell.value  > DOWN_THRESHOLD)  keys[n++] = HID_KEY_ARROW_DOWN;
        if (left_cell.value  > LEFT_THRESHOLD)  keys[n++] = HID_KEY_ARROW_LEFT;
        if (right_cell.value > RIGHT_THRESHOLD) keys[n++] = HID_KEY_ARROW_RIGHT;

        // Send a report only when the pressed set changes (an all-zero report releases keys)
        if ((first || memcmp(keys, last_keys, 6) != 0) && tud_hid_ready()) {
            tud_hid_keyboard_report(0, 0, keys);
            memcpy(last_keys, keys, 6);
            first = false;
        }
    }
}

// Required TinyUSB HID callbacks (unused)
uint16_t tud_hid_get_report_cb(uint8_t i, uint8_t id, hid_report_type_t t, uint8_t *b, uint16_t l) { (void)i;(void)id;(void)t;(void)b;(void)l; return 0; }
void tud_hid_set_report_cb(uint8_t i, uint8_t id, hid_report_type_t t, uint8_t const *b, uint16_t l) { (void)i;(void)id;(void)t;(void)b;(void)l; }
from machine import Pin, disable_irq, enable_irq
from utime import sleep, sleep_ms, sleep_us, ticks_diff, ticks_ms


DOUT_PIN = 14
SCK_PIN = 15
THRESHOLD_KG = 10.0
HYSTERESIS_KG = 0.2
CALIBRATION_MASS_KG = 1.0

# Calibrate once with a known mass, then paste the printed value here and set
# CALIBRATE_ON_STARTUP to False. Keep the sign of the value as printed.
CALIBRATE_ON_STARTUP = True
CALIBRATION_COUNTS_PER_KG = 1.0


class HX711:
    def __init__(self, dout_pin, sck_pin):
        self.dout = Pin(dout_pin, Pin.IN, Pin.PULL_UP) 
        self.sck = Pin(sck_pin, Pin.OUT, value=0)

    def read_raw(self):
        started = ticks_ms()
        while self.dout.value():
            if ticks_diff(ticks_ms(), started) > 1000:
                raise OSError("HX711 not ready; check power and DT wiring")
            sleep_ms(1)

        irq_state = disable_irq()
        value = 0
        print('hi')
        for _ in range(24):
            self.sck.value(1)
            sleep_us(1)
            value = (value << 1) | self.dout.value()
            self.sck.value(0)
            sleep_us(1)

        # One extra clock selects channel A at gain 128 for the next reading.
        self.sck.value(1)
        sleep_us(1)
        self.sck.value(0)
        enable_irq(irq_state)

        if value & 0x800000:
            value -= 0x1000000
        return value

    def read_average(self, samples=10):
        total = 0
        for _ in range(samples):
            total += self.read_raw()
        return total / samples


sensor = HX711(DOUT_PIN, SCK_PIN)
print("Remove all weight from the load cell.")
sleep(2)
zero = sensor.read_average(20)
print("Zero reading:", zero)

if CALIBRATE_ON_STARTUP:
    input("Place a known calibration mass on the cell, then press Enter: ")
    loaded = sensor.read_average(20)
    calibration_counts_per_kg = (loaded - zero) / CALIBRATION_MASS_KG
    if calibration_counts_per_kg == 0:
        raise ValueError("Calibration produced zero; check the load cell wiring")
    print("CALIBRATION_COUNTS_PER_KG =", calibration_counts_per_kg)
    input("Remove the calibration mass, then press Enter: ")
else:
    calibration_counts_per_kg = CALIBRATION_COUNTS_PER_KG
    if calibration_counts_per_kg == 0:
        raise ValueError("Set a non-zero calibration factor before running")

print("Monitoring for", THRESHOLD_KG, "kg or more.")
pressed = False

while True:
    raw = sensor.read_average(5)
    weight_kg = (raw - zero) / calibration_counts_per_kg
    print("Weight:", round(weight_kg, 2), "kg")

    if not pressed and weight_kg >= THRESHOLD_KG:
        pressed = True
        print("10 kg threshold reached:", round(weight_kg, 2), "kg")
    elif pressed and weight_kg < THRESHOLD_KG - HYSTERESIS_KG:
        pressed = False
        print("Below threshold:", round(weight_kg, 2), "kg")

    sleep(3)

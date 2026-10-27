from hx711_gpio import HX711
from machine import Pin

pin_OUT = Pin(15, Pin.IN, Pin.PULL_UP) 
pin_SCK = Pin(14, Pin.OUT, value=0)
pin_2OUT = Pin(19, Pin.IN, Pin.PULL_UP)
pin_2SCK = Pin(18, Pin.OUT, value=0)

# hx711 = HX711(pin_SCK, pin_OUT)
hx711_2 = HX711(pin_2SCK, pin_2OUT)

# hx711.tare()
hx711_2.tare()

while True:
    a = hx711_2.read()
    print(a)
from hx711_gpio import HX711
from machine import Pin

pin_OUT = Pin(14, Pin.IN, Pin.PULL_UP) 
pin_SCK = Pin(15, Pin.OUT, value=0)

hx711 = HX711(pin_SCK, pin_OUT)

hx711.tare()

while(True):
    #value = hx711.read()
    value = hx711.get_value()
    print(f"value:{value}")
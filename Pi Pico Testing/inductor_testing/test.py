from machine import mem32
from time import sleep

IO_BANK0_BASE = 0x40014000
GPIO0_CTRL = IO_BANK0_BASE + 0x004
GPIO1_CTRL = IO_BANK0_BASE + 0x00c

mem32[GPIO0_CTRL] = 4
mem32[GPIO1_CTRL] = 4

PWM_BASE = 0x40050000
CH0_CSR = PWM_BASE + 0x00
CH0_DIV = PWM_BASE + 0x04
CH0_CC = PWM_BASE + 0x0c
CH0_TOP = PWM_BASE + 0x10
# CH1_CSR = PWM_BASE + 0x14
# CH1_DIV = PWM_BASE + 0x18
# CH1_CC = PWM_BASE + 0x20

mem32[CH0_CSR] = 0b00001001
mem32[CH0_DIV] = 16 #7.8125mhz || 128ns per step
mem32[CH0_CC] = ((7812 - 39) << 16) |  (7812 + 39) #50% duty with 10us deadtime
mem32[CH0_TOP] = 15624

while True:
    sleep(1)
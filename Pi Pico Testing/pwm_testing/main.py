from machine import mem32
from time import sleep
from machine import freq

IO_BANK0_BASE = 0x40014000
GPIO0_CTRL = IO_BANK0_BASE + 0x004
GPIO1_CTRL = IO_BANK0_BASE + 0x00c
SIO_BASE = 0xd0000000
GPIO_OUT = SIO_BASE + 0x010

PLL_SYS_BASE = 0x40028000
CS = PLL_SYS_BASE + 0x0
PWR = PLL_SYS_BASE + 0x4
FBDIV_INT = PLL_SYS_BASE + 0x08
PRIM = PLL_SYS_BASE + 0xc

PWM_BASE = 0x40050000
CH0_CSR = PWM_BASE + 0x00
CH0_DIV = PWM_BASE + 0x04
CH0_CC = PWM_BASE + 0x0c
CH0_TOP = PWM_BASE + 0x10

# setting the pll
mem32[CS] = 1 << 0
mem32[PWR] &= ~((1 << 0) | (1 << 5))
mem32[FBDIV_INT] = 100 << 0 # divide reference from 16-320, 100 div is 1200MHz
mem32[PRIM] = (6 << 16) | (1 << 12) # div 6 for 200MHz overclock
while (mem32[CS] & (1 << 31)) == 0: # wait for lock
    pass

# setting the pwm
mem32[CH0_CSR] = 0b00001011
mem32[CH0_DIV] = 1 << 4 # divide pll to 200MHz resolution
mem32[CH0_CC] = ((50 + 4) << 16) | (50 - 4)  # 50% duty with 10ns deadtime = dead/(2*div*fclk)
mem32[CH0_TOP] = 100  # top = clock/(2*div*frequency)

while True:
    sleep(2)
    # turn off pwm
    mem32[GPIO0_CTRL] = 4
    mem32[GPIO1_CTRL] = 4
    mem32[CH0_CSR] |= 1 << 0
    sleep(0.01)
    # pulse pwm
    mem32[CH0_CSR] &= ~(1 << 0)
    mem32[GPIO0_CTRL] = 5
    mem32[GPIO1_CTRL] = 5
    mem32[GPIO_OUT] &= ~(0b11 << 0)
    
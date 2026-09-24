from machine import mem32
from time import sleep
from machine import freq

IO_BANK0_BASE = 0x40014000
GPIO0_CTRL = IO_BANK0_BASE + 0x004
GPIO2_CTRL = IO_BANK0_BASE + 0x014
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
CH0_CTR = PWM_BASE + 0x08
CH1_CSR = PWM_BASE + 0x14
CH1_DIV = PWM_BASE + 0x18
CH1_CC = PWM_BASE + 0x20
CH1_TOP = PWM_BASE + 0x24
CH1_CTR = PWM_BASE + 0x1c
EN = PWM_BASE + 0xa0

# setting the pll
mem32[CS] = 1 << 0
mem32[PWR] &= ~((1 << 0) | (1 << 5))
mem32[FBDIV_INT] = 100 << 0 # divide reference from 16-320, 100 div is 1200MHz
mem32[PRIM] = (6 << 16) | (1 << 12) # div 6 for 200MHz overclock
while (mem32[CS] & (1 << 31)) == 0: # wait for lock
    pass


#counter values
top = 600
half = int(top/2)
# setting the pwm slice 0
mem32[EN] = 0b00
mem32[CH0_CSR] = 0b00001001
mem32[CH0_DIV] = 1 << 4 # divide pll to 200MHz resolution
mem32[CH0_CC] = (half - 15)  # 50% duty with 50ns deadtime = dead/(div*fclk)
mem32[CH0_TOP] = top  # top = clock/(2*frequency)
# setting the pwm slice 1
mem32[CH1_CSR] = 0b00001001
mem32[CH1_DIV] = 1 << 4 # divide pll to 200MHz resolution
mem32[CH1_CC] = (half - 15)  # 50% duty with 50ns deadtime = dead/(div*fclk)
mem32[CH1_TOP] = top  # top = clock/(frequency)
mem32[CH0_CTR] = 0
mem32[CH1_CTR] = 0

while True:
    sleep(2)
    # clock stop
    mem32[EN] = 0b00
    mem32[CH0_CSR] = 0
    mem32[CH1_CSR] = 0
    
    # set pwm
    mem32[GPIO0_CTRL] = 4
    mem32[GPIO2_CTRL] = 4

    # phase shift
    mem32[CH0_CTR] = 0
    mem32[CH1_CTR] = (half + 10)
    
    # enable
    mem32[EN] = 0b11
    sleep(0.003)
    
    # off pwm
    mem32[EN] = 0b00
    mem32[CH0_CSR] &= ~(1 << 0)
    mem32[CH1_CSR] &= ~(1 << 0)
    mem32[GPIO0_CTRL] = 5
    mem32[GPIO2_CTRL] = 5
    mem32[GPIO_OUT] &= ~(0b101 << 0)
    
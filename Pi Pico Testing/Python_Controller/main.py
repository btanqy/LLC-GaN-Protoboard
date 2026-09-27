from machine import mem32
from time import sleep
from machine import freq
from machine import ADC
from time import sleep_us

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
top_value = 350
half = int(top_value/2)
# setting the pwm slice 0
mem32[EN] = 0b00
mem32[CH0_CSR] = 0b00001001
mem32[CH0_DIV] = 1 << 4 # divide pll to 200MHz resolution
mem32[CH0_CC] = (half - 15)  # 50% duty with 50ns deadtime = dead/(div*fclk)
mem32[CH0_TOP] = top_value  # top = clock/(2*frequency)
# setting the pwm slice 1
mem32[CH1_CSR] = 0b00001001
mem32[CH1_DIV] = 1 << 4 # divide pll to 200MHz resolution
mem32[CH1_CC] = (half - 15)  # 50% duty with 50ns deadtime = dead/(div*fclk)
mem32[CH1_TOP] = top_value  # top = clock/(frequency)
mem32[CH0_CTR] = 0
mem32[CH1_CTR] = 0

sleep(10)

# set pwm
mem32[GPIO0_CTRL] = 4
mem32[GPIO2_CTRL] = 4

# phase shift
mem32[CH0_CTR] = 0
mem32[CH1_CTR] = (half + 10)

# enable
mem32[EN] = 0b11

# KI = 10
KP = 1e-3

vset = 200
vset_adc = int( 65535 * (1.4 + 0.002147 * vset) / 3.3 )
vout = ADC(26)
error_memory = 0

sleep(0.05)


while True:

    for i in range(30):
        # clock stopS

        # for ii in range(20):
        #     vout_meas = vout_meas + vout.read_u16()
        #     sleep(0.0005)

        # vout_meas = int(vout_meas / 20)
        
        vout_meas = vout.read_u16()

        error = (vset_adc - vout_meas)

        top_value = int(top_value + error * KP)

        if top_value < 300:
            top_value = 300
        if top_value > 600:
            top_value = 600
        half = int(top_value/2)
        sleep_us(10)

        mem32[CH0_CC] = (half - 15)  # 50% duty with 50ns deadtime = dead/(div*fclk)
        mem32[CH0_TOP] = top_value  # top = clock/(2*frequency)
        mem32[CH1_CC] = (half - 15)  # 50% duty with 50ns deadtime = dead/(div*fclk)
        mem32[CH1_TOP] = top_value  # top = clock/(frequency)
        mem32[CH0_CTR] = 0
        mem32[CH1_CTR] = (half + 10)

        error_memory = error
        
        sleep_us(100)

    mem32[CH0_CTR] = 0
    mem32[CH1_CTR] = (half + 10)
    mem32[EN] = 0b00
    sleep(10)
    mem32[EN] = 0b11
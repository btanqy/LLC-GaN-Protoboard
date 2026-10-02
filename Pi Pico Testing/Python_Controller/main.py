from machine import mem32
from time import sleep, sleep_us
from machine import ADC

IO_BANK0_BASE = 0x40014000
GPIO0_CTRL = IO_BANK0_BASE + 0x004
GPIO2_CTRL = IO_BANK0_BASE + 0x014

PLL_SYS_BASE = 0x40028000
CS = PLL_SYS_BASE + 0x0
PWR = PLL_SYS_BASE + 0x4
FBDIV_INT = PLL_SYS_BASE + 0x08
PRIM = PLL_SYS_BASE + 0xc

PWM_BASE = 0x40050000
CH0_CSR = PWM_BASE + 0x00
CH0_DIV = PWM_BASE + 0x04
CH0_CC  = PWM_BASE + 0x0c
CH0_TOP = PWM_BASE + 0x10
CH0_CTR = PWM_BASE + 0x08
CH1_CSR = PWM_BASE + 0x14
CH1_DIV = PWM_BASE + 0x18
CH1_CC  = PWM_BASE + 0x20
CH1_TOP = PWM_BASE + 0x24
CH1_CTR = PWM_BASE + 0x1c
EN      = PWM_BASE + 0xa0

# ---- PLL setup ----
mem32[CS] = 1 << 0
mem32[PWR] &= ~((1 << 0) | (1 << 5))
mem32[FBDIV_INT] = 100 << 0
mem32[PRIM] = (6 << 16) | (1 << 12)
while (mem32[CS] & (1 << 31)) == 0:
    pass

# ---- Frequency limits (TUNE THESE to your measured resonant freq) ----
# freq = 200MHz / (top+1)
TOP_START = 400   # ~570 kHz, well above resonance for a soft start
TOP_MIN   = 300   # ~664 kHz max switching freq (light-load limit)
TOP_MAX   = 600    # ~355 kHz -- keep margin ABOVE measured resonance (350kHz)
                   # do not let this reach/cross the actual resonant point

DEADTIME  = 15     # counts, at 200MHz -> 75ns, adjust if you want 50ns (11-12 counts)
PHASE_OFF = 10      # extra counts added to CH1 offset

def apply_top(top):
    half = top // 2
    mem32[CH0_CC]  = half - DEADTIME
    mem32[CH0_TOP] = top
    mem32[CH1_CC]  = half - DEADTIME
    mem32[CH1_TOP] = top
    return half

def start_pwm(top):
    half = apply_top(top)
    mem32[CH0_CTR] = 0
    mem32[CH1_CTR] = half + PHASE_OFF   # set phase ONCE per enable
    mem32[EN] = 0b11
    sleep_us(100)

# ---- PWM slice config ----
mem32[EN] = 0b00
mem32[CH0_CSR] = 0b00001001
mem32[CH0_DIV] = 1 << 4
mem32[CH1_CSR] = 0b00001001
mem32[CH1_DIV] = 1 << 4
apply_top(TOP_START)

sleep(10)

mem32[GPIO0_CTRL] = 4
mem32[GPIO2_CTRL] = 4

top_value = TOP_START
start_pwm(top_value)

# ---- PI controller ----
KP = 0.0013/20
KI = 14/20          # start conservative; tune experimentally, see note below
LOOP_DT = 200e-6    # approx loop period (sleep_us(10)+sleep_us(100)+overhead)

vout = ADC(26)
zero_offset = vout.read_u16()
vset = 200
vset_adc = int(zero_offset + 65535 * (0.00225 * vset) / 3.3)

integral = 0.0

def clamp(x, lo, hi):
    return lo if x < lo else hi if x > hi else x


while True:
    for i in range(300):
        vout_meas = 0
        for ii in range(10):
            vout_meas += vout.read_u16() / 10
            sleep_us(1)
        error = vset_adc - int(vout_meas)

        integral_try = integral + error * LOOP_DT
        candidate = clamp(TOP_START + KP * error + KI * integral_try, TOP_MIN, TOP_MAX)
        if TOP_MIN < candidate < TOP_MAX:   # strict inequality: don't integrate further once saturated
            integral = integral_try

        top_value = int(clamp(TOP_START + KP * error + KI * integral, TOP_MIN, TOP_MAX))
        apply_top(top_value)
        sleep_us(1)

    mem32[EN] = 0b00
    sleep(10)
    start_pwm(TOP_START)   # re-apply phase offset on every restart
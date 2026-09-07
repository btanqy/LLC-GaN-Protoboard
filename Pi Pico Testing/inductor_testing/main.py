from machine import Pin
import machine

# RP2040 PWM slice 0 registers
PWM_BASE = 0x40050000
CSR = PWM_BASE + 0x00
DIV = PWM_BASE + 0x04
CTR = PWM_BASE + 0x08
CC  = PWM_BASE + 0x0C
TOP = PWM_BASE + 0x10

# GP0 = PWM slice 0 channel A
# GP1 = PWM slice 0 channel B

# 125 MHz clock / 500 Hz = 250000 counts
# Use TOP = 249999
top = 249999

# 40% = 100000 counts
duty = 100000

# 1 us at 125 MHz = 125 counts
dead = 125

# Configure GPIOs for PWM function
Pin(0, Pin.OUT)
Pin(1, Pin.OUT)

# GP0/GP1 alternate function PWM
Pin(0).init(Pin.OUT, alt=5)
Pin(1).init(Pin.OUT, alt=5)

# Stop PWM
machine.mem32[CSR] = 0

# Clock divider = 1
machine.mem32[DIV] = 1 << 4

# Period
machine.mem32[TOP] = top

# GP0:
# HIGH from counter 0 through duty
channel_a = duty

# GP1 is inverted.
# It becomes HIGH near the end of the cycle.
# Set its compare point so there is dead time after GP0.
channel_b = top - duty + dead

machine.mem32[CC] = (channel_b << 16) | channel_a

# Enable PWM, invert channel B
# CSR bits:
# bit 0 = enable
# bit 1 = B invert
machine.mem32[CSR] = (1 << 1) | 1
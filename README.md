# STM32 Automatic Headlight and Turn Signal System

An STM32 Nucleo-F401RE project that controls automatic headlights, left/right turn signals, and a buzzer.

## Features

- Automatic headlights controlled by an LDR
- Headlights turn on in darkness
- Headlights turn off in daylight
- Separate left and right turn signal LEDs
- Press once to start a turn signal
- Press the same button again to stop it
- Activating the opposite signal disables the current signal
- Active buzzer synchronized with the turn signals
- Button debounce
- Non-blocking timing using `HAL_GetTick()`
- Light hysteresis to prevent headlight flickering

## Hardware

- STM32 Nucleo-F401RE
- 1 × LDR
- 1 × 10 kΩ resistor
- 2 × yellow LEDs
- 2 × red LEDs
- 4 × 220 Ω resistors
- 2 × push buttons
- 1 × 5 V active buzzer
- 1 × BC337 transistor
- 1 × 1 kΩ resistor
- Breadboard
- Jumper wires

## Pin Configuration

| Function | STM32 Pin |
|---|---|
| LDR analog input | PA0 |
| Left headlight | PA10 |
| Right headlight | PB3 |
| Left turn signal | PB5 |
| Right turn signal | PB4 |
| Left signal button | PB10 |
| Right signal button | PA8 |
| Buzzer control | PA9 |

## LDR Connection

```text
3.3 V
  |
 LDR
  |
  +------ PA0 / ADC1_IN0
  |
10 kΩ
  |
 GND

# Arduino Connect-4 3D

## Components

- 64 bicolor red/blue common cathode LEDs
- 20 AWG tinned copper wire or similar
- 32 470 ohm or similar resistors for the LEDs
- 4 shift registers similar or equivalent to 74HC595
- 4 NPN transistors similar or equivalent to 2N2222A
- 4 1k ohm or similar resistors for the transistors
- 1 capacitor similar or equivalent to T0344, 6.3v 2200uF
- 1 5D (digital) joystick
- 1 Arduino uno

## Wiring

- LEDs are in a 4x4 cube.
  - For each LED vertical column, all red leads are joined vertically and all blue leads are joined vertically
  - For each LED horizontal layer, all LED cathodes are joined across the layer

- Shift register #1 pin 1 (Q1) to 470 ohm resistor to LED column #1 blue pin
- Shift register #1 pin 2 (Q2) to 470 ohm resistor to LED column #2 red pin
- Shift register #1 pin 3 (Q3) to 470 ohm resistor to LED column #2 blue pin
- Shift register #1 pin 4 (Q4) to 470 ohm resistor to LED column #3 red pin
- Shift register #1 pin 5 (Q5) to 470 ohm resistor to LED column #3 blue pin
- Shift register #1 pin 6 (Q6) to 470 ohm resistor to LED column #4 red pin
- Shift register #1 pin 7 (Q7) to 470 ohm resistor to LED column #4 blue pin
- Shift register #1 pin 8 (ground) to ground
- Shift register #1 pin 9 (serial out) to shift register #2 pin 14
- Shift register #1 pin 10 (MR, master reclear) to 5V
- Shift register #1 pin 11 (SH_CP, clock pin) to Arduino pin D6 + Shift register #2 pin 11
- Shift register #1 pin 12 (ST_CP, latch pin) to Arduino pin D7 + Shift register #2 pin 12
- Shift register #1 pin 13 (OE, output enable) to ground
- Shift register #1 pin 14 (DS, serial in) to Arduino pin D8
- Shift register #1 pin 15 (Q0) to 470 ohm resistor to LED column #1 red pin
- Shift register #1 pin 16 (Vcc) to 5V

- Shift register #2 pin 1 (Q1) to 470 ohm resistor to LED column #5 blue pin
- Shift register #2 pin 2 (Q2) to 470 ohm resistor to LED column #6 red pin
- Shift register #2 pin 3 (Q3) to 470 ohm resistor to LED column #6 blue pin
- Shift register #2 pin 4 (Q4) to 470 ohm resistor to LED column #7 red pin
- Shift register #2 pin 5 (Q5) to 470 ohm resistor to LED column #7 blue pin
- Shift register #2 pin 6 (Q6) to 470 ohm resistor to LED column #8 red pin
- Shift register #2 pin 7 (Q7) to 470 ohm resistor to LED column #8 blue pin
- Shift register #2 pin 8 (ground) to ground
- Shift register #2 pin 9 (serial out) to shift register #3 pin 14
- Shift register #2 pin 10 (MR, master reclear) to 5V
- Shift register #2 pin 11 (SH_CP, clock pin) to shift register #1 pin 11 + shift register #3 pin 11
- Shift register #2 pin 12 (ST_CP, latch pin) to shift register #1 pin 12 + Shift register #3 pin 12
- Shift register #2 pin 13 (OE, output enable) to ground
- Shift register #2 pin 14 (DS, serial in) to shift register #1 pin 9
- Shift register #2 pin 15 (Q0) to 470 ohm resistor to LED column #5 red pin
- Shift register #2 pin 16 (Vcc) to 5V

- Shift register #3 pin 1 (Q1) to 470 ohm resistor to LED column #9 blue pin
- Shift register #3 pin 2 (Q2) to 470 ohm resistor to LED column #10 red pin
- Shift register #3 pin 3 (Q3) to 470 ohm resistor to LED column #10 blue pin
- Shift register #3 pin 4 (Q4) to 470 ohm resistor to LED column #11 red pin
- Shift register #3 pin 5 (Q5) to 470 ohm resistor to LED column #11 blue pin
- Shift register #3 pin 6 (Q6) to 470 ohm resistor to LED column #12 red pin
- Shift register #3 pin 7 (Q7) to 470 ohm resistor to LED column #12 blue pin
- Shift register #3 pin 8 (ground) to ground
- Shift register #3 pin 9 (serial out) to shift register #4 pin 14
- Shift register #3 pin 10 (MR) to 5V
- Shift register #3 pin 11 (SH_CP, clock pin) to shift register #2 pin 11 + Shift register #4 pin 11
- Shift register #3 pin 12 (ST_CP, latch pin) to shift register #2 pin 12 + Shift register #4 pin 12
- Shift register #3 pin 13 (OE, output enable) to ground
- Shift register #3 pin 14 (DS, serial in) to shift register #2 pin 9
- Shift register #3 pin 15 (Q0) to 470 ohm resistor to LED column #9 red pin
- Shift register #3 pin 16 (Vcc) to 5V

- Shift register #4 pin 1 (Q1) to 470 ohm resistor to LED column #13 blue pin
- Shift register #4 pin 2 (Q2) to 470 ohm resistor to LED column #14 red pin
- Shift register #4 pin 3 (Q3) to 470 ohm resistor to LED column #14 blue pin
- Shift register #4 pin 4 (Q4) to 470 ohm resistor to LED column #15 red pin
- Shift register #4 pin 5 (Q5) to 470 ohm resistor to LED column #15 blue pin
- Shift register #4 pin 6 (Q6) to 470 ohm resistor to LED column #16 red pin
- Shift register #4 pin 7 (Q7) to 470 ohm resistor to LED column #16 blue pin
- Shift register #4 pin 8 (ground) to ground
- Shift register #4 pin 9 (serial out) to nothing
- Shift register #4 pin 10 (MR) to 5V
- Shift register #4 pin 11 (SH_CP, clock pin) to shift register #3 pin 11
- Shift register #4 pin 12 (ST_CP, latch pin) to shift register #3 pin 12
- Shift register #4 pin 13 (OE, output enable) to ground
- Shift register #4 pin 14 (DS, serial in) to shift register #3 pin 9
- Shift register #4 pin 15 (Q0) to 470 ohm resistor to LED column #13 red pin
- Shift register #4 pin 16 (Vcc) to 5V

- Transistor #1 emitter pin to ground
- Transistor #1 base pin to 1k ohm resistor to Arduino pin D2
- Transistor #1 collector pin to LED layer #1 pin

- Transistor #2 emitter pin to ground
- Transistor #2 base pin to 1k ohm resistor to Arduino pin D3
- Transistor #2 collector pin to LED layer #2 pin

- Transistor #3 emitter pin to ground
- Transistor #3 base pin to 1k ohm resistor to Arduino pin D4
- Transistor #3 collector pin to LED layer #3 pin

- Transistor #4 emitter pin to ground
- Transistor #4 base pin to 1k ohm resistor to Arduino pin D5
- Transistor #4 collector pin to LED layer #4 pin

- Joystick COM pin to ground
- Joystick up pin to Arduino D9
- Joystick down pin to Arduino D10
- Joystick left pin to Arduino D11
- Joystick right pin to Arduino D12
- Joystick middle pin to Arduino A3
- Joystick set pin to Arduino A2
- Joystick reset pin to Arduino A1

- Capacitor

- TODO power supply?

## Output

- All LEDs are off initially. One randomly selected LED flashes between off and red+blue (purple).
- Moving the joystick changes which LED is flashing to indicate your current position.
  You can only move within the current layer.
- Clicking the joystick places the current player's color at that position.
- If the placement causes a stretch of the opponent's color to be flanked by the current player's color (in any direction, spanning any layers), the flanked spots are cleared.
- If 4-in-a-row of a color is achieved (in any direction, spanning any layers), the 4-in-a-row(s) flashes several times and then the game resets.
- If all LEDs are lit without a 4-in-a-row of any color, the whole board flashes alternating red/blue and then the game resets.
- If the game is not over, a new random LED flashes and the second player takes their turn.

Demo:

Todo

## Potential future additions todo

- Make winner an enum (and possibly board values too)
- Look into using direct port writes instead of shiftOut

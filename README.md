# Arduino Connect-4 3D

## Components

- 64 bicolor red/blue common cathode LEDs
- 32 470 ohm or similar resistors for the LEDs
- 4 shift registers similar or equivalent to 74HC595
- 4 NPN transistors similar or equivalent to 2A222 todo check part number
- 4 todo ohm or similar resistors for the transistors
- 1 capacitor similar or equivalent to todo get part number
- 1 5D joystick
- 1 Arduino uno

## Wiring

- Joystick pin VRx to Arduino pin A0
- Joystick pin VRy to Arduino pin A1
- Joystick pin SW to Arduino pin D7
- Joystick pin 5V to 5V
- Joystick pin ground to ground

- Shift register #1 pin 1 (Q1) to 470 ohm resistor to LED #1 blue pin
- Shift register #1 pin 2 (Q2) to 470 ohm resistor to LED #2 red pin
- Shift register #1 pin 3 (Q3) to 470 ohm resistor to LED #2 blue pin
- Shift register #1 pin 4 (Q4) to 470 ohm resistor to LED #3 red pin
- Shift register #1 pin 5 (Q5) to 470 ohm resistor to LED #3 blue pin
- Shift register #1 pin 6 (Q6) to 470 ohm resistor to LED #4 red pin
- Shift register #1 pin 7 (Q7) to 470 ohm resistor to LED #4 blue pin
- Shift register #1 pin 8 (ground) to ground
- Shift register #1 pin 9 (Serial out) to Shift register #2 pin 14
- Shift register #1 pin 10 (MR) to 5V
- Shift register #1 pin 11 (SH_CP, clock pin) to Arduino pin D12 + Shift register #2 pin 11
- Shift register #1 pin 12 (ST_CP, latch pin) to Arduino pin D8 + Shift register #2 pin 12
- Shift register #1 pin 13 (OE) to ground
- Shift register #1 pin 14 (DS, serial in) to Arduino pin D11
- Shift register #1 pin 15 (Q0) to 470 ohm resistor to LED #1 red pin
- Shift register #1 pin 16 (Vcc) to 5V

- Shift register #2 pin 1 (Q1) to 470 ohm resistor to LED #5 blue pin
- Shift register #2 pin 2 (Q2) to 470 ohm resistor to LED #6 red pin
- Shift register #2 pin 3 (Q3) to 470 ohm resistor to LED #6 blue pin
- Shift register #2 pin 4 (Q4) to 470 ohm resistor to LED #7 red pin
- Shift register #2 pin 5 (Q5) to 470 ohm resistor to LED #7 blue pin
- Shift register #2 pin 6 (Q6) to 470 ohm resistor to LED #8 red pin
- Shift register #2 pin 7 (Q7) to 470 ohm resistor to LED #8 blue pin
- Shift register #2 pin 8 (ground) to ground
- Shift register #2 pin 9 (Serial out) to Shift register #3 pin 14
- Shift register #2 pin 10 (MR) to 5V
- Shift register #2 pin 11 (SH_CP, clock pin) to Shift register #1 pin 11 + Shift register #3 pin 11
- Shift register #2 pin 12 (ST_CP, latch pin) to Shift register #1 pin 12 + Shift register #3 pin 12
- Shift register #2 pin 13 (OE) to ground
- Shift register #2 pin 14 (DS, serial in) to Shift register #1 pin 9
- Shift register #2 pin 15 (Q0) to 470 ohm resistor to LED #5 red pin
- Shift register #2 pin 16 (Vcc) to 5V

- Shift register #3 pin 1 (Q1) to 470 ohm resistor to LED #9 blue pin
- Shift register #3 pin 8 (ground) to ground
- Shift register #3 pin 10 (MR) to 5V
- Shift register #3 pin 11 (SH_CP, clock pin) to Shift register #2 pin 11
- Shift register #3 pin 12 (ST_CP, latch pin) to Shift register #2 pin 12
- Shift register #3 pin 13 (OE) to ground
- Shift register #3 pin 14 (DS, serial in) to Shift register #2 pin 9
- Shift register #3 pin 15 (Q0) to 470 ohm resistor to LED #9 red pin
- Shift register #3 pin 16 (Vcc) to 5V

- All LED cathodes to ground
- LEDs should be in a 9x9 grid. The code assumes this layout:

  Column 1 | Column 2 | Column 3
  --- | --- | ---
  LED #1 | LED #2 | LED #3
  LED #4 | LED #5 | LED #6
  LED #7 | LED #8 | LED #9

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

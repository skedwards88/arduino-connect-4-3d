#include "Arduino.h"

// Pins for the first shift register
const int LATCH_PIN = 8;
const int CLOCK_PIN = 12;
const int DATA_PIN = 11;

// Pins for the joystick
const int X_PIN = A0;
const int Y_PIN = A1;
const int BUTTON_PIN = 7;

void setup()
{
  pinMode(LATCH_PIN, OUTPUT);
  pinMode(CLOCK_PIN, OUTPUT);
  pinMode(DATA_PIN, OUTPUT);
  pinMode(X_PIN, INPUT);
  pinMode(Y_PIN, INPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
}

void loop()
{
}

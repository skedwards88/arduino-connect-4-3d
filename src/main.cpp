#include "Arduino.h"

// Pins for the first shift register
const int LATCH_PIN = 8;
const int CLOCK_PIN = 12;
const int DATA_PIN = 11;

// Pins for the layers
const int NUM_LAYERS = 4;
const int LAYER0_PIN = 2;
const int LAYER1_PIN = 3;
const int LAYER2_PIN = 4;
const int LAYER3_PIN = 5;
const int layerPins[NUM_LAYERS] = {LAYER0_PIN, LAYER1_PIN, LAYER2_PIN, LAYER3_PIN};

// Duration that each layer is turned on
// Note this is microseconds, not milliseconds
const unsigned int LAYER_ON_TIME_US = 800;

// Pins for the joystick
const int X_PIN = A0;
const int Y_PIN = A1;
const int BUTTON_PIN = 7;

struct GameState
{
  // Board representation
  // 4 layers of 16 states
  // State 0 = empty, 1 = Player 1, 2 = Player 2
  uint8_t board[4][16];

  // Layer on the board (0..3) that is currently active
  uint8_t activeLayer;

  // Position on the layer (0..15) that is currently active
  uint8_t cursorPosition;

  // Blink state to indicate cursor position
  bool blinkIsOn;
  unsigned long lastBlinkMs = 0;

  bool isPlayer1Turn;

  // 0 = none, 1 = X, 2 = O, 3 = stalemate
  uint8_t winner;
};

static GameState gameState;

void initializeGameState(GameState &gameState)
{
  for (int l = 0; l < 4; l++)
  {
    for (int i = 0; i < 16; i++)
    {
      gameState.board[l][i] = 0;
    }
  }

  // random is min inclusive, max exclusive
  gameState.activeLayer = random(0, 4);
  gameState.cursorPosition = random(0, 16);
  gameState.blinkIsOn = false;
  gameState.lastBlinkMs = 0;
  gameState.isPlayer1Turn = true;
  gameState.winner = 0;
}

void updateCursorPosition(GameState &gameState)
{
  // Joystick movement classification
  static const int LEFT_THRESHOLD = 400;
  static const int RIGHT_THRESHOLD = 800;
  static const int UP_THRESHOLD = 400;
  static const int DOWN_THRESHOLD = 800;

  // Prevent the joystick from moving too fast
  static const unsigned long JOYSTICK_LIMITER_MS = 300;
  static unsigned long lastMoveMs = 0;

  int xValue = analogRead(X_PIN);
  int yValue = analogRead(Y_PIN);

  unsigned long now = millis();

  if (now - lastMoveMs >= JOYSTICK_LIMITER_MS)
  {
    const int oldColumn = gameState.cursorPosition % 4;
    const int oldRow = gameState.cursorPosition / 4; // C++ automatically rounds down for integer division, so no need for something like Math.floor()

    int newColumn = oldColumn;
    int newRow = oldRow;

    if (xValue < LEFT_THRESHOLD)
    {
      // moving left
      newColumn = max(0, oldColumn - 1);
    }
    else if (xValue > RIGHT_THRESHOLD)
    {
      // moving right
      newColumn = min(3, oldColumn + 1);
    }

    if (yValue < UP_THRESHOLD)
    {
      // moving up
      newRow = max(0, oldRow - 1);
    }
    else if (yValue > DOWN_THRESHOLD)
    {
      // moving down
      newRow = min(3, oldRow + 1);
    }

    gameState.cursorPosition = (newRow * 4) + newColumn;

    lastMoveMs = now;
  }
}

uint8_t getValueAtXYZ(const uint8_t board[4][16], uint8_t x, uint8_t y, uint8_t z)
{
  return board[z][y * 4 + x];
}

uint8_t getStreakLength(const uint8_t board[4][16], uint8_t x, uint8_t y, uint8_t z, int dx, int dy, int dz, uint8_t player)
{
  // Not counting the starting position as part of the streak,
  // since will call this for both directions,
  // which would result in the start position being double counted
  uint8_t streakLength = 0;

  for (uint8_t step = 1; step < 4; step++)
  {
    int x2 = x + (dx * step);
    int y2 = y + (dy * step);
    int z2 = z + (dz * step);

    // stop if outside of cube
    if (x2 < 0 || y2 < 0 || z2 < 0 || x2 >= 4 || y2 >= 4 || z2 >= 4)
      break;

    uint8_t value = getValueAtXYZ(board, x2, y2, z2);

    // stop if different color
    if (value != player)
      break;

    streakLength++;
  }

  return streakLength;
}

uint8_t checkForGameOver(const uint8_t board[4][16], uint8_t lastLayer, uint8_t lastPosition)
{
  // Don't need to scan for ALL possible 4 in a rows,
  // just need to check if the last played spot makes a 4 in a row
  // These are the possible 4-in-a-row directions
  // (consider a line with between each of these coordinates and
  // the coordinate {0,0,0} representing the player's location)
  static const int DIRECTIONS[13][3] = {
      // Vary just x, y, or z
      {1, 0, 0},
      {0, 1, 0},
      {0, 0, 1},
      // Vary xy, xz, or yz
      {1, 1, 0},
      {1, 0, 1},
      {0, 1, 1},
      {1, -1, 0},
      {1, 0, -1},
      {0, 1, -1},
      // Vary xyz
      {1, 1, 1},
      {1, -1, 1},
      {1, 1, -1},
      {-1, 1, 1}};

  uint8_t x = lastPosition % 4;
  uint8_t y = lastPosition / 4; // C++ automatically rounds down for integer division, so no need for something like Math.floor()
  uint8_t z = lastLayer;
  uint8_t player = getValueAtXYZ(board, x, y, z);

  for (uint8_t d = 0; d < 13; d++)
  {
    int dx = DIRECTIONS[d][0];
    int dy = DIRECTIONS[d][1];
    int dz = DIRECTIONS[d][2];
    uint8_t streakLength = 1 + getStreakLength(board, x, y, z, dx, dy, dz, player) + getStreakLength(board, x, y, z, -dx, -dy, -dz, player);

    if (streakLength == 4)
      return player;
  }
  // todo check for stalemate and return 3 if so. stalemate = board full. any other conditions?

  return 0; // no winner
}

void updateBoard(GameState &gameState)
{
  static int lastButtonValue = HIGH;

  int buttonValue = digitalRead(BUTTON_PIN);

  if (buttonValue == LOW && lastButtonValue == HIGH && gameState.board[gameState.activeLayer][gameState.cursorPosition] == 0)
  {
    // Update board
    gameState.board[gameState.activeLayer][gameState.cursorPosition] = gameState.isPlayer1Turn ? 1 : 2;

    // Check for game over (win or stalemate)
    gameState.winner = checkForGameOver(gameState.board, gameState.activeLayer, gameState.cursorPosition);

    // Switch players
    gameState.isPlayer1Turn = !gameState.isPlayer1Turn;

    // Choose a new layer and position
    // todo may want to make it so can't choose the current layer or a non-empty position. Definitely want to make it so that the layer chosen has at least 1 legal move. Could maybe select for any empty position, to bias towards levels with more empty spots.
    gameState.activeLayer = random(0, 4);
    gameState.cursorPosition = random(0, 16);
  }

  lastButtonValue = buttonValue;
}

void disableAllLayers()
{
  for (int i = 0; i < NUM_LAYERS; i++)
  {
    digitalWrite(layerPins[i], LOW);
  }
}

void layerToBytes(uint8_t layer[16], uint8_t cursorPosition, bool blinkIsOn, uint8_t outBytes[4])
{
  // To send to the 4 shift registers that control the LEDs
  outBytes[0] = outBytes[1] = outBytes[2] = outBytes[3] = 0;

  for (uint8_t i = 0; i < 16; i++)
  {
    // State 0 = empty, 1 = Player 1, 2 = Player 2
    bool color1IsOn = (layer[i] == 1);
    bool color2IsOn = (layer[i] == 2);

    // If blinking, both are on regardless
    if (i == cursorPosition && blinkIsOn)
    {
      color1IsOn = true;
      color2IsOn = true;
    }

    // On the board, the bicolor LEDs are wired to the shift registers in alternating color order
    // (LED 1 color 1 pin, LED 1 color 2 pin, LED 2 color 1 pin, ...)
    // so color 1 pins are even and color2 pins are odd
    uint8_t color1BitIndex = i * 2;     // 0, 2, ...30
    uint8_t color2BitIndex = i * 2 + 1; // 1, 3, ...31

    // Use |= instead of = to write just that bit in the byte
    if (color1IsOn)
    {
      outBytes[color1BitIndex / 8] |= (1u << (color1BitIndex % 8));
    }
    if (color2IsOn)
    {
      outBytes[color2BitIndex / 8] |= (1u << (color2BitIndex % 8));
    }
  }
}

void renderLEDsForLayer(uint8_t layerIndex, uint8_t bytesToRender[4])
{
  disableAllLayers();

  digitalWrite(LATCH_PIN, LOW);

  shiftOut(DATA_PIN, CLOCK_PIN, MSBFIRST, bytesToRender[3]); // shift register 3
  shiftOut(DATA_PIN, CLOCK_PIN, MSBFIRST, bytesToRender[2]); // shift register 3
  shiftOut(DATA_PIN, CLOCK_PIN, MSBFIRST, bytesToRender[1]); // shift register 2
  shiftOut(DATA_PIN, CLOCK_PIN, MSBFIRST, bytesToRender[0]); // shift register 1

  digitalWrite(LATCH_PIN, HIGH);

  digitalWrite(layerPins[layerIndex], HIGH);
}

// Refreshes one layer when called
// Should call as often as possible
// todo should probably separate out the logic from the actual rendering
// e.g. currentLayer and incrementation, renderLEDsForLayer, delayMicroseconds gets called fast and frequent, but the blink stuff and layerToBytes stuff doesn't
void renderGame(GameState &gameState)
{
  static const unsigned long BLINK_PERIOD_MS = 200;

  unsigned long now = millis();

  if (now - gameState.lastBlinkMs >= BLINK_PERIOD_MS)
  {
    gameState.blinkIsOn = !gameState.blinkIsOn;
    gameState.lastBlinkMs = now;
  }

  static int currentLayer = 0;

  bool blinkForThisLayer = (currentLayer == gameState.activeLayer) && gameState.blinkIsOn;

  uint8_t layerBytes[4];

  layerToBytes(gameState.board[currentLayer], gameState.cursorPosition, blinkForThisLayer, layerBytes);

  renderLEDsForLayer(currentLayer, layerBytes);

  // Hold it briefly
  delayMicroseconds(LAYER_ON_TIME_US);

  // Next layer
  currentLayer++;
  if (currentLayer >= NUM_LAYERS)
    currentLayer = 0;
}

void setup()
{
  pinMode(LATCH_PIN, OUTPUT);
  pinMode(CLOCK_PIN, OUTPUT);
  pinMode(DATA_PIN, OUTPUT);
  pinMode(X_PIN, INPUT);
  pinMode(Y_PIN, INPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  for (int i = 0; i < NUM_LAYERS; i++)
  {
    pinMode(layerPins[i], OUTPUT);
  }
  disableAllLayers();

  initializeGameState(gameState);
}

void loop()
{
  updateCursorPosition(gameState);

  updateBoard(gameState);

  if (gameState.winner == 3 || gameState.winner == 1 || gameState.winner == 2)
  {

    // Give a visual indication that the game is over
    // renderGameOver(gameState); // todo add back

    // Reinitialize the variables for a new game
    initializeGameState(gameState);
  }
  else
  {
    // no winner yet; render the board
    renderGame(gameState);
  }
}

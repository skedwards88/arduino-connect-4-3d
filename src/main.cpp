#include "Arduino.h"

// Pins for the first shift register
const int LATCH_PIN = 10;
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

// Minimum elapsed time between recording joystick input
const unsigned long JOYSTICK_DEBOUNCE_MS = 50;

// Pins for the joystick
const int UP_PIN = 9;
const int DOWN_PIN = 8;
const int LEFT_PIN = 7;
const int RIGHT_PIN = 6;
const int BUTTON_PIN = A1;

enum GameStatus : uint8_t
{
  IN_PROGRESS = 0,
  WON = 1,
  STALEMATE = 2
};

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

  // winMask[z], bits 0..15 correspond to board[z][i]
  // Corresponds to the LEDs that are part of the winning 4-in-a-row(s)
  uint16_t winMask[4];

  GameStatus status;

  // The game freezes at the end to display the win/stalemate animation
  unsigned long frozenUntilMs;

  // Cache the bytes to display for each layer
  // since the bytes are rendered much more frequently than they change
  uint8_t cachedLayerBytes[4][4];
  bool cacheIsDirty[4];
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
    for (int b = 0; b < 4; b++)
    {
      gameState.cachedLayerBytes[l][b] = 0;
    }
    gameState.winMask[l] = 0;
    gameState.cacheIsDirty[l] = true;
  }

  // initialize the pseudo-random number generator
  randomSeed(analogRead(A4)); // Note: this should use a floating/unused pin

  // random is min inclusive, max exclusive
  gameState.activeLayer = random(0, 4);
  gameState.cursorPosition = random(0, 16);
  gameState.blinkIsOn = false;
  gameState.lastBlinkMs = 0;
  gameState.isPlayer1Turn = true;
  gameState.status = IN_PROGRESS;
  gameState.frozenUntilMs = 0;
}

void updateCursorPosition(GameState &gameState)
{
  static unsigned long lastMoveCheckMs = 0;
  static int lastLeft = HIGH;
  static int lastRight = HIGH;
  static int lastUp = HIGH;
  static int lastDown = HIGH;

  unsigned long now = millis();

  if (now - lastMoveCheckMs >= JOYSTICK_DEBOUNCE_MS)
  {
    lastMoveCheckMs = now;

    int currentLeft = digitalRead(LEFT_PIN);
    int currentRight = digitalRead(RIGHT_PIN);
    int currentUp = digitalRead(UP_PIN);
    int currentDown = digitalRead(DOWN_PIN);

    const int oldColumn = gameState.cursorPosition % 4;
    const int oldRow = gameState.cursorPosition / 4; // C++ automatically rounds down for integer division, so no need for something like Math.floor()

    int newColumn = oldColumn;
    int newRow = oldRow;

    if (currentLeft == LOW && lastLeft == HIGH)
    {
      // moving left
      newColumn = max(0, oldColumn - 1);
    }
    else if (currentRight == LOW && lastRight == HIGH)
    {
      // moving right
      newColumn = min(3, oldColumn + 1);
    }
    else if (currentUp == LOW && lastUp == HIGH)
    {
      // moving up
      newRow = max(0, oldRow - 1);
    }
    else if (currentDown == LOW && lastDown == HIGH)
    {
      // moving down
      newRow = min(3, oldRow + 1);
    }

    if (newRow != oldRow || newColumn != oldColumn)
    {
      gameState.cursorPosition = (newRow * 4) + newColumn;
      gameState.cacheIsDirty[gameState.activeLayer] = true;
    }

    lastLeft = currentLeft;
    lastRight = currentRight;
    lastUp = currentUp;
    lastDown = currentDown;
  }
}

uint8_t getValueAtXYZ(const uint8_t board[4][16], uint8_t x, uint8_t y, uint8_t z)
{
  return board[z][y * 4 + x];
}

void freezeGame(GameState &gameState)
{
  // for game over/stalemate, freeze gameplay for a few seconds to render the game over sequence
  gameState.frozenUntilMs = millis() + 3000;
}

// Return 0,1,2 to indicate the number of opponent pieces that are flanked in the given dxyz direction
uint8_t getNumFlanked(const uint8_t board[4][16], uint8_t x, uint8_t y, uint8_t z, int dx, int dy, int dz, uint8_t player)
{
  uint8_t opponentStreak = 0;
  bool isFlanked = false;
  uint8_t opponent = (player == 1) ? 2 : 1;

  for (uint8_t step = 1; step < 4; step++)
  {
    int x2 = x + (dx * step);
    int y2 = y + (dy * step);
    int z2 = z + (dz * step);

    // stop if outside of cube
    if (x2 < 0 || y2 < 0 || z2 < 0 || x2 >= 4 || y2 >= 4 || z2 >= 4)
    {
      break;
    }

    uint8_t value = getValueAtXYZ(board, x2, y2, z2);

    if (value == opponent)
    {
      opponentStreak++;
    }
    // stop if same color as player, but toggle isFlanked
    else if (value == player)
    {
      isFlanked = true;
      break;
    }
    else
    {
      // stop if empty spot (or for any other case)
      break;
    }
  }
  return isFlanked ? opponentStreak : 0;
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

bool isStalemate(const uint8_t board[4][16])
{
  for (uint8_t layer = 0; layer < 4; layer++)
  {
    for (uint8_t i = 0; i < 16; i++)
    {
      if (board[layer][i] == 0)
        return false;
    }
  }
  return true;
}

void updatePotentialGameOver(GameState &gameState)
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

  uint8_t x = gameState.cursorPosition % 4;
  uint8_t y = gameState.cursorPosition / 4; // C++ automatically rounds down for integer division, so no need for something like Math.floor()
  uint8_t z = gameState.activeLayer;
  uint8_t player = getValueAtXYZ(gameState.board, x, y, z);

  bool foundWin = false;

  for (uint8_t d = 0; d < 13; d++)
  {
    int dx = DIRECTIONS[d][0];
    int dy = DIRECTIONS[d][1];
    int dz = DIRECTIONS[d][2];
    uint8_t positiveStreak = getStreakLength(gameState.board, x, y, z, dx, dy, dz, player);
    uint8_t negativeStreak = getStreakLength(gameState.board, x, y, z, -dx, -dy, -dz, player);

    uint8_t streakLength = 1 + positiveStreak + negativeStreak;

    if (streakLength == 4)
    {
      int startX = x - dx * negativeStreak;
      int startY = y - dy * negativeStreak;
      int startZ = z - dz * negativeStreak;

      for (int step = 0; step < 4; step++)
      {
        int currentX = startX + dx * step;
        int currentY = startY + dy * step;
        int currentZ = startZ + dz * step;
        int currentIndex = currentY * 4 + currentX;

        gameState.winMask[currentZ] |= (1u << (currentIndex));
      }
      foundWin = true;
    }

    uint8_t positiveNumFlanked = getNumFlanked(gameState.board, x, y, z, dx, dy, dz, player);
    uint8_t negativeNumFlanked = getNumFlanked(gameState.board, x, y, z, -dx, -dy, -dz, player);
    for (uint8_t step = 1; step <= positiveNumFlanked; step++)
    {
      int currentX = x + dx * step;
      int currentY = y + dy * step;
      int currentZ = z + dz * step;
      int currentIndex = currentY * 4 + currentX;

      gameState.board[currentZ][currentIndex] = 0;
      gameState.cacheIsDirty[currentZ] = true;
    }
    for (int step = 1; step <= negativeNumFlanked; step++)
    {
      int currentX = x - dx * step;
      int currentY = y - dy * step;
      int currentZ = z - dz * step;
      int currentIndex = currentY * 4 + currentX;

      gameState.board[currentZ][currentIndex] = 0;
      gameState.cacheIsDirty[currentZ] = true;
    }
  }

  if (foundWin)
  {
    gameState.status = WON;
    for (int l = 0; l < 4; l++)
    {
      gameState.cacheIsDirty[l] = true;
    }
    freezeGame(gameState);
  }
  else if (isStalemate(gameState.board))
  {
    gameState.status = STALEMATE;
    for (int l = 0; l < 4; l++)
    {
      gameState.cacheIsDirty[l] = true;
    }
    freezeGame(gameState);
  }
  else
  {
    gameState.status = IN_PROGRESS;
  }
}

uint8_t selectRandomEmptyIndex(const uint8_t board[4][16])
{
  uint8_t start = random(0, 64);

  for (uint8_t offset = 0; offset < 64; offset++)
  {
    uint8_t index = (start + offset) % 64;

    uint8_t layer = index / 16;
    uint8_t position = index % 16;

    if (board[layer][position] == 0)
    {
      return index;
    }
  }

  // technically should never reach here since this should never be called if the board is full (stalemate)
  return 0;
}

void updateBoard(GameState &gameState)
{
  static int lastButtonValue = HIGH;
  static unsigned long lastPressMs = 0;

  unsigned long now = millis();

  int buttonValue = digitalRead(BUTTON_PIN);

  if (buttonValue == LOW && lastButtonValue == HIGH && gameState.board[gameState.activeLayer][gameState.cursorPosition] == 0 && (now - lastPressMs) > JOYSTICK_DEBOUNCE_MS)
  {
    lastPressMs = now;

    // Update board
    gameState.board[gameState.activeLayer][gameState.cursorPosition] = gameState.isPlayer1Turn ? 1 : 2;
    gameState.cacheIsDirty[gameState.activeLayer] = true;

    // Check for game over (win or stalemate)
    updatePotentialGameOver(gameState);

    if (gameState.status == IN_PROGRESS)
    {
      // Switch players
      gameState.isPlayer1Turn = !gameState.isPlayer1Turn;

      // Choose a new layer and position
      uint8_t nextIndex = selectRandomEmptyIndex(gameState.board);
      gameState.activeLayer = nextIndex / 16;
      gameState.cursorPosition = nextIndex % 16;
    }
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

void setBytes(uint8_t position, bool color1IsOn, bool color2IsOn, uint8_t outBytes[4])
{
  // On the board, the bicolor LEDs are wired to the shift registers in alternating color order
  // (LED 1 color 1 pin, LED 1 color 2 pin, LED 2 color 1 pin, ...)
  // so color 1 pins are even and color2 pins are odd
  uint8_t color1BitIndex = position * 2;     // 0, 2, ...30
  uint8_t color2BitIndex = position * 2 + 1; // 1, 3, ...31

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

void renderLEDsForLayer(uint8_t layerIndex, uint8_t bytesToRender[4])
{
  disableAllLayers();

  digitalWrite(LATCH_PIN, LOW);

  shiftOut(DATA_PIN, CLOCK_PIN, MSBFIRST, bytesToRender[3]); // shift register 4
  shiftOut(DATA_PIN, CLOCK_PIN, MSBFIRST, bytesToRender[2]); // shift register 3
  shiftOut(DATA_PIN, CLOCK_PIN, MSBFIRST, bytesToRender[1]); // shift register 2
  shiftOut(DATA_PIN, CLOCK_PIN, MSBFIRST, bytesToRender[0]); // shift register 1

  digitalWrite(LATCH_PIN, HIGH);

  digitalWrite(layerPins[layerIndex], HIGH);
}

void updateBlink(GameState &gameState)
{
  static const unsigned long BLINK_PERIOD_MS = 200;

  unsigned long now = millis();

  if (now - gameState.lastBlinkMs >= BLINK_PERIOD_MS)
  {
    gameState.blinkIsOn = !gameState.blinkIsOn;
    gameState.lastBlinkMs = now;
    if (gameState.status == IN_PROGRESS)
    {
      gameState.cacheIsDirty[gameState.activeLayer] = true;
    }
    else
    {
      for (uint8_t l = 0; l < 4; l++)
      {
        gameState.cacheIsDirty[l] = true;
      }
    }
  }
}

// Refreshes one layer when called
// Should call as often as possible
void renderGame(GameState &gameState)
{
  static int currentLayer = 0;

  if (gameState.cacheIsDirty[currentLayer])
  {
    // clear the cached bytes
    for (uint8_t i = 0; i < 4; i++)
    {
      gameState.cachedLayerBytes[currentLayer][i] = 0;
    }

    bool blinkForThisLayer = (currentLayer == gameState.activeLayer) && gameState.blinkIsOn;

    for (uint8_t i = 0; i < 16; i++)
    {
      // State 0 = empty, 1 = Player 1, 2 = Player 2
      bool color1IsOn = (gameState.board[currentLayer][i] == 1);
      bool color2IsOn = (gameState.board[currentLayer][i] == 2);

      // If blinking, both are on regardless
      if (i == gameState.cursorPosition && blinkForThisLayer)
      {
        color1IsOn = true;
        color2IsOn = true;
      }

      setBytes(i, color1IsOn, color2IsOn, gameState.cachedLayerBytes[currentLayer]);
    }

    gameState.cacheIsDirty[currentLayer] = false;
  }

  renderLEDsForLayer(currentLayer, gameState.cachedLayerBytes[currentLayer]);

  // Hold it briefly
  delayMicroseconds(LAYER_ON_TIME_US);

  // Next layer
  currentLayer++;
  if (currentLayer >= NUM_LAYERS)
    currentLayer = 0;
}

void renderGameOver(GameState &gameState)
{
  static int currentLayer = 0;

  if (gameState.cacheIsDirty[currentLayer])
  {
    // clear the cached bytes
    for (uint8_t i = 0; i < 4; i++)
    {
      gameState.cachedLayerBytes[currentLayer][i] = 0;
    }

    uint16_t winMask = gameState.winMask[currentLayer];

    for (uint8_t i = 0; i < 16; i++)
    {
      bool highlight = winMask & (1u << i);
      if (highlight || !gameState.blinkIsOn)
      {
        // State 0 = empty, 1 = Player 1, 2 = Player 2
        bool color1IsOn = (gameState.board[currentLayer][i] == 1);
        bool color2IsOn = (gameState.board[currentLayer][i] == 2);

        setBytes(i, color1IsOn, color2IsOn, gameState.cachedLayerBytes[currentLayer]);
      }
    }

    gameState.cacheIsDirty[currentLayer] = false;
  }

  renderLEDsForLayer(currentLayer, gameState.cachedLayerBytes[currentLayer]);

  // Hold it briefly
  delayMicroseconds(LAYER_ON_TIME_US);

  // Next layer
  currentLayer++;
  if (currentLayer >= NUM_LAYERS)
    currentLayer = 0;
}

void renderStalemate(GameState &gameState)
{
  static int currentLayer = 0;

  if (gameState.cacheIsDirty[currentLayer])
  {
    // clear the cached bytes
    for (uint8_t i = 0; i < 4; i++)
    {
      gameState.cachedLayerBytes[currentLayer][i] = 0;
    }

    if (gameState.blinkIsOn)
    {
      for (uint8_t i = 0; i < 16; i++)
      {
        // State 0 = empty, 1 = Player 1, 2 = Player 2
        bool color1IsOn = (gameState.board[currentLayer][i] == 1);
        bool color2IsOn = (gameState.board[currentLayer][i] == 2);

        setBytes(i, color1IsOn, color2IsOn, gameState.cachedLayerBytes[currentLayer]);
      }
    }

    gameState.cacheIsDirty[currentLayer] = false;
  }

  renderLEDsForLayer(currentLayer, gameState.cachedLayerBytes[currentLayer]);

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
  pinMode(UP_PIN, INPUT_PULLUP);
  pinMode(DOWN_PIN, INPUT_PULLUP);
  pinMode(LEFT_PIN, INPUT_PULLUP);
  pinMode(RIGHT_PIN, INPUT_PULLUP);
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
  const unsigned long now = millis();

  updateBlink(gameState);

  // If frozen for game over, just render and return
  if (now < gameState.frozenUntilMs)
  {
    if (gameState.status == WON)
    {
      // Give a visual indication that the game is over
      renderGameOver(gameState);
    }
    else if (gameState.status == STALEMATE)
    {
      renderStalemate(gameState);
    }
  }
  else
  {
    if (gameState.status == WON || gameState.status == STALEMATE)
    {
      // Reinitialize the variables for a new game
      initializeGameState(gameState);
    }
    else
    {
      // game not over; keep playing and render the board

      updateCursorPosition(gameState);

      updateBoard(gameState);

      renderGame(gameState);
    }
  }
}

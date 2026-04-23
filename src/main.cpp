#include "Arduino.h"

const int GRID_DIMENSION = 4;
const int NUM_LAYERS = GRID_DIMENSION;
const int NUM_POSITIONS = 16;      // Positions (bicolor LEDs) per layer
const int NUM_SHIFT_REGISTERS = 4; // 16 bicolor LEDs = 32 leads; 8 leads controlled by 1 shift register

// Pins for the first shift register
const int CLOCK_PIN = 6;
const int LATCH_PIN = 7;
const int DATA_PIN = 8;

// Pins for the layers
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
const int UP_PIN = A2;
const int DOWN_PIN = A1;
const int LEFT_PIN = 12;
const int RIGHT_PIN = A3;
const int BUTTON_PIN = 11;
const int SET_PIN = 10;
const int RESET_PIN = 9;

// Don't need to scan for ALL possible 4 in a rows,
// just need to check if the last played spot makes a 4 in a row.
// These are the possible 4-in-a-row directions
// (consider a line with between each of these coordinates and
// the coordinate {0,0,0} representing the player's location).
// Ditto for checking for captured spots -- just need to follow
// the direction forward and backwards from the placed piece.
const int DIRECTIONS[13][3] = {
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

enum GameStatus : uint8_t
{
  IN_PROGRESS = 0,
  WON = 1,
  STALEMATE = 2
};

struct GameState
{
  // Board representation
  // State 0 = empty, 1 = Player 1, 2 = Player 2
  uint8_t board[NUM_LAYERS][NUM_POSITIONS];

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
  uint16_t winMask[NUM_LAYERS];

  GameStatus status;

  // The game freezes at the end to display the win/stalemate animation
  unsigned long frozenUntilMs;

  // Cache the bytes to display for each layer
  // since the bytes are rendered much more frequently than they change
  uint8_t cachedLayerBytes[NUM_LAYERS][NUM_SHIFT_REGISTERS];
  bool cacheIsDirty[NUM_LAYERS];
};

static GameState gameState;

void initializeGameState(GameState &gameState)
{
  for (int layer = 0; layer < NUM_LAYERS; layer++)
  {
    for (int position = 0; position < NUM_POSITIONS; position++)
    {
      gameState.board[layer][position] = 0;
    }
    for (int b = 0; b < NUM_SHIFT_REGISTERS; b++)
    {
      gameState.cachedLayerBytes[layer][b] = 0;
    }
    gameState.winMask[layer] = 0;
    gameState.cacheIsDirty[layer] = true;
  }

  // random is min inclusive, max exclusive
  gameState.activeLayer = random(0, NUM_LAYERS);
  gameState.cursorPosition = random(0, NUM_POSITIONS);
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

    const int oldColumn = gameState.cursorPosition % GRID_DIMENSION;
    const int oldRow = gameState.cursorPosition / GRID_DIMENSION; // C++ automatically rounds down for integer division, so no need for something like Math.floor()

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
      newColumn = min(GRID_DIMENSION - 1, oldColumn + 1);
    }
    else if (currentUp == LOW && lastUp == HIGH)
    {
      // moving up
      newRow = max(0, oldRow - 1);
    }
    else if (currentDown == LOW && lastDown == HIGH)
    {
      // moving down
      newRow = min(GRID_DIMENSION - 1, oldRow + 1);
    }

    if (newRow != oldRow || newColumn != oldColumn)
    {
      gameState.cursorPosition = (newRow * GRID_DIMENSION) + newColumn;
      gameState.cacheIsDirty[gameState.activeLayer] = true;
    }

    lastLeft = currentLeft;
    lastRight = currentRight;
    lastUp = currentUp;
    lastDown = currentDown;
  }
}

uint8_t getValueAtXYZ(const uint8_t board[NUM_LAYERS][NUM_POSITIONS], int x, int y, int z)
{
  return board[z][y * GRID_DIMENSION + x];
}

void freezeGame(GameState &gameState)
{
  // for game over/stalemate, freeze gameplay for a few seconds to render the game over sequence
  gameState.frozenUntilMs = millis() + 3000;
}

// Return 0,1,2 to indicate the number of opponent pieces that are flanked in the given dxyz direction
uint8_t getNumFlanked(const uint8_t board[NUM_LAYERS][NUM_POSITIONS], int x, int y, int z, int dx, int dy, int dz, uint8_t player)
{
  uint8_t opponentStreak = 0;
  bool isFlanked = false;
  uint8_t opponent = (player == 1) ? 2 : 1;

  for (uint8_t step = 1; step < GRID_DIMENSION; step++)
  {
    int x2 = x + (dx * step);
    int y2 = y + (dy * step);
    int z2 = z + (dz * step);

    // stop if outside of cube
    if (x2 < 0 || y2 < 0 || z2 < 0 || x2 >= GRID_DIMENSION || y2 >= GRID_DIMENSION || z2 >= GRID_DIMENSION)
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

uint8_t getStreakLength(const uint8_t board[NUM_LAYERS][NUM_POSITIONS], int x, int y, int z, int dx, int dy, int dz, uint8_t player)
{
  // Not counting the starting position as part of the streak,
  // since will call this for both directions,
  // which would result in the start position being double counted
  uint8_t streakLength = 0;

  for (uint8_t step = 1; step < GRID_DIMENSION; step++)
  {
    int x2 = x + (dx * step);
    int y2 = y + (dy * step);
    int z2 = z + (dz * step);

    // stop if outside of cube
    if (x2 < 0 || y2 < 0 || z2 < 0 || x2 >= GRID_DIMENSION || y2 >= GRID_DIMENSION || z2 >= GRID_DIMENSION)
    {
      break;
    }

    uint8_t value = getValueAtXYZ(board, x2, y2, z2);

    // stop if different color
    if (value != player)
    {
      break;
    }

    streakLength++;
  }

  return streakLength;
}

bool isStalemate(const uint8_t board[NUM_LAYERS][NUM_POSITIONS])
{
  for (uint8_t layer = 0; layer < NUM_LAYERS; layer++)
  {
    for (uint8_t position = 0; position < NUM_POSITIONS; position++)
    {
      if (board[layer][position] == 0)
      {
        return false;
      }
    }
  }
  return true;
}

void updatePotentialCaptures(GameState &gameState)
{
  const int x = gameState.cursorPosition % GRID_DIMENSION;
  const int y = gameState.cursorPosition / GRID_DIMENSION; // C++ automatically rounds down for integer division, so no need for something like Math.floor()
  const int z = gameState.activeLayer;
  uint8_t player = getValueAtXYZ(gameState.board, x, y, z);

  uint16_t capturedMask[NUM_LAYERS] = {0};

  for (uint8_t d = 0; d < 13; d++)
  {
    const int dx = DIRECTIONS[d][0];
    const int dy = DIRECTIONS[d][1];
    const int dz = DIRECTIONS[d][2];

    // Flanked pieces can be either forwards or backwards from the placed piece
    uint8_t positiveNumFlanked = getNumFlanked(gameState.board, x, y, z, dx, dy, dz, player);
    uint8_t negativeNumFlanked = getNumFlanked(gameState.board, x, y, z, -dx, -dy, -dz, player);

    for (uint8_t step = 1; step <= positiveNumFlanked; step++)
    {
      int currentX = x + dx * step;
      int currentY = y + dy * step;
      int currentZ = z + dz * step;
      int currentPosition = currentY * GRID_DIMENSION + currentX;

      capturedMask[currentZ] |= (uint16_t)(1u << currentPosition);
    }

    for (uint8_t step = 1; step <= negativeNumFlanked; step++)
    {
      int currentX = x - dx * step;
      int currentY = y - dy * step;
      int currentZ = z - dz * step;
      int currentPosition = currentY * GRID_DIMENSION + currentX;

      capturedMask[currentZ] |= (uint16_t)(1u << currentPosition);
    }
  }

  for (uint8_t layer = 0; layer < NUM_LAYERS; layer++)
  {
    if (capturedMask[layer] == 0)
    {
      continue;
    }

    for (uint8_t position = 0; position < NUM_POSITIONS; position++)
    {
      if (capturedMask[layer] & (uint16_t)(1u << position))
      {
        gameState.board[layer][position] = 0;
      }
    }
    gameState.cacheIsDirty[layer] = true;
  }
}

void updatePotentialGameOver(GameState &gameState)
{
  int x = gameState.cursorPosition % GRID_DIMENSION;
  int y = gameState.cursorPosition / GRID_DIMENSION; // C++ automatically rounds down for integer division, so no need for something like Math.floor()
  int z = gameState.activeLayer;
  uint8_t player = getValueAtXYZ(gameState.board, x, y, z);

  bool foundWin = false;

  for (uint8_t d = 0; d < 13; d++)
  {
    int dx = DIRECTIONS[d][0];
    int dy = DIRECTIONS[d][1];
    int dz = DIRECTIONS[d][2];

    // Check the streak from both the forwards and backwards direction from the piece
    uint8_t positiveStreak = getStreakLength(gameState.board, x, y, z, dx, dy, dz, player);
    uint8_t negativeStreak = getStreakLength(gameState.board, x, y, z, -dx, -dy, -dz, player);

    uint8_t streakLength = 1 + positiveStreak + negativeStreak;

    if (streakLength == GRID_DIMENSION)
    {
      int startX = x - dx * negativeStreak;
      int startY = y - dy * negativeStreak;
      int startZ = z - dz * negativeStreak;

      for (int step = 0; step < GRID_DIMENSION; step++)
      {
        int currentX = startX + dx * step;
        int currentY = startY + dy * step;
        int currentZ = startZ + dz * step;
        int currentPosition = currentY * GRID_DIMENSION + currentX;

        gameState.winMask[currentZ] |= (1u << (currentPosition));
      }
      foundWin = true;
    }
  }

  if (foundWin)
  {
    gameState.status = WON;
    for (int layer = 0; layer < NUM_LAYERS; layer++)
    {
      gameState.cacheIsDirty[layer] = true;
    }
    freezeGame(gameState);
  }
  else if (isStalemate(gameState.board))
  {
    gameState.status = STALEMATE;
    for (int layer = 0; layer < NUM_LAYERS; layer++)
    {
      gameState.cacheIsDirty[layer] = true;
    }
    freezeGame(gameState);
  }
  else
  {
    gameState.status = IN_PROGRESS;
  }
}

uint8_t selectRandomEmptyIndex(const uint8_t board[NUM_LAYERS][NUM_POSITIONS])
{
  int totalNumPositions = NUM_LAYERS * NUM_POSITIONS;
  uint8_t start = random(0, totalNumPositions);

  for (uint8_t offset = 0; offset < totalNumPositions; offset++)
  {
    uint8_t index = (start + offset) % totalNumPositions;

    uint8_t layer = index / NUM_POSITIONS;
    uint8_t position = index % NUM_POSITIONS;

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

    // Record the placement on the board
    gameState.board[gameState.activeLayer][gameState.cursorPosition] = gameState.isPlayer1Turn ? 1 : 2;
    gameState.cacheIsDirty[gameState.activeLayer] = true;

    // Record captures
    updatePotentialCaptures(gameState);

    // Check for game over (win or stalemate)
    updatePotentialGameOver(gameState);

    if (gameState.status == IN_PROGRESS)
    {
      // Switch players
      gameState.isPlayer1Turn = !gameState.isPlayer1Turn;

      // Choose a new layer and position
      uint8_t nextIndex = selectRandomEmptyIndex(gameState.board);
      gameState.activeLayer = nextIndex / NUM_POSITIONS;
      gameState.cursorPosition = nextIndex % NUM_POSITIONS;
      gameState.cacheIsDirty[gameState.activeLayer] = true;
    }
  }
  lastButtonValue = buttonValue;
}

void disableAllLayers()
{
  for (int layer = 0; layer < NUM_LAYERS; layer++)
  {
    digitalWrite(layerPins[layer], LOW);
  }
}

void setBytes(uint8_t position, bool color1IsOn, bool color2IsOn, uint8_t outBytes[NUM_SHIFT_REGISTERS])
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

void renderLEDsForLayer(uint8_t layerIndex, uint8_t bytesToRender[NUM_SHIFT_REGISTERS])
{
  disableAllLayers();

  digitalWrite(LATCH_PIN, LOW);

  for (uint8_t b = 0; b < NUM_SHIFT_REGISTERS; b++)
  {
    shiftOut(DATA_PIN, CLOCK_PIN, MSBFIRST, bytesToRender[b]);
  }

  digitalWrite(LATCH_PIN, HIGH);

  digitalWrite(layerPins[layerIndex], HIGH);
}

void updateBlink(GameState &gameState)
{
  static const unsigned long BLINK_PERIOD_MS = 350;

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
      for (uint8_t layer = 0; layer < NUM_LAYERS; layer++)
      {
        gameState.cacheIsDirty[layer] = true;
      }
    }
  }
}

// Refreshes one layer when called
// Should call as often as possible
void renderGame(GameState &gameState)
{
  static uint8_t currentLayer = 0;

  if (gameState.cacheIsDirty[currentLayer])
  {
    // clear the cached bytes
    for (uint8_t b = 0; b < NUM_SHIFT_REGISTERS; b++)
    {
      gameState.cachedLayerBytes[currentLayer][b] = 0;
    }

    for (uint8_t position = 0; position < NUM_POSITIONS; position++)
    {
      bool color1IsOn = false;
      bool color2IsOn = false;

      if (gameState.status == IN_PROGRESS)
      {
        // For the cursor position,
        // blink on = player color
        // blink off = bicolor if space is occupied, off otherwise
        if (position == gameState.cursorPosition && currentLayer == gameState.activeLayer)
        {
          if (gameState.blinkIsOn)
          {
            color1IsOn = gameState.isPlayer1Turn;
            color2IsOn = !gameState.isPlayer1Turn;
          }
          else
          {
            if (gameState.board[currentLayer][position] != 0)
            {
              color1IsOn = true;
              color2IsOn = true;
            }
          }
        }
        // For non-cursor position, just show what has been placed
        else
        {
          color1IsOn = (gameState.board[currentLayer][position] == 1);
          color2IsOn = (gameState.board[currentLayer][position] == 2);
        }
      }
      else if (gameState.status == WON)
      {
        // When blink is on, only show winning 4-in-a-row(s)
        // When blink is off, show the full board
        bool highlight = (gameState.winMask[currentLayer] & (1u << position)) != 0;
        if (highlight || !gameState.blinkIsOn)
        {
          color1IsOn = (gameState.board[currentLayer][position] == 1);
          color2IsOn = (gameState.board[currentLayer][position] == 2);
        }
      }
      else if (gameState.status == STALEMATE)
      {
        // When blink is on, show the full board
        // Otherwise, everything is off
        if (gameState.blinkIsOn)
        {
          color1IsOn = (gameState.board[currentLayer][position] == 1);
          color2IsOn = (gameState.board[currentLayer][position] == 2);
        }
      }

      setBytes(position, color1IsOn, color2IsOn, gameState.cachedLayerBytes[currentLayer]);
    }

    gameState.cacheIsDirty[currentLayer] = false;
  }

  renderLEDsForLayer(currentLayer, gameState.cachedLayerBytes[currentLayer]);

  // Hold it briefly
  delayMicroseconds(LAYER_ON_TIME_US);

  // Next layer
  currentLayer++;
  if (currentLayer >= NUM_LAYERS)
  {
    currentLayer = 0;
  }
}

void setup()
{
  // initialize the pseudo-random number generator
  randomSeed(analogRead(A4)); // Note: this should use a floating/unused pin

  pinMode(LATCH_PIN, OUTPUT);
  pinMode(CLOCK_PIN, OUTPUT);
  pinMode(DATA_PIN, OUTPUT);
  pinMode(UP_PIN, INPUT_PULLUP);
  pinMode(DOWN_PIN, INPUT_PULLUP);
  pinMode(LEFT_PIN, INPUT_PULLUP);
  pinMode(RIGHT_PIN, INPUT_PULLUP);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  for (int layer = 0; layer < NUM_LAYERS; layer++)
  {
    pinMode(layerPins[layer], OUTPUT);
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
    renderGame(gameState);
    return;
  }

  // If not frozen and game is over, reinitialize and return
  // (This happens right after the game over freeze ends)
  if (gameState.status == WON || gameState.status == STALEMATE)
  {
    initializeGameState(gameState);
    return;
  }

  updateCursorPosition(gameState);
  updateBoard(gameState);
  renderGame(gameState);
}

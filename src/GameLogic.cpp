#include <stdint.h>
#include "GameConfig.h"

uint8_t getValueAtXYZ(const uint8_t board[NUM_LAYERS][NUM_POSITIONS], int x, int y, int z)
{
  return board[z][y * GRID_DIMENSION + x];
}

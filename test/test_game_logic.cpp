#include <unity.h>
#include "GameLogic.h"

void test_getValueAtXYZ_reads_correct_cell() {
  uint8_t board[4][16] = {0};
  // z=2, x=1, y=3 => index y*4+x = 13
  board[2][13] = 2;

  TEST_ASSERT_EQUAL_UINT8(2, getValueAtXYZ(board, 1, 3, 2));
  TEST_ASSERT_EQUAL_UINT8(0, getValueAtXYZ(board, 0, 0, 0));
}

int main(int argc, char **argv) {
  UNITY_BEGIN();
  RUN_TEST(test_getValueAtXYZ_reads_correct_cell);
  return UNITY_END();
}

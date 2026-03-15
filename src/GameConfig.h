#pragma once

constexpr int GRID_DIMENSION = 4;
constexpr int NUM_LAYERS = GRID_DIMENSION;
constexpr int NUM_POSITIONS = 16;      // Positions (bicolor LEDs) per layer
constexpr int NUM_SHIFT_REGISTERS = 4; // 16 bicolor LEDs = 32 leads; 8 leads controlled by 1 shift register

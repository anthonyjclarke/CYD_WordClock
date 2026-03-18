#pragma once
// wordclock.h — letter grid, word-map, time-to-words logic
// Architecture ported from Brett Oliver wordclock_v4_9 (Arduino/MAX7219)
// Same {row, col, length} PROGMEM word descriptor pattern, same showTimeWords() logic.

#include <Arduino.h>
#include "config.h"

// ── Frame buffer ─────────────────────────────────────────────────────────────
// Mirrors Brett's frame2[] — true = cell is lit
extern bool gridLit[GRID_ROWS][GRID_COLS];

// ── Frame operations (mirrors Brett's display.cpp API) ────────────────────────
// Clear the frame (all cells dim)
void clearFrame();

// Light up a word in the frame. theword[3] = { row, col, len } in PROGMEM.
void addWordToFrame(const byte theword[3]);

// Render gridLit[][] to gridSprite. Call pushGrid() after to send to screen.
void renderGrid();

// ── Time logic (mirrors Brett's time.cpp showTimeWords()) ─────────────────────
// Compute which words to light for the given 24h hour and minute.
// Calls clearFrame() + addWordToFrame() internally, then renderGrid().
void showTimeWords(uint8_t h, uint8_t m);

// ── Touch ─────────────────────────────────────────────────────────────────────
void initTouch();

// Poll touch hardware. Handles long-press brightness cycle.
void handleTouch();

// ── Main loop tick ────────────────────────────────────────────────────────────
// Call from loop(). Handles time check, conditional redraw, touch, LDR.
void wordclockTick();

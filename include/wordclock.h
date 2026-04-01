#pragma once
// wordclock.h — letter grid, word-map, time-to-words logic
// Ported from Brett Oliver's Word Clock (wordclock_v4_9, Arduino/MAX7219)
// https://www.brettoliver.org.uk/Word_Clock/Word_Clock.htm
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

// Access the static letter grid row text for external renderers such as the WebUI.
const char* getGridRowText(uint8_t row);

// ── Time logic (mirrors Brett's time.cpp showTimeWords()) ─────────────────────
// Compute which words to light for the given 24h hour and minute.
// Calls clearFrame() + addWordToFrame() internally, then renderGrid().
void showTimeWords(uint8_t h, uint8_t m);

// ── Touch ─────────────────────────────────────────────────────────────────────
void initTouch();

// Poll touch hardware. Handles long-press brightness cycle.
void handleTouch();
void updateTouchRotation(uint8_t rotation);

// ── Main loop tick ────────────────────────────────────────────────────────────
// Call from loop(). Handles time check, conditional redraw, touch, LDR.
void wordclockTick();
void redrawClockNow();

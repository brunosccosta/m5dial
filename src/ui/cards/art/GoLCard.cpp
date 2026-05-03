#include "GoLCard.h"
#include <Arduino.h>
#include <esp_log.h>
#include <esp_random.h>
#include <string.h>

static const char* TAG = "GOL";

// ---------------------------------------------------------------- lifecycle --

void GoLCard::onShow() {
    buildPalette();
    precomputeClip();
    seed();
    _genTick = 0;
}

void GoLCard::onHide() {
    _genCount = 0;
}

void GoLCard::onTick() {
    _genTick++;
    if (_genTick >= _genInterval) {
        _genTick = 0;
        stepGeneration();
    }
    drawGrid();
    invalidateCanvas();
}

// ----------------------------------------------------------------- palette ---

void GoLCard::buildPalette() {
    _alivePalette[0] = 0x0000;
    for (int age = 1; age <= 255; age++) {
        float t = (float)(age - 1) / 254.0f;
        float hue, sat, val;
        if (t < 0.25f) {
            float u = t / 0.25f;
            hue = 60.0f; sat = u * 0.8f; val = 1.0f;
        } else if (t < 0.6f) {
            float u = (t - 0.25f) / 0.35f;
            hue = 60.0f - u * 50.0f;
            sat = 0.8f + u * 0.2f;
            val = 1.0f - u * 0.1f;
        } else {
            float u = (t - 0.6f) / 0.4f;
            hue = 10.0f + u * 230.0f;
            sat = 1.0f;
            val = 0.9f - u * 0.5f;
        }
        _alivePalette[age] = hsv565(hue, sat, val);
    }
    _trailPalette[0] = 0x0000;
    for (int i = 1; i <= TRAIL_LEN; i++) {
        float v = (float)i / TRAIL_LEN * 0.25f;
        _trailPalette[i] = hsv565(220.0f, 0.7f, v);
    }
}

// ------------------------------------------------------------------- clip ---

void GoLCard::precomputeClip() {
    for (int gy = 0; gy < ROWS; gy++) {
        for (int gx = 0; gx < COLS; gx++) {
            int px = gx * CELL, py = gy * CELL;
            int inside = 0;
            for (int cy = 0; cy <= CELL - 1; cy += (CELL - 1)) {
                for (int cx = 0; cx <= CELL - 1; cx += (CELL - 1)) {
                    float dx = (px + cx) - 119.5f;
                    float dy = (py + cy) - 119.5f;
                    if (dx*dx + dy*dy <= 120.0f * 120.0f) inside++;
                }
            }
            _cellClip[gy * COLS + gx] = (inside == 0) ? 0 : (inside == 4) ? 1 : 2;
        }
    }
}

// -------------------------------------------------------------------- seed --

void GoLCard::seed() {
    memset(_grid,  0, NCELLS);
    memset(_trail, 0, NCELLS);
    int cx = COLS / 2, cy = ROWS / 2;
    for (int dy = -10; dy < 10; dy++) {
        for (int dx = -10; dx < 10; dx++) {
            int gx = cx + dx, gy = cy + dy;
            if (gx >= 0 && gx < COLS && gy >= 0 && gy < ROWS)
                _grid[gy * COLS + gx] = (esp_random() & 1) ? 1 : 0;
        }
    }
    _genCount = 0;
    ESP_LOGI(TAG, "seeded");
}

// --------------------------------------------------------------- GoL step ---

int GoLCard::countNeighbors(int gx, int gy) {
    int n = 0;
    for (int dy = -1; dy <= 1; dy++) {
        for (int dx = -1; dx <= 1; dx++) {
            if (dx == 0 && dy == 0) continue;
            int nx = (gx + dx + COLS) % COLS;
            int ny = (gy + dy + ROWS) % ROWS;
            if (_grid[ny * COLS + nx] > 0) n++;
        }
    }
    return n;
}

void GoLCard::stepGeneration() {
    int alive = 0;
    for (int gy = 0; gy < ROWS; gy++) {
        for (int gx = 0; gx < COLS; gx++) {
            int  idx      = gy * COLS + gx;
            bool wasAlive = _grid[idx] > 0;
            int  n        = countNeighbors(gx, gy);
            bool nowAlive = wasAlive ? (n == 2 || n == 3) : (n == 3);

            _scratch[idx] = nowAlive ? (_grid[idx] < 255 ? _grid[idx] + 1 : 255) : 0;
            if (nowAlive) alive++;

            if (wasAlive && !nowAlive)
                _trail[idx] = TRAIL_LEN;
            else if (!wasAlive && !nowAlive && _trail[idx] > 0)
                _trail[idx]--;
        }
    }
    memcpy(_grid, _scratch, NCELLS);
    _genCount++;
    if (alive < 30) seed();
}

// ------------------------------------------------------------------- draw ---

void GoLCard::drawGrid() {
    memset(pixBuf, 0, W * H * 2);
    for (int gy = 0; gy < ROWS; gy++) {
        for (int gx = 0; gx < COLS; gx++) {
            int idx = gy * COLS + gx;
            uint8_t clip = _cellClip[idx];
            if (clip == 0) continue;

            uint16_t color;
            if      (_grid[idx]  > 0) color = _alivePalette[_grid[idx]];
            else if (_trail[idx] > 0) color = _trailPalette[_trail[idx]];
            else continue;

            int px = gx * CELL, py = gy * CELL;
            if (clip == 1) {
                for (int dy = 0; dy < CELL; dy++) {
                    uint16_t* row = pixBuf + (py + dy) * W + px;
                    for (int dx = 0; dx < CELL; dx++) row[dx] = color;
                }
            } else {
                for (int dy = 0; dy < CELL; dy++) {
                    for (int dx = 0; dx < CELL; dx++) {
                        float ddx = (px + dx) - 119.5f;
                        float ddy = (py + dy) - 119.5f;
                        if (ddx*ddx + ddy*ddy <= 120.0f * 120.0f)
                            pixBuf[(py + dy) * W + (px + dx)] = color;
                    }
                }
            }
        }
    }
}

#pragma once
#include "MathArtCard.h"

class GoLCard : public MathArtCard {
protected:
    void onShow() override;
    void onTick() override;
    void onHide() override;

private:
    static const int COLS = 48, ROWS = 48, NCELLS = COLS * ROWS;
    static const int CELL = 5;
    static const uint8_t TRAIL_LEN = 8;

    uint8_t _grid[NCELLS]     = {};
    uint8_t _trail[NCELLS]    = {};
    uint8_t _scratch[NCELLS]  = {};
    uint8_t _cellClip[NCELLS] = {};

    uint16_t _alivePalette[256] = {};
    uint16_t _trailPalette[16]  = {};

    int      _genInterval = 6;
    int      _genTick     = 0;
    uint32_t _genCount    = 0;

    void buildPalette();
    void precomputeClip();
    void seed();
    void stepGeneration();
    void drawGrid();

    int countNeighbors(int gx, int gy);
};

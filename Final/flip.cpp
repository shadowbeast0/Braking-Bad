// flip.cpp
#include "flip.h"
#include <QtMath>
#include <algorithm>

static inline int gx_from_px(int px){
    return px / Constants::PIXEL_SIZE;
}

static inline int gy_from_px(int py){
    return py / Constants::PIXEL_SIZE;
}

void FlipTracker::update(double angleRad, double carX, double carY, double nowSec, const std::function<void(int)>& onAward)
{
    if (!m_init) {
        m_init = true;
        m_lastAng = angleRad;
        m_accum = 0.0;
        return;
    }

    double d = angleRad - m_lastAng;
    while (d >  M_PI) d -= TWO_PI;
    while (d <= -M_PI) d += TWO_PI;

    m_accum += d;
    m_lastAng = angleRad;

    while (m_accum >= TWO_PI) {
        ++m_ccw; m_accum -= TWO_PI;
        if (onAward) onAward(COINS_PER_FLIP);
        m_popups.append({int(carX), int(carY - POPUP_OFFSET_CELLS * Constants::PIXEL_SIZE),
                         nowSec + POPUP_LIFETIME});
    }
    while (m_accum <= -TWO_PI) {
        ++m_cw; m_accum += TWO_PI;
        if (onAward) onAward(COINS_PER_FLIP);
        m_popups.append({int(carX), int(carY - POPUP_OFFSET_CELLS * Constants::PIXEL_SIZE),
                         nowSec + POPUP_LIFETIME});
    }

    for (int i = 0; i < m_popups.size();) {
        if (m_popups[i].until <= nowSec) m_popups.removeAt(i);
        else ++i;
    }
}


void FlipTracker::drawHUD(QPainter& p, int levelIndex) const
{
    const int textGX = Constants::HUD_LEFT_MARGIN + 1;
    const int nitroBaselineGY = Constants::HUD_TOP_MARGIN + Constants::COIN_RADIUS_CELLS*2 + 4;
    const int extraGapCells = 10;
    const int textGY = nitroBaselineGY + 7 + extraGapCells;
    QFont f; f.setFamily("Monospace"); f.setBold(true); f.setPointSize(12);
    p.setFont(f);
    p.setPen(Constants::TEXT_COLOR[levelIndex]);
    p.drawText(textGX * Constants::PIXEL_SIZE,
               textGY  * Constants::PIXEL_SIZE,
               QString("Flips: %1").arg(total()));
}


// === Popup ===
void FlipTracker::drawPixelWordFlip(QPainter& p, int gx, int gy, int cell, const QColor& c)
{
    auto plot=[&](int x,int y){ p.fillRect((gx+x)*cell,(gy+y)*cell,cell,cell,c); };
    static const uint8_t F[7]={0x1F,0x10,0x1E,0x10,0x10,0x10,0x10};
    static const uint8_t l[7]={0x04,0x04,0x04,0x04,0x04,0x04,0x06};
    static const uint8_t i[7]={0x00,0x08,0x00,0x18,0x08,0x08,0x1C};
    static const uint8_t glyphP[7]={0x00,0x00,0x1C,0x12,0x1C,0x10,0x10};
    static const uint8_t ex[7]={0x04,0x04,0x04,0x04,0x04,0x00,0x04};

    auto drawChar=[&](const uint8_t rows[7], int ox){
        for(int ry=0; ry<7; ++ry){
            uint8_t row=rows[ry];
            for(int rx=0; rx<5; ++rx)
                if (row & (1<<(4-rx))) plot(ox+rx, ry);
        }
    };

    int adv=6;
    drawChar(F, 0);
    drawChar(l, adv*1);
    drawChar(i, adv*2);
    drawChar(glyphP, adv*3);
    drawChar(ex,adv*4);
}

void FlipTracker::drawWorldPopups(QPainter& p, int cameraX, int cameraY, int level_index) const
{
    const int cell = Constants::PIXEL_SIZE;
    const int screenPadCells = 10;

    for (const auto& pop : m_popups) {
        const int gx = gx_from_px(pop.wx - cameraX) + screenPadCells;
        const int gy = gy_from_px(pop.wy + cameraY);
        drawPixelWordFlip(p, gx, gy, cell, Constants::FLIP_COLOR[level_index]);
    }
}

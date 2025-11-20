// flip.h
#pragma once
#include <QPainter>
#include <QList>
#include <QString>
#include <functional>
#include "constants.h"

class FlipTracker {
public:
    void reset() {
        m_init = false;
        m_lastAng = 0.0;
        m_accum = 0.0;
        m_cw = m_ccw = 0;
        m_popups.clear();
    }

    void update(double angleRad, double carX, double carY, double nowSec, const std::function<void(int)>& onAward);
    void drawHUD(QPainter& p, int levelIndex) const;
    void drawWorldPopups(QPainter& p, int cameraX, int cameraY, int level_index) const;

    int total() const { return m_cw + m_ccw; }
    int cw()    const { return m_cw; }
    int ccw()   const { return m_ccw; }

private:
    static constexpr double TWO_PI = 6.283185307179586;
    static constexpr int    COINS_PER_FLIP     = 50;
    static constexpr double POPUP_LIFETIME     = 1.5; // seconds
    static constexpr int    POPUP_OFFSET_CELLS = 10;

    struct Popup {
        int wx, wy;
        double until;
    };

    static void drawPixelWordFlip(QPainter& p, int gx, int gy, int cell, const QColor& c);

    bool   m_init = false;
    double m_lastAng = 0.0;
    double m_accum   = 0.0;
    int    m_cw = 0, m_ccw = 0;
    QList<Popup> m_popups;
};

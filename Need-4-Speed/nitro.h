#ifndef NITRO_H
#define NITRO_H

#include <QPainter>
#include <QColor>
#include <QList>
#include <cmath>
#include <functional>
#include "constants.h"
#include "wheel.h"

class NitroSystem {
public:
    bool   active = false;
    double endTime = 0.0;
    double cooldownUntil = 0.0;
    double dirX = 0.0;
    double dirY = 1.0;
    int    ceilY = -1000000000;

    void update(
        bool nitroKey,
        double fuel,
        double elapsedSeconds,
        double avgX,
        const std::function<int(int)>& groundGyNearestGX,
        const std::function<double(double)>& terrainTangentAngleAtX
        );

    // Use the *previous* thrust direction (first two wheels) and clamp behavior
    void applyThrust(QList<Wheel*>& wheels) const;

    // Keep the *previous* pixel HUD (rocket icon + countdown)
    void drawHUD(QPainter& p, double elapsedSeconds, int levelIndex) const;

    // Keep the *previous* nitro flame look (based on first/back wheel and first front)
    void drawFlame(QPainter& p, const QList<Wheel*>& wheels, int cameraX, int cameraY, int viewW, int viewH) const;
};

#endif // NITRO_H

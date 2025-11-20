#include "fuel.h"
#include <cmath>
#include <algorithm>

int FuelSystem::currentFuelSpacing(double difficulty, double elapsedSeconds) const {
    int base    = 700;
    int byDiff  = int(500 * (difficulty / 0.005));
    int byTime  = int(35 * elapsedSeconds);
    int spacing = base + byDiff + byTime;
    spacing     = int(std::round(spacing * Constants::FUEL_SPAWN_EASE));

    int minSpacing = int(std::round(10000 * Constants::FUEL_SPAWN_EASE));
    int maxSpacing = int(std::round(200000 * Constants::FUEL_SPAWN_EASE));
    if (spacing < minSpacing) spacing = minSpacing;
    if (spacing > maxSpacing) spacing = maxSpacing;

    return spacing;
}

void FuelSystem::maybePlaceFuelAtEdge(
    int lastTerrainX,
    const QHash<int,int>& heightAtGX,
    double difficulty,
    double elapsedSeconds
    ) {
    if (lastTerrainX - lastPlacedFuelX < currentFuelSpacing(difficulty, elapsedSeconds)) return;

    int gx = lastTerrainX / Constants::PIXEL_SIZE;
    auto it = heightAtGX.constFind(gx);
    if (it == heightAtGX.constEnd()) return;

    const int gyGround = it.value();

    FuelCan f;
    f.wx = lastTerrainX;
    f.wy = (gyGround - Constants::FUEL_FLOOR_OFFSET_CELLS) * Constants::PIXEL_SIZE;
    f.taken = false;
    cans.append(f);

    lastPlacedFuelX = lastTerrainX;
}

void FuelSystem::drawWorldFuel(QPainter& p, int cameraX, int cameraY) const {
    const int camGX = cameraX / Constants::PIXEL_SIZE;
    const int camGY = cameraY / Constants::PIXEL_SIZE;

    QColor body(230, 60, 60);
    QColor cap(230,230,230);
    QColor label(255,200,50);
    QColor shadow(0,0,0,90);

    auto plotGridPixel = [&](int gx, int gy, const QColor& c) {
        p.fillRect(gx * Constants::PIXEL_SIZE,
                   gy * Constants::PIXEL_SIZE,
                   Constants::PIXEL_SIZE,
                   Constants::PIXEL_SIZE, c);
    };

    for (const FuelCan& f : cans) {
        if (f.taken) continue;

        int sx = (f.wx / Constants::PIXEL_SIZE) - camGX;
        int sy = (f.wy / Constants::PIXEL_SIZE) + camGY;

        auto px = [&](int gx, int gy, const QColor& c){
            plotGridPixel(sx+gx, sy+gy, c);
        };

        px(2,1,shadow);
        px(5,2,shadow);

        px(1,0,cap);
        px(2,0,cap);

        for (int y=1; y<=5; ++y) {
            for (int x=0; x<=4; ++x) {
                px(x,y,body);
            }
        }

        for (int x=1; x<=3; ++x) {
            px(x,3,label);
        }
    }
}

void FuelSystem::handlePickups(const QList<Wheel*>& wheels, double& fuel) {
    if (wheels.isEmpty()) return;

    for (FuelCan& f : cans) {
        if (f.taken) continue;
        const double fx = f.wx + 2 * Constants::PIXEL_SIZE;
        const double fy = f.wy + 3 * Constants::PIXEL_SIZE;
        double minD2 = 1e18;
        for (const Wheel* w : wheels) {
            const double dx = w->x - fx;
            const double dy = w->y - fy;
            const double d2 = dx*dx + dy*dy;
            if (d2 < minD2) minD2 = d2;
        }
        const double R = Constants::FUEL_PICKUP_RADIUS + 20;
        if (minD2 <= R*R) { f.taken = true; fuel = Constants::FUEL_MAX; }
    }
}

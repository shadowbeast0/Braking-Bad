#ifndef FUEL_H
#define FUEL_H

#include <QVector>
#include <QColor>
#include <QPainter>
#include <QHash>
#include <random>
#include "constants.h"
#include "wheel.h"

struct FuelCan {
    int wx;
    int wy;
    bool taken = false;
};

class FuelSystem {
public:
    QVector<FuelCan> cans;
    int lastPlacedFuelX = 0;

    int currentFuelSpacing(double difficulty, double elapsedSeconds) const;

    void maybePlaceFuelAtEdge(int lastTerrainX, const QHash<int,int>& heightAtGX, double difficulty, double elapsedSeconds);

    void drawWorldFuel(QPainter& p, int cameraX, int cameraY) const;
    void handlePickups(const QList<Wheel*>& wheels, double& fuel);
};

#endif // FUEL_H

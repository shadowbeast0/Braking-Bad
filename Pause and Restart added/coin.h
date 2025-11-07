#ifndef COIN_H
#define COIN_H

#include <QVector>
#include <QColor>
#include <QPainter>
#include <QHash>
#include <random>
#include "constants.h"
#include "wheel.h"

struct Coin {
    int cx;
    int cy;
    bool taken = false;
};

class CoinSystem {
public:
    QVector<Coin> coins;
    int    lastPlacedCoinX = 0;
    double lastSpawnTimeSec = 0.0;

    void maybePlaceCoinStreamAtEdge(
        double elapsedSeconds,
        int cameraX,
        int viewWidth,
        const QHash<int,int>& heightAtGX,
        int lastTerrainX,
        std::mt19937& rng,
        std::uniform_real_distribution<float>& dist
        );

    void drawWorldCoins(QPainter& p, int cameraX, int cameraY, int gridW, int gridH) const;

    void handlePickups(const QList<Wheel*>& wheels, int& coinCount);
};

#endif // COIN_H

#include "coin.h"
#include <cmath>
#include <algorithm>

void CoinSystem::maybePlaceCoinStreamAtEdge(
    double elapsedSeconds,
    int cameraX,
    int viewWidth,
    const QHash<int,int>& heightAtGX,
    int lastTerrainX,
    std::mt19937& rng,
    std::uniform_real_distribution<float>& dist
    ) {
    if ((elapsedSeconds - lastSpawnTimeSec) < 5.0) {
        return;
    }

    const int viewRightX = cameraX + viewWidth;
    const int marginPx   = Constants::COIN_SPAWN_MARGIN_CELLS * Constants::PIXEL_SIZE;
    const int offRightX  = viewRightX + marginPx;

    const int terrainLimitX = lastTerrainX - Constants::PIXEL_SIZE * 10;
    if (terrainLimitX <= offRightX) {
        return;
    }

    std::uniform_int_distribution<int> coinLenDist(Constants::COIN_GROUP_MIN, Constants::COIN_GROUP_MAX);
    const int groupN = coinLenDist(rng);

    std::uniform_int_distribution<int> stepDist(Constants::COIN_GROUP_STEP_MIN, Constants::COIN_GROUP_STEP_MAX);
    const int stepCells = stepDist(rng);

    const int streamWidthPx = (groupN - 1) * stepCells * Constants::PIXEL_SIZE;

    int startX = offRightX + Constants::PIXEL_SIZE * 2;
    int endX   = startX + streamWidthPx;

    if (endX > terrainLimitX) {
        startX = terrainLimitX - streamWidthPx;
        endX   = terrainLimitX;
    }

    if (startX <= offRightX) {
        return;
    }

    const int ampCells = Constants::COIN_STREAM_AMP_CELLS;
    const double phase = dist(rng) * 6.2831853;

    for (int i = 0; i < groupN; ++i) {
        const int wx = startX + i * (stepCells * Constants::PIXEL_SIZE);
        const int gx = wx / Constants::PIXEL_SIZE;

        auto it = heightAtGX.constFind(gx);
        if (it == heightAtGX.constEnd()) {
            continue;
        }

        const int gyGround = it.value();
        const int arcOffsetCells =
            int(std::lround(std::sin(phase + i * 0.55) * ampCells));
        const int gy = gyGround - Constants::COIN_FLOOR_OFFSET_CELLS - arcOffsetCells;

        Coin c;
        c.cx = wx;
        c.cy = gy * Constants::PIXEL_SIZE;
        c.taken = false;
        coins.append(c);
    }

    lastPlacedCoinX   = endX;
    lastSpawnTimeSec  = elapsedSeconds;
}

void CoinSystem::drawWorldCoins(QPainter& p, int cameraX, int cameraY, int /*gridW*/, int /*gridH*/) const {
    const int camGX = cameraX / Constants::PIXEL_SIZE;
    const int camGY = cameraY / Constants::PIXEL_SIZE;

    QColor rim  (195,140,40);
    QColor fill (250,204,77);
    QColor fill2(245,184,50);
    QColor shine(255,255,220);

    auto plotGridPixel = [&](int gx, int gy, const QColor& c) {
        p.fillRect(gx * Constants::PIXEL_SIZE,
                   gy * Constants::PIXEL_SIZE,
                   Constants::PIXEL_SIZE,
                   Constants::PIXEL_SIZE, c);
    };

    auto drawCircleFilledMidpointGrid = [&](int gcx, int gcy, int gr, const QColor& color) {
        auto span = [&](int cy, int xl, int xr) {
            for (int xg = xl; xg <= xr; ++xg) {
                plotGridPixel(xg, cy, color);
            }
        };
        int x = 0;
        int y = gr;
        int d = 1 - gr;
        while (y >= x) {
            span(gcy + y, gcx - x, gcx + x);
            span(gcy - y, gcx - x, gcx + x);
            span(gcy + x, gcx - y, gcx + y);
            span(gcy - x, gcx - y, gcx + y);
            ++x;
            if (d < 0) {
                d += 2 * x + 1;
            } else {
                --y;
                d += 2 * (x - y) + 1;
            }
        }
    };

    for (const Coin& c : coins) {
        if (c.taken) continue;

        int scx = (c.cx / Constants::PIXEL_SIZE) - camGX;
        int scy = (c.cy / Constants::PIXEL_SIZE) + camGY;
        int r   = Constants::COIN_RADIUS_CELLS;

        drawCircleFilledMidpointGrid(scx, scy, r,   rim);
        if (r-1 > 0) {
            drawCircleFilledMidpointGrid(scx, scy, r-1, fill);
        }
        if (r-2 > 0) {
            drawCircleFilledMidpointGrid(scx, scy, r-2, fill2);
        }

        plotGridPixel(scx-1, scy-r+1, shine);
        plotGridPixel(scx,   scy-r+1, shine);
    }
}

void CoinSystem::handlePickups(const QList<Wheel*>& wheels, int& coinCount) {
    if (wheels.isEmpty()) return;

    for (Coin& c : coins) {
        if (c.taken) continue;
        double minD2 = 1e18;
        for (const Wheel* w : wheels) {
            const double dx = w->x - c.cx;
            const double dy = w->y - c.cy;
            const double d2 = dx*dx + dy*dy;
            if (d2 < minD2) minD2 = d2;
        }
        const double R = Constants::COIN_PICKUP_RADIUS;
        if (minD2 <= R*R) { c.taken = true; ++coinCount; }
    }
}

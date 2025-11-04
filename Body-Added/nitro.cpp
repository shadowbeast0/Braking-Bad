#include "nitro.h"

void NitroSystem::update(
    bool nitroKey,
    double fuel,
    double elapsedSeconds,
    double avgX,
    const std::function<int(int)>& groundGyNearestGX,
    const std::function<double(double)>& terrainTangentAngleAtX
    ) {
    bool wantNitro = nitroKey;

    if (!active) {
        if (wantNitro && fuel > 0.0 && elapsedSeconds >= cooldownUntil) {
            active = true;
            endTime = elapsedSeconds + Constants::NITRO_DURATION_SECOND;

            const double launchAngle = terrainTangentAngleAtX(avgX) + M_PI/3.0;
            dirX = std::cos(launchAngle);
            dirY = std::sin(launchAngle);

            int gx = int(avgX / Constants::PIXEL_SIZE);
            int gyGround = groundGyNearestGX(gx);
            ceilY = (gyGround - Constants::NITRO_MAX_ALT_CELLS) * Constants::PIXEL_SIZE;
        }
    } else {
        if (!wantNitro || fuel <= 0.0 || elapsedSeconds >= endTime) {
            active = false;
            cooldownUntil = elapsedSeconds + 2;
        }
    }
}

void NitroSystem::applyThrust(QList<Wheel*>& wheels) const {
    if (!active) return;

    // previous behavior: direction from wheels[0] -> wheels[1] if available
    double tDirX = 1.0;
    double tDirY = 0.0;
    if (wheels.size() >= 2) {
        const Wheel* back  = wheels.first();
        const Wheel* front = wheels[1];
        const double dx   = (front->x - back->x);
        const double dyUp = (back->y  - front->y);
        const double len  = std::sqrt(dx*dx + dyUp*dyUp);
        if (len > 1e-6) { tDirX = dx / len; tDirY = dyUp / len; }
    }

    for (Wheel* w : wheels) {
        w->m_vx += Constants::NITRO_THRUST * tDirX;
        w->m_vy += Constants::NITRO_THRUST * tDirY;
        if (w->y < ceilY) {
            w->y = ceilY;
            if (w->m_vy > 0.0) w->m_vy = 0.0;
        }
    }
}

void NitroSystem::drawHUD(QPainter& p, double elapsedSeconds) const {
    // previous pixel rocket icon + countdown
    int baseGX = Constants::HUD_LEFT_MARGIN;
    int baseGY = Constants::HUD_TOP_MARGIN + Constants::COIN_RADIUS_CELLS*2 + 4;

    QColor hull    (90,90,110);
    QColor tip     (180,180,190);
    QColor windowC (120,200,230);
    QColor flame1  (255,180,60);
    QColor flame2  (255,110,40);
    QColor shadow  (20,14,24);

    auto plot = [&](int gx, int gy, const QColor& c){
        p.fillRect((baseGX+gx) * Constants::PIXEL_SIZE,
                   (baseGY+gy) * Constants::PIXEL_SIZE,
                   Constants::PIXEL_SIZE, Constants::PIXEL_SIZE, c);
    };

    // rocket
    plot(1,1,hull); plot(2,1,hull); plot(3,1,hull); plot(4,1,hull);
    plot(1,2,hull); plot(2,2,windowC); plot(3,2,hull); plot(4,2,hull); plot(5,2,tip);
    plot(1,3,hull); plot(2,3,hull);    plot(3,3,hull); plot(4,3,hull);
    plot(0,2,flame1); plot(0,3,flame2);
    plot(2,4,shadow);

    // countdown text
    QFont f;
    f.setFamily("Monospace");
    f.setBold(true);
    f.setPointSize(12);
    p.setFont(f);
    p.setPen(QColor(20,14,24));

    double tleft = 0.0;
    if (active) {
        tleft = std::max(0.0, endTime - elapsedSeconds);
    } else if (elapsedSeconds < cooldownUntil) {
        tleft = std::max(0.0, cooldownUntil - elapsedSeconds);
    }

    int pxText = (baseGX + 8) * Constants::PIXEL_SIZE;
    int pyText = (baseGY + 5) * Constants::PIXEL_SIZE;
    p.drawText(pxText, pyText, QString::number(int(std::ceil(tleft))));
}

void NitroSystem::drawFlame(QPainter& p, const QList<Wheel*>& wheels, int cameraX, int cameraY, int viewW, int viewH) const {
    if (!active) return;
    if (wheels.isEmpty()) return;

    const Wheel* back = wheels.first();
    auto info = back->get(0, 0, viewW, viewH, -cameraX, cameraY);
    if (!info) return;

    int cx = (*info)[0];
    int cy = (*info)[1];
    int r  = (*info)[2];

    int gcx = cx / Constants::PIXEL_SIZE;
    int gcy = cy / Constants::PIXEL_SIZE;
    int gr  = std::max(1, r / Constants::PIXEL_SIZE);

    double dirX = 1.0;
    double dirY = 0.0;
    if (wheels.size() >= 2) {
        const Wheel* front = wheels[1];
        const double dx   = (front->x - back->x);
        const double dyUp = (back->y  - front->y);
        const double len  = std::sqrt(dx*dx + dyUp*dyUp);
        if (len > 1e-6) { dirX = dx / len; dirY = dyUp / len; }
    }

    double nUpX = -dirY;
    double nUpY =  dirX;

    int nozzleGX = gcx + int(std::round(nUpX * (gr + 1)));
    int nozzleGY = gcy - int(std::round(nUpY * (gr + 1)));

    QColor flameOuter(255,80,30);
    QColor flameMid  (255,140,40);
    QColor flameCore (255,230,90);
    QColor nozzle    (60, 60, 70);

    auto plotGridPixel = [&](int gx, int gy, const QColor& c) {
        p.fillRect(gx * Constants::PIXEL_SIZE,
                   gy * Constants::PIXEL_SIZE,
                   Constants::PIXEL_SIZE,
                   Constants::PIXEL_SIZE, c);
    };

    plotGridPixel(nozzleGX, nozzleGY, nozzle);

    double fx = nozzleGX;
    double fy = nozzleGY;

    double vx = -dirX;
    double vy =  dirY;

    int segments = 4;
    for (int i = 1; i <= segments; ++i) {
        int gx = int(std::round(fx + vx * i));
        int gy = int(std::round(fy - vy * i));
        if (i == segments) {
            plotGridPixel(gx, gy, flameCore);
        } else if (i == segments - 1) {
            plotGridPixel(gx, gy, flameMid);
        } else {
            plotGridPixel(gx, gy, flameOuter);
        }
    }
}

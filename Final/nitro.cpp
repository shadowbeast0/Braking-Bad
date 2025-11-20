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

void NitroSystem::drawHUD(QPainter& p, double elapsedSeconds, int levelIndex) const {
    int baseGX = Constants::HUD_LEFT_MARGIN;
    int baseGY = Constants::HUD_TOP_MARGIN + Constants::COIN_RADIUS_CELLS*2 + 4;
    QColor hull(90,90,110);
    QColor tip(180,180,190);
    QColor windowC(120,200,230);
    QColor flame1(255,180,60);
    QColor flame2(255,110,40);
    QColor shadow(20,14,24);
    auto plot = [&](int gx, int gy, const QColor& c){
        p.fillRect((baseGX+gx) * Constants::PIXEL_SIZE,
                   (baseGY+gy) * Constants::PIXEL_SIZE,
                   Constants::PIXEL_SIZE, Constants::PIXEL_SIZE, c);
    };
    plot(1,1,hull); plot(2,1,hull); plot(3,1,hull); plot(4,1,hull);
    plot(1,2,hull); plot(2,2,windowC); plot(3,2,hull); plot(4,2,hull); plot(5,2,tip);
    plot(1,3,hull); plot(2,3,hull); plot(3,3,hull); plot(4,3,hull);
    plot(0,2,flame1); plot(0,3,flame2);
    plot(2,4,shadow);
    QFont f; f.setFamily("Monospace"); f.setBold(true); f.setPointSize(12);
    p.setFont(f);
    p.setPen(Constants::TEXT_COLOR[levelIndex]);
    double tleft = 0.0;
    if (active) tleft = std::max(0.0, endTime - elapsedSeconds);
    else if (elapsedSeconds < cooldownUntil) tleft = std::max(0.0, cooldownUntil - elapsedSeconds);
    int pxText = (baseGX + 8) * Constants::PIXEL_SIZE;
    int pyText = (baseGY + 5) * Constants::PIXEL_SIZE;
    p.drawText(pxText, pyText, QString::number(int(std::ceil(tleft))));
}


void NitroSystem::drawFlame(QPainter& p, const QList<Wheel*>& wheels, int cameraX, int cameraY, int viewW, int viewH) const {
    if (!active) return;
    if (wheels.size() < 2) return;

    const Wheel* back  = wheels.first();
    const Wheel* front = wheels[1];

    auto info = back->get(0, 0, viewW, viewH, -cameraX, cameraY);
    if (!info) return;

    const int cx = (*info)[0];
    const int cy = (*info)[1];
    const int r  = (*info)[2];
    if (r <= 0) return;

    const int gcx = cx / Constants::PIXEL_SIZE;
    const int gcy = cy / Constants::PIXEL_SIZE;
    const int gr  = std::max(1, r / Constants::PIXEL_SIZE);

    double ux = front->x - back->x;
    double uy = front->y - back->y;
    double L = std::sqrt(ux*ux + uy*uy);
    if (L < 1e-6) return;
    ux /= L; uy /= L;

    const int nozzleOff = gr + 2;
    int nozzleGX = gcx - int(std::round(ux * nozzleOff));
    int nozzleGY = gcy - int(std::round(uy * nozzleOff));

    const double upx = uy;
    const double upy = -ux;
    const int liftCells = std::max(1, gr/2);
    nozzleGX += int(std::round(upx * liftCells));
    nozzleGY += int(std::round(upy * liftCells));

    QColor cNoz (70, 70, 80);
    QColor cOuter(255, 100, 35);
    QColor cMid  (255, 160, 45);
    QColor cCore (255, 240, 120);

    auto plot = [&](int gx, int gy, const QColor& c){
        p.fillRect(gx * Constants::PIXEL_SIZE,
                   gy * Constants::PIXEL_SIZE,
                   Constants::PIXEL_SIZE,
                   Constants::PIXEL_SIZE, c);
    };

    plot(nozzleGX, nozzleGY, cNoz);

    const int lenCells   = 5;
    const int baseWidth  = 1;

    const double nx = -uy;
    const double ny =  ux;

    for (int i = 1; i <= lenCells; ++i) {
        const int cxg = nozzleGX - int(std::round(ux * i));
        const int cyg = nozzleGY - int(std::round(uy * i));

        const double t = double(i) / double(lenCells);
        const int wOuter = std::max(0, int(std::lround(baseWidth * (1.0 - t))));
        const int wMid   = std::max(0, wOuter - 1);
        const int wCore  = std::max(0, int(std::floor(wOuter * 0.5)));

        for (int j = -wOuter; j <= wOuter; ++j) {
            const int gx = cxg + int(std::round(nx * j));
            const int gy = cyg + int(std::round(ny * j));
            plot(gx, gy, cOuter);
        }
        for (int j = -wMid; j <= wMid; ++j) {
            const int gx = cxg + int(std::round(nx * j));
            const int gy = cyg + int(std::round(ny * j));
            plot(gx, gy, cMid);
        }
        for (int j = -wCore; j <= wCore; ++j) {
            const int gx = cxg + int(std::round(nx * j));
            const int gy = cyg + int(std::round(ny * j));
            plot(gx, gy, cCore);
        }
    }
}




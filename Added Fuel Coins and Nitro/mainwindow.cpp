#include "mainwindow.h"
#include <QPainter>
#include <QKeyEvent>
#include <QPalette>
#include <QFont>
#include <QFontMetrics>
#include <cmath>
#include <algorithm>

MainWindow::MainWindow(QWidget *parent)
    : QWidget(parent),
    m_rng(std::random_device{}() ),
    m_dist(0.0f, 1.0f)
{
    setWindowTitle("Driver (Pixel Grid)");
    setFocusPolicy(Qt::StrongFocus);

    QPalette pal = palette();
    pal.setColor(QPalette::Window, QColor(100, 210, 255));
    setAutoFillBackground(true);
    setPalette(pal);

    m_grassPalette = {
        QColor(100, 140,  50), QColor(115, 155,  60), QColor(130, 170,  80),
        QColor( 85, 120,  40), QColor(100, 140,  50), QColor(115, 155,  60),
        QColor(130, 170,  80), QColor(115, 155,  60), QColor(130, 170,  80),
        QColor( 85, 120,  40), QColor( 85, 120,  40), QColor(100, 140,  50),
        QColor(100, 140,  50), QColor(115, 155,  60), QColor(130, 170,  80),
        QColor( 85, 120,  40), QColor(100, 140,  50), QColor(115, 155,  60),
        QColor( 85, 120,  40), QColor(100, 140,  50), QColor(115, 155,  60),
        QColor(100, 140,  50), QColor(115, 155,  60), QColor(130, 170,  80),
        QColor( 85, 120,  40), QColor(130, 170,  80), QColor(130, 170,  80),
        QColor( 85, 120,  40)
    };

    m_dirtPalette = {
        QColor(125,105,40), QColor(135,117,50), QColor(145,125,60),
        QColor(145,125,60), QColor(150,120,50), QColor(120,100,30),
        QColor(125,105,40), QColor(150,120,50), QColor(120,100,30),
        QColor(150,120,50), QColor(125,105,40), QColor(135,117,50),
        QColor(135,117,50), QColor(145,125,60), QColor(150,120,50),
        QColor(120,100,30), QColor(125,105,40), QColor(135,117,50),
        QColor(125,105,40), QColor(135,117,50), QColor(150,120,50),
        QColor(145,125,60), QColor(145,125,60), QColor(145,125,60),
        QColor(120,100,30), QColor(125,105,40), QColor(120,100,30),
        QColor(120,100,30), QColor(150,120,50), QColor(125,105,40),
        QColor(135,117,50), QColor(135,117,50), QColor(150,120,50),
        QColor(145,125,60), QColor(120,100,30), QColor(150,120,50)
    };

    showFullScreen();

    generateInitialTerrain();

    Wheel* w1 = new Wheel(100, 300, 20);
    Wheel* w2 = new Wheel(220, 300, 20);
    w1->attach(w2);
    m_wheels.append(w1);
    m_wheels.append(w2);

    m_timer = new QTimer(this);
    connect(m_timer, &QTimer::timeout, this, &MainWindow::gameLoop);
    m_timer->start(10);

    m_camX = m_cameraX;
    m_camY = m_cameraY;
    m_clock.start();

    m_showGrid = false;
}

MainWindow::~MainWindow() {
    qDeleteAll(m_wheels);
}

void MainWindow::generateInitialTerrain() {
    m_lastY = height() / 2;
    for (int i = m_step; i <= width() + m_step; i += m_step) {
        m_slope += (m_dist(m_rng) - static_cast<float>(m_lastY) / height()) * m_difficulty;
        m_slope = std::clamp(m_slope, -1.0f, 1.0f);
        const int newY = m_lastY + std::lround(m_slope * std::pow(std::abs(m_slope), m_irregularity) * m_step);
        Line seg(i - m_step, m_lastY, i, newY);
        m_lines.append(seg);
        rasterizeSegmentToHeightMapWorld(seg.getX1(), m_lastY, seg.getX2(), newY);
        m_lastY = newY;
        m_difficulty += m_difficultyIncrement;
    }
    m_lastX = m_step * m_lines.size();
}

void MainWindow::gameLoop() {
    const qint64 now = m_clock.nsecsElapsed();
    static qint64 prev = now;
    const qint64 dtns = now - prev;
    prev = now;
    const double dt = std::clamp(dtns / 1e9, 0.001, 0.033);

    m_elapsedSeconds += dt;

    double avgX = 0.0, avgY = 0.0;
    if (!m_wheels.isEmpty()) {
        for (const Wheel* w : m_wheels) { avgX += w->x; avgY += w->y; }
        avgX /= m_wheels.size();
        avgY /= m_wheels.size();
    }

    const double targetX = avgX - 200.0;
    const double targetY = -avgY + height() / 2.0;

    updateCamera(targetX, targetY, dt);

    m_cameraX = int(std::lround(m_camX));
    m_cameraY = int(std::lround(m_camY));

    while (targetX > m_cameraXFarthest) {
        m_cameraXFarthest += m_step;
        m_slope += (m_dist(m_rng) - static_cast<float>(m_lastY) / height()) * m_difficulty;
        m_slope = std::clamp(m_slope, -1.0f, 1.0f);
        const int newY = m_lastY + std::lround(m_slope * std::pow(std::abs(m_slope), m_irregularity) * m_step);
        Line seg(m_lastX, m_lastY, m_lastX + m_step, newY);
        m_lines.append(seg);
        rasterizeSegmentToHeightMapWorld(seg.getX1(), m_lastY, seg.getX2(), newY);
        m_lastY = newY;
        m_lastX += m_step;
        if (m_lines.size() > (width() / m_step) * 3) {
            m_lines.removeFirst();
            pruneHeightMap();
        }
        m_difficulty += m_difficultyIncrement;
        maybePlaceFuelAtEdge();
        maybePlaceCoinStreamAtEdge();
    }

    bool bothKeys = m_accelerating && m_braking;
    if (!m_nitro) {
        if (bothKeys && m_fuel > 0.0 && m_elapsedSeconds >= m_nitroCooldownUntil) {
            m_nitro = true;
            m_nitroEndTime = m_elapsedSeconds + NITRO_DURATION_SECOND;
            const double launchAngle = terrainTangentAngleAtX(avgX) + M_PI/3.0;
            m_nitroDirX = std::cos(launchAngle);
            m_nitroDirY = std::sin(launchAngle);
            int gx = int(avgX / PIXEL_SIZE);
            int gyGround = groundGyNearestGX(gx);
            m_nitroCeilY = (gyGround - NITRO_MAX_ALT_CELLS) * PIXEL_SIZE;
        }
    } else {
        if (!bothKeys || m_fuel <= 0.0 || m_elapsedSeconds >= m_nitroEndTime) {
            m_nitro = false;
            m_nitroCooldownUntil = m_elapsedSeconds + 10.0;
        }
    }

    const bool allowInput = (m_fuel > 0.0);
    bool accelDrive = allowInput && m_accelerating && !m_nitro;
    bool brakeDrive = allowInput && m_braking && !m_nitro;
    bool nitroDrive = allowInput && m_nitro;

    for (Wheel* w : m_wheels) w->simulate(m_lines, accelDrive, brakeDrive, nitroDrive);

    if (m_nitro) {
        for (Wheel* w : m_wheels) {
            w->m_vx += NITRO_THRUST * m_nitroDirX;
            w->m_vy += NITRO_THRUST * m_nitroDirY;
            if (w->y < m_nitroCeilY) {
                w->y = m_nitroCeilY;
                if (w->m_vy > 0.0) w->m_vy = 0.0;
            }
        }
    }

    if (m_fuel > 0.0) {
        double baseBurn = FUEL_BASE_BURN_PER_SEC * dt;
        double extra = 0.0;
        if (m_accelerating) extra = std::max(0.0, averageSpeed()) * FUEL_EXTRA_PER_SPEED * dt;
        double burnMult = m_nitro ? 3.0 : 1.0;
        m_fuel = std::max(0.0, m_fuel - burnMult * (baseBurn + extra));
    }

    const int minX = leftmostTerrainX();
    for (Wheel* w : m_wheels) {
        if (w->x < minX) {
            w->x = minX;
            w->m_omega = 0;
            w->m_vx = 0;
        }
    }

    if (!m_wheels.isEmpty()) {
        for (FuelCan& f : m_worldFuel) {
            if (f.taken) continue;
            const double fx = f.wx + 2 * PIXEL_SIZE;
            const double fy = f.wy + 3 * PIXEL_SIZE;
            double minD2 = 1e18;
            for (const Wheel* w : m_wheels) {
                const double dx = w->x - fx;
                const double dy = w->y - fy;
                const double d2 = dx*dx + dy*dy;
                if (d2 < minD2) minD2 = d2;
            }
            const double R = FUEL_PICKUP_RADIUS + 20;
            if (minD2 <= R*R) { f.taken = true; m_fuel = FUEL_MAX; }
        }
    }

    if (!m_wheels.isEmpty()) {
        for (Coin& c : m_worldCoins) {
            if (c.taken) continue;
            double minD2 = 1e18;
            for (const Wheel* w : m_wheels) {
                const double dx = w->x - c.cx;
                const double dy = w->y - c.cy;
                const double d2 = dx*dx + dy*dy;
                if (d2 < minD2) minD2 = d2;
            }
            const double R = COIN_PICKUP_RADIUS;
            if (minD2 <= R*R) { c.taken = true; ++m_coinCount; }
        }
    }

    update();
}

int MainWindow::leftmostTerrainX() const {
    if (m_lines.isEmpty()) return 0;
    return m_lines.first().getX1();
}

void MainWindow::paintEvent(QPaintEvent *event) {
    Q_UNUSED(event);
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);
    p.setPen(Qt::NoPen);

    const int camGX = m_cameraX / PIXEL_SIZE;
    const int camGY = m_cameraY / PIXEL_SIZE;
    const int offX  = -(m_cameraX - camGX * PIXEL_SIZE);
    const int offY  =  (m_cameraY - camGY * PIXEL_SIZE);

    p.save();
    p.translate(offX, offY);

    if (m_showGrid) drawGridOverlay(p);

    drawFilledTerrain(p);
    drawWorldFuel(p);
    drawWorldCoins(p);
    drawNitroFlame(p);

    const QColor wheelColor(40, 50, 60);
    for (const Wheel* wheel : m_wheels) {
        if (auto info = wheel->get(0, 0, width(), height(), -m_cameraX, m_cameraY)) {
            const int cx = (*info)[0];
            const int cy = (*info)[1];
            const int r  = (*info)[2];
            const int gcx = cx / PIXEL_SIZE;
            const int gcy = cy / PIXEL_SIZE;
            const int gr  = std::max(1, r  / PIXEL_SIZE);
            drawCircleFilledMidpointGrid(p, gcx, gcy, gr, wheelColor);
        }
    }

    p.restore();
    drawHUDFuel(p);
    drawHUDCoins(p);
    drawHUDNitro(p);
}

void MainWindow::updateCamera(double tx, double ty, double dt) {
    const double wn = m_camWN;
    const double z  = m_camZeta;
    const double ax = wn*wn * (tx - m_camX) - 2.0*z*wn * m_camVX;
    m_camVX += ax * dt;
    m_camX  += m_camVX * dt;
    const double ay = wn*wn * (ty - m_camY) - 2.0*z*wn * m_camVY;
    m_camVY += ay * dt;
    m_camY  += m_camVY * dt;
}

void MainWindow::drawFilledTerrain(QPainter& p) {
    const int camGX = m_cameraX / PIXEL_SIZE;
    const int camGY = m_cameraY / PIXEL_SIZE;
    for (int sgx = 0; sgx <= gridW(); ++sgx) {
        const int worldGX = sgx + camGX;
        auto it = m_heightAtGX.constFind(worldGX);
        if (it == m_heightAtGX.constEnd()) continue;
        const int groundWorldGY = it.value();
        int startScreenGY = groundWorldGY + camGY;
        if (startScreenGY < 0) startScreenGY = 0;
        if (startScreenGY >= gridH()) continue;
        for (int sGY = startScreenGY; sGY <= gridH(); ++sGY) {
            const int worldGY = sGY - camGY;
            bool greenTopZone = (sGY < groundWorldGY + camGY + 3*SHADING_BLOCK);
            const QColor shade = grassShadeForBlock(worldGX, worldGY, greenTopZone);
            plotGridPixel(p, sgx, sGY, shade);
        }
        const QColor edge = grassShadeForBlock(worldGX, groundWorldGY, true).darker(115);
        plotGridPixel(p, sgx, groundWorldGY + camGY, edge);
    }
}

QColor MainWindow::grassShadeForBlock(int worldGX, int worldGY, bool greenify) const {
    const int bx = worldGX / SHADING_BLOCK;
    const int by = worldGY / SHADING_BLOCK;
    const quint32 h = hash2D(bx, by);
    if (greenify) {
        const int idxG = int(h % m_grassPalette.size());
        return m_grassPalette[idxG];
    } else {
        const int idxD = int(h % m_dirtPalette.size());
        return m_dirtPalette[idxD];
    }
}

void MainWindow::drawNitroFlame(QPainter& p) {
    if (!m_nitro) return;
    if (m_wheels.isEmpty()) return;
    const Wheel* thruster = m_wheels.first();
    auto info = thruster->get(0, 0, width(), height(), -m_cameraX, m_cameraY);
    if (!info) return;
    int cx = (*info)[0];
    int cy = (*info)[1];
    int r  = (*info)[2];
    int gcx = cx / PIXEL_SIZE;
    int gcy = cy / PIXEL_SIZE;
    int gr  = std::max(1, r / PIXEL_SIZE);
    QColor flameOuter(255,80,30);
    QColor flameMid(255,140,40);
    QColor flameCore(255,230,90);
    for (int yy = -1; yy <= 1; ++yy) {
        plotGridPixel(p, gcx-(gr+1), gcy+yy, flameMid);
        plotGridPixel(p, gcx-(gr+2), gcy+yy, flameOuter);
    }
    plotGridPixel(p, gcx-(gr+3), gcy, flameCore);
}

void MainWindow::drawGridOverlay(QPainter& p) {
    p.save();
    QPen pen(QColor(140, 140, 140));
    pen.setWidth(1);
    p.setPen(pen);
    p.setBrush(Qt::NoBrush);
    for (int x = 0; x <= width(); x += PIXEL_SIZE) p.drawLine(x, 0, x, height());
    for (int y = 0; y <= height(); y += PIXEL_SIZE) p.drawLine(0, y, width(), y);
    p.restore();
}

void MainWindow::plotGridPixel(QPainter& p, int gx, int gy, const QColor& c) {
    if (gx < 0 || gy < 0 || gx >= gridW() + 1 || gy >= gridH() + 1) return;
    p.fillRect(gx * PIXEL_SIZE, gy * PIXEL_SIZE, PIXEL_SIZE, PIXEL_SIZE, c);
}

void MainWindow::drawCircleFilledMidpointGrid(QPainter& p, int gcx, int gcy, int gr, const QColor& c) {
    int x = 0, y = gr, d = 1 - gr;
    auto span = [&](int cy, int xl, int xr) { for (int xg = xl; xg <= xr; ++xg) plotGridPixel(p, xg, cy, c); };
    while (y >= x) {
        span(gcy + y, gcx - x, gcx + x);
        span(gcy - y, gcx - x, gcx + x);
        span(gcy + x, gcx - y, gcx + y);
        span(gcy - x, gcx - y, gcx + y);
        ++x;
        if (d < 0) d += 2 * x + 1; else { --y; d += 2 * (x - y) + 1; }
    }
}

void MainWindow::rasterizeSegmentToHeightMapWorld(int x1, int y1, int x2, int y2) {
    if (x2 < x1) { std::swap(x1, x2); std::swap(y1, y2); }
    const int gx1 = x1 / PIXEL_SIZE;
    const int gx2 = x2 / PIXEL_SIZE;
    if (x2 == x1) {
        const int gy = static_cast<int>(std::floor(y1 / double(PIXEL_SIZE) + 0.5));
        m_heightAtGX.insert(gx1, gy);
        return;
    }
    const double dx = double(x2 - x1);
    const double dy = double(y2 - y1);
    for (int gx = gx1; gx <= gx2; ++gx) {
        const double wx = gx * double(PIXEL_SIZE);
        double t = (wx - x1) / dx;
        t = std::clamp(t, 0.0, 1.0);
        const double wy = y1 + t * dy;
        const int gy = static_cast<int>(std::floor(wy / double(PIXEL_SIZE) + 0.5));
        m_heightAtGX.insert(gx, gy);
    }
}

void MainWindow::pruneHeightMap() {
    if (m_lines.isEmpty()) return;
    const int keepFromGX = (m_lines.first().getX1() / PIXEL_SIZE) - 4;
    QList<int> toRemove;
    toRemove.reserve(m_heightAtGX.size());
    for (auto it = m_heightAtGX.constBegin(); it != m_heightAtGX.constEnd(); ++it) {
        if (it.key() < keepFromGX) toRemove.append(it.key());
    }
    for (int k : toRemove) m_heightAtGX.remove(k);
}

void MainWindow::keyPressEvent(QKeyEvent *event) {
    if (event->isAutoRepeat()) return;
    switch (event->key()) {
    case Qt::Key_D:
    case Qt::Key_Right: m_accelerating = true; break;
    case Qt::Key_A:
    case Qt::Key_Left:  m_braking = true; break;
    case Qt::Key_G:     m_showGrid = !m_showGrid; break;
    case Qt::Key_Escape: close(); break;
    default: QWidget::keyPressEvent(event);
    }
}

void MainWindow::keyReleaseEvent(QKeyEvent *event) {
    if (event->isAutoRepeat()) return;
    switch (event->key()) {
    case Qt::Key_D:
    case Qt::Key_Right: m_accelerating = false; break;
    case Qt::Key_A:
    case Qt::Key_Left:  m_braking = false; break;
    default: QWidget::keyReleaseEvent(event);
    }
}

int MainWindow::currentFuelSpacing() {
    int base = 700;
    int byDiff = int(500 * (m_difficulty / 0.005));
    int byTime = int(35 * m_elapsedSeconds);
    int spacing = base + byDiff + byTime;
    spacing = int(std::round(spacing * FUEL_SPAWN_EASE));
    int minSpacing = int(std::round(450 * FUEL_SPAWN_EASE));
    if (spacing < minSpacing) spacing = minSpacing;
    return spacing;
}

void MainWindow::maybePlaceFuelAtEdge() {
    if (m_lastX - m_lastPlacedFuelX < currentFuelSpacing()) return;
    int gx = m_lastX / PIXEL_SIZE;
    auto it = m_heightAtGX.constFind(gx);
    if (it == m_heightAtGX.constEnd()) return;
    const int gyGround = it.value();
    FuelCan f; f.wx = m_lastX; f.wy = (gyGround - FUEL_FLOOR_OFFSET_CELLS) * PIXEL_SIZE; f.taken = false;
    m_worldFuel.append(f);
    m_lastPlacedFuelX = m_lastX;
}

void MainWindow::drawWorldFuel(QPainter& p) {
    const int camGX = m_cameraX / PIXEL_SIZE;
    const int camGY = m_cameraY / PIXEL_SIZE;
    QColor body(230, 60, 60), cap(230,230,230), label(255,200,50), shadow(0,0,0,90);
    for (const FuelCan& f : m_worldFuel) {
        if (f.taken) continue;
        int sx = (f.wx / PIXEL_SIZE) - camGX;
        int sy = (f.wy / PIXEL_SIZE) + camGY;
        auto px = [&](int gx, int gy, const QColor& c){ plotGridPixel(p, sx+gx, sy+gy, c); };
        px(2,1,shadow); px(5,2,shadow);
        px(1,0,cap); px(2,0,cap);
        for (int y=1; y<=5; ++y) for (int x=0; x<=4; ++x) px(x,y,body);
        for (int x=1; x<=3; ++x) px(x,3,label);
    }
}

void MainWindow::drawHUDFuel(QPainter& p) {
    int gy = HUD_TOP_MARGIN;
    int wcells = std::min(std::max(gridW()/4, 24), 48);
    int gx = (gridW() - wcells)/2;
    int barH = 3;
    double frac = std::clamp(m_fuel / FUEL_MAX, 0.0, 1.0);
    int filled = int(std::floor(wcells * frac));
    auto lerp = [](const QColor& c1, const QColor& c2, double t)->QColor{
        int r = int((1-t)*c1.red() + t*c2.red());
        int g = int((1-t)*c1.green() + t*c2.green());
        int b = int((1-t)*c1.blue() + t*c2.blue());
        return QColor(r,g,b);
    };
    QColor startC(40,230,55), midC(250,230,80), endC(230,50,40);
    for (int x=0;x<wcells;++x) for (int y=0;y<barH;++y) plotGridPixel(p, gx+x, gy+y, QColor(20,14,24));
    for (int x=0;x<filled;++x){
        double t = double(x)/std::max(wcells-1,1);
        QColor c = (t<0.5) ? lerp(startC, midC, t*2) : lerp(midC, endC, (t-0.5)*2);
        for (int y=0;y<barH;++y) plotGridPixel(p, gx+x, gy+y, c);
    }
    for (int y=0;y<barH;++y){
        plotGridPixel(p,gx-1,gy+y,QColor(35,35,48));
        plotGridPixel(p,gx+wcells,gy+y,QColor(35,35,48));
    }
    for (int x=-1;x<=wcells;++x){
        plotGridPixel(p,gx+x,gy-1,QColor(35,35,48));
        plotGridPixel(p,gx+x,gy+barH,QColor(35,35,48));
    }
    int tickEvery = std::max(6, wcells/6);
    for (int x=tickEvery; x<wcells; x+=tickEvery)
        plotGridPixel(p, gx+x, gy+barH, QColor(80,80,70));
}

double MainWindow::averageSpeed() const {
    if (m_wheels.isEmpty()) return 0.0;
    double s = 0.0;
    for (const Wheel* w : m_wheels) s += std::sqrt(w->m_vx*w->m_vx + w->m_vy*w->m_vy);
    return s / m_wheels.size();
}

int MainWindow::currentCoinSpacing() {
    int base = 650;
    int jitter = 120 + int(m_dist(m_rng) * 260);
    int byDiff = int(150 * (m_difficulty / 0.005));
    int byTime = int(6 * m_elapsedSeconds);
    int spacing = base + jitter + byDiff + byTime;
    int minSpacing = 520;
    if (spacing < minSpacing) spacing = minSpacing;
    return spacing;
}

void MainWindow::maybePlaceCoinStreamAtEdge() {
    if (m_lastX - m_lastPlacedCoinX < currentCoinSpacing()) return;
    const int screenRight = m_cameraX + width();
    const int margin = COIN_SPAWN_MARGIN_CELLS * PIXEL_SIZE;
    const int latestEndX = m_lastX - PIXEL_SIZE * 2;
    const int desiredEndX = std::max(screenRight + margin, m_lastPlacedCoinX + currentCoinSpacing());
    if (desiredEndX > latestEndX) return;

    float r = m_dist(m_rng);
    int minN = 5, maxN = 8;
    if (r >= 0.65f && r < 0.95f) { minN = 9; maxN = 12; }
    else if (r >= 0.95f) { minN = 13; maxN = COIN_GROUP_MAX; }
    int groupN = minN + int(m_dist(m_rng) * (maxN - minN + 1));

    int stepCells = COIN_GROUP_STEP_MIN + int(m_dist(m_rng) * (COIN_GROUP_STEP_MAX - COIN_GROUP_STEP_MIN + 1));
    int ampCells = 1 + int(m_dist(m_rng) * 3);
    double phase = m_dist(m_rng) * 6.2831853;

    const int totalCells = (groupN - 1) * stepCells;
    int startX = desiredEndX - totalCells * PIXEL_SIZE;

    const int leftClamp = leftmostTerrainX() + PIXEL_SIZE * 6;
    if (startX < leftClamp) {
        int maxCoins = 1 + (desiredEndX - leftClamp) / (stepCells * PIXEL_SIZE);
        if (maxCoins < 2) return;
        groupN = std::min(groupN, maxCoins);
        startX = desiredEndX - (groupN - 1) * stepCells * PIXEL_SIZE;
        if (startX < leftClamp) startX = leftClamp;
    }

    for (int i = 0; i < groupN; ++i) {
        int wx = startX + i * (stepCells * PIXEL_SIZE);
        int gx = wx / PIXEL_SIZE;
        auto it = m_heightAtGX.constFind(gx);
        if (it == m_heightAtGX.constEnd()) continue;
        int gyGround = it.value();
        int arcOffset = int(std::lround(std::sin(phase + i*0.55) * ampCells));
        int gy = gyGround - COIN_FLOOR_OFFSET_CELLS - arcOffset;
        Coin c; c.cx = wx; c.cy = gy * PIXEL_SIZE; c.taken = false;
        m_worldCoins.append(c);
    }

    m_lastPlacedCoinX = desiredEndX;
}

void MainWindow::drawWorldCoins(QPainter& p) {
    const int camGX = m_cameraX / PIXEL_SIZE;
    const int camGY = m_cameraY / PIXEL_SIZE;
    QColor rim(195,140,40), fill(250,204,77), fill2(245,184,50), shine(255,255,220);
    for (const Coin& c : m_worldCoins) {
        if (c.taken) continue;
        int scx = (c.cx / PIXEL_SIZE) - camGX;
        int scy = (c.cy / PIXEL_SIZE) + camGY;
        int r = COIN_RADIUS_CELLS;
        drawCircleFilledMidpointGrid(p, scx, scy, r, rim);
        if (r-1 > 0) drawCircleFilledMidpointGrid(p, scx, scy, r-1, fill);
        if (r-2 > 0) drawCircleFilledMidpointGrid(p, scx, scy, r-2, fill2);
        plotGridPixel(p, scx-1, scy-r+1, shine);
        plotGridPixel(p, scx,   scy-r+1, shine);
    }
}

void MainWindow::drawHUDCoins(QPainter& p) {
    int iconGX = HUD_LEFT_MARGIN + COIN_RADIUS_CELLS + 1;
    int iconGY = HUD_TOP_MARGIN + COIN_RADIUS_CELLS;
    drawCircleFilledMidpointGrid(p, iconGX, iconGY, COIN_RADIUS_CELLS, QColor(195,140,40));
    drawCircleFilledMidpointGrid(p, iconGX, iconGY, std::max(1, COIN_RADIUS_CELLS-1), QColor(250,204,77));
    plotGridPixel(p, iconGX-1, iconGY-COIN_RADIUS_CELLS+1, QColor(255,255,220));
    QFont f; f.setFamily("Monospace"); f.setBold(true); f.setPointSize(12);
    p.setFont(f); p.setPen(QColor(20,14,24));
    int px = (HUD_LEFT_MARGIN + COIN_RADIUS_CELLS*2 + 3) * PIXEL_SIZE;
    int py = (HUD_TOP_MARGIN + COIN_RADIUS_CELLS + 2) * PIXEL_SIZE;
    p.drawText(px, py, QString::number(m_coinCount));
}

void MainWindow::drawHUDNitro(QPainter& p) {
    int baseGX = HUD_LEFT_MARGIN;
    int baseGY = HUD_TOP_MARGIN + COIN_RADIUS_CELLS*2 + 4;
    QColor hull(90,90,110), tip(180,180,190), windowC(120,200,230), flame1(255,180,60), flame2(255,110,40), shadow(20,14,24);
    auto px = [&](int gx, int gy, const QColor& c){ plotGridPixel(p, baseGX+gx, baseGY+gy, c); };
    px(1,1,hull); px(2,1,hull); px(3,1,hull); px(4,1,hull);
    px(1,2,hull); px(2,2,windowC); px(3,2,hull); px(4,2,hull); px(5,2,tip);
    px(1,3,hull); px(2,3,hull); px(3,3,hull); px(4,3,hull);
    px(0,2,flame1); px(0,3,flame2);
    px(2,4,shadow);
    QFont f; f.setFamily("Monospace"); f.setBold(true); f.setPointSize(12);
    p.setFont(f); p.setPen(QColor(20,14,24));
    double tleft = 0.0;
    if (m_nitro) tleft = std::max(0.0, m_nitroEndTime - m_elapsedSeconds);
    else if (m_elapsedSeconds < m_nitroCooldownUntil) tleft = std::max(0.0, m_nitroCooldownUntil - m_elapsedSeconds);
    int pxText = (baseGX + 8) * PIXEL_SIZE;
    int pyText = (baseGY + 5) * PIXEL_SIZE;
    p.drawText(pxText, pyText, QString::number(int(std::ceil(tleft))));
}

int MainWindow::groundGyNearestGX(int gx) const {
    auto it = m_heightAtGX.constFind(gx);
    if (it != m_heightAtGX.constEnd()) return it.value();
    for (int d = 1; d <= 8; ++d) {
        auto itL = m_heightAtGX.constFind(gx - d);
        if (itL != m_heightAtGX.constEnd()) return itL.value();
        auto itR = m_heightAtGX.constFind(gx + d);
        if (itR != m_heightAtGX.constEnd()) return itR.value();
    }
    return 0;
}

double MainWindow::terrainTangentAngleAtX(double wx) const {
    int gx = int(wx / PIXEL_SIZE);
    int gyL = groundGyNearestGX(gx - 1);
    int gyR = groundGyNearestGX(gx + 1);
    double dyCells = double(gyR - gyL);
    return std::atan2(-dyCells, 2.0);
}

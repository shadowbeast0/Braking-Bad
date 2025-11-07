// mainwindow.cpp
#include "mainwindow.h"
#include "coin.h"
#include "outro.h"
#include <QSettings>
#include <QCloseEvent>
#include <QPainter>
#include <QKeyEvent>
#include <QPalette>
#include <QTimer>
#include <QFont>
#include <cmath>
#include <algorithm>
#include <map>
#include <list>
#include <limits>

MainWindow::MainWindow(QWidget *parent)
    : QWidget(parent),
    m_rng(std::random_device{}() ),
    m_dist(0.0f, 1.0f)
{
    setWindowTitle("Driver (Pixel Grid)");
    setFocusPolicy(Qt::StrongFocus);

    m_pause = new PauseOverlay(this);
    m_pause->setGeometry(rect());
    m_pause->hide();
    connect(m_pause, &PauseOverlay::resumeRequested, this, [this]{
        if (m_pause) m_pause->hide();
        if (m_timer) m_timer->start();
        setFocus();
    });


    showFullScreen();

    m_media = new Media(this);
    m_media->setupBgm();
    m_media->setBgmVolume(0.35);
    m_media->playBgm();
    generateInitialTerrain();

    Wheel* w1 = new Wheel(100, 300, 20);
    Wheel* w2 = new Wheel(220, 300, 20);
    Wheel* w3 = new Wheel(160, 300, 0);
    w1->attach(w3);
    w2->attach(w3);
    w1->attach(w2);
    m_wheels.append(w1);
    m_wheels.append(w2);
    m_wheels.append(w3);

    CarBody* body = new CarBody();

    auto bodyPoints = QVector<QPoint>{QPoint(0,0), QPoint(0,31), QPoint(9,37), QPoint(15,19), QPoint(44,19), QPoint(50,37), QPoint(138,37), QPoint(144,19), QPoint(172,19), QPoint(179,37), QPoint(188,31), QPoint(181,6), QPoint(137,0), QPoint(112,-25), QPoint(62,-25), QPoint(37,-6), QPoint(25,-7), QPoint(19,-19), QPoint(4,-21), QPoint(1,-15), QPoint(12, -10)};
    body->addPoints(bodyPoints);

    auto hitboxPoints = QVector<QPoint>{QPoint(15,6), QPoint(173, 1),  QPoint(137,0), QPoint(112,-25), QPoint(62,-25), QPoint(37,-6), QPoint(25,-7), QPoint(19,-19), QPoint(4,-21), QPoint(1,-15), QPoint(12, -10)};
    body->addHitbox(hitboxPoints);

    auto killPoints = QVector<QPoint>{QPoint(112, -25), QPoint(62, -25)};
    body->addKillSwitches(killPoints);

    body->addWheel(w1);
    body->addWheel(w2);
    body->addWheel(w3);

    QColor winC(120,170,220);
    QColor handleC(20,20,24);

    QVector<QPoint> glassWin{
        QPoint(48, 0), QPoint(68, -14), QPoint(105, -14), QPoint(112, -1), QPoint(110, 0)
    };

    QVector<QPoint> handle{
        QPoint(96, 10), QPoint(102, 10), QPoint(102, 13), QPoint(96, 13)
    };

    body->addAttachment(glassWin,  winC);
    body->addAttachment(handle, handleC);
    body->finish();
    m_bodies.append(body);

    m_timer = new QTimer(this);
    connect(m_timer, &QTimer::timeout, this, &MainWindow::gameLoop);
    m_timer->start(10);

    m_camX = m_cameraX;
    m_camY = m_cameraY;
    m_clock.start();

    m_showGrid = false;

    m_intro = new IntroScreen(this);
    m_intro->setGeometry(rect());
    m_intro->show();
    m_timer->stop();

    connect(m_intro, &IntroScreen::exitRequested, this, &QWidget::close);
    connect(m_intro, &IntroScreen::startRequested, this, [this](int levelIndex){
        if (m_intro) { m_intro->hide(); m_intro->deleteLater(); m_intro = nullptr; }
        level_index = levelIndex;

        QPalette pal = palette();
        pal.setColor(QPalette::Window, Constants::SKY_COLOR[level_index]);
        setAutoFillBackground(true);
        setPalette(pal);

        resetGameRound();
        setFocus();
        if (m_timer) m_timer->start();
    });
}

MainWindow::~MainWindow() {
    qDeleteAll(m_wheels);
}


void MainWindow::resizeEvent(QResizeEvent *e) {
    QWidget::resizeEvent(e);
    if (m_intro) m_intro->setGeometry(rect());
    if (m_pause) m_pause->setGeometry(rect());
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
    double fuelBefore = m_fuel;
    int coinsBefore = m_coinCount;
    double avgX = 0.0, avgY = 0.0;
    if (!m_wheels.isEmpty()) {
        for (const Wheel* w : m_wheels) { avgX += w->x; avgY += w->y; }
        avgX /= m_wheels.size();
        avgY /= m_wheels.size();
    }

    double dx = std::max(0.0, avgX - m_lastScoreX);
    m_totalDistanceCells += dx / double(Constants::PIXEL_SIZE);
    m_lastScoreX = avgX;

    m_score = int(std::llround(Constants::SCORE_DIST_PER_CELL * m_totalDistanceCells +
                               Constants::SCORE_PER_COIN * m_coinCount +
                               Constants::SCORE_PER_NITRO * m_nitroUses));

    double bodyX = (!m_bodies.isEmpty()) ? m_bodies.first()->getX() : avgX;
    double bodyY = (!m_bodies.isEmpty()) ? m_bodies.first()->getY() : avgY;
    const double targetX = bodyX - 200.0;
    const double targetY = -bodyY + height() / 2.0;
    updateCamera(targetX, targetY, dt);
    m_cameraX = int(std::lround(m_camX));
    m_cameraY = int(std::lround(m_camY));
    double angleRad = 0.0;
    if (m_wheels.size() >= 2) {
        const double dx = (m_wheels[1]->x - m_wheels[0]->x);
        const double dy = (m_wheels[1]->y - m_wheels[0]->y);
        angleRad = std::atan2(dy, dx);
    }
    double carX = (!m_bodies.isEmpty()) ? m_bodies.first()->getX() : avgX;
    double carY = (!m_bodies.isEmpty()) ? m_bodies.first()->getY() : avgY;

    m_flip.update(angleRad, carX, carY, m_elapsedSeconds, [this](int bonus){ m_coinCount += bonus; });


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
        if (m_lines.size() > (width() / m_step) * 3) { m_lines.removeFirst(); pruneHeightMap(); }
        m_difficulty += m_difficultyIncrement;

        m_fuelSys.maybePlaceFuelAtEdge(m_lastX, m_heightAtGX, m_difficulty, m_elapsedSeconds);
        maybeSpawnCloud();
    }

    {
        const int viewRightX = m_cameraX + width();
        const int marginPx   = Constants::COIN_SPAWN_MARGIN_CELLS * Constants::PIXEL_SIZE;
        const int offRightX  = viewRightX + marginPx;
        const int maxStreamWidthPx =
            (Constants::COIN_GROUP_MAX - 1) * Constants::COIN_GROUP_STEP_MAX * Constants::PIXEL_SIZE;
        ensureAheadTerrain(offRightX + maxStreamWidthPx + Constants::PIXEL_SIZE * 20);
    }

    m_coinSys.maybePlaceCoinStreamAtEdge(
        m_elapsedSeconds, m_cameraX, width(), m_heightAtGX, m_lastX, m_rng, m_dist);

    m_nitroSys.update(
        m_nitroKey, m_fuel, m_elapsedSeconds, avgX,
        [this](int gx){ return this->groundGyNearestGX(gx); },
        [this](double wx){ return this->terrainTangentAngleAtX(wx); }
        );

    if (m_nitroSys.active && !m_prevNitroActive) ++m_nitroUses;
    m_prevNitroActive = m_nitroSys.active;

    const bool allowInput = (m_fuel > 0.0);
    bool accelDrive = false, brakeDrive = false, nitroDrive = false;

    if (m_nitroSys.active && allowInput) {
        accelDrive = false; brakeDrive = false; nitroDrive = true;
    }
    else {
        nitroDrive = false;
        bool bothKeys = m_accelerating && m_braking;
        if (allowInput) {
            if (bothKeys) { accelDrive = true; brakeDrive = true; }
            else { accelDrive = m_accelerating; brakeDrive = m_braking; }
        } else { accelDrive = false; brakeDrive = false; }
    }

    for (Wheel* w : m_wheels)
        w->simulate(level_index, m_lines, accelDrive, brakeDrive, nitroDrive);

    for (CarBody* b : m_bodies)
        b->simulate(level_index, m_lines, accelDrive, brakeDrive);

    m_nitroSys.applyThrust(m_wheels);

    if (m_fuel > 0.0) {
        double baseBurn = Constants::FUEL_BASE_BURN_PER_SEC * dt;
        double extra = 0.0;
        if (m_accelerating) extra = std::max(0.0, averageSpeed()) * Constants::FUEL_EXTRA_PER_SPEED * dt;
        double burnMult = m_nitroSys.active ? 3.0 : 1.0;
        m_fuel = std::max(0.0, m_fuel - burnMult * (baseBurn + extra));
    }

    const int minX = leftmostTerrainX();
    for (Wheel* w : m_wheels) {
        if (w->x < minX) { w->x = minX; w->m_vx = 0; }
    }

    if (!isFullyUpsideDown()) {
        m_fuelSys.handlePickups(m_wheels, m_fuel);
        m_coinSys.handlePickups(m_wheels, m_coinCount);
    }
    if (m_coinCount > coinsBefore) m_media->coinPickup();
    if ((m_fuel - fuelBefore) > 1e-3 && !m_suppressFuelSfx) m_media->fuelPickup();
    {
        auto ptSegDist2 = [](double px, double py, const Line& ln)->double {
            double x1 = ln.getX1(), y1 = ln.getY1();
            double x2 = ln.getX2(), y2 = ln.getY2();
            double vx = x2 - x1,   vy = y2 - y1;
            double wx = px - x1,   wy = py - y1;
            double len2 = vx*vx + vy*vy;
            double t = (len2 > 0.0) ? (wx*vx + wy*vy) / len2 : 0.0;
            if (t < 0.0) t = 0.0; else if (t > 1.0) t = 1.0;
            double cx = x1 + t*vx, cy = y1 + t*vy;
            double dx = px - cx,   dy = py - cy;
            return dx*dx + dy*dy;
        };

        const double R2 = double(Constants::COIN_PICKUP_RADIUS) * double(Constants::COIN_PICKUP_RADIUS);

        for (auto& coin : m_coinSys.coins) {
            if (coin.taken) continue;
            bool hit = false;

            for (CarBody* body : m_bodies) {
                const auto edges = body->getLines();
                for (const Line& ln : edges) {
                    if (ptSegDist2(coin.cx, coin.cy, ln) <= R2) {
                        hit = true;
                        break;
                    }
                }

                if (!hit) {
                    const auto bodyPoints = body->get(0, 0);
                    QPolygon polygon;
                    for (const QPoint& p : bodyPoints)
                        polygon << QPoint(p.x(), p.y());
                    if (polygon.containsPoint(QPoint(coin.cx, coin.cy), Qt::OddEvenFill))
                        hit = true;
                }

                if (hit) break;
            }

            if (hit) {
                coin.taken = true;
                ++m_coinCount;
            }
        }

    }

    const bool fuelEmpty = (m_fuel <= 0.0);
    const bool roofHit   = isRoofTouchingTerrain();

    if (roofHit && !m_roofCrashLatched) {
        m_roofCrashLatched = true;
        if (!m_bodies.isEmpty() && m_bodies[0]->isAlive()) {
            m_bodies[0]->kill();
        }
    }

    if (fuelEmpty || m_roofCrashLatched) {
        armGameOver();
    } else {
        disarmGameOver();
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

    const int camGX = m_cameraX / Constants::PIXEL_SIZE;
    const int camGY = m_cameraY / Constants::PIXEL_SIZE;
    const int offX = -(m_cameraX - camGX * Constants::PIXEL_SIZE);
    const int offY = (m_cameraY - camGY * Constants::PIXEL_SIZE);

    p.save();
    p.translate(offX, offY);

    if (m_showGrid) { drawGridOverlay(p); }
    drawClouds(p);
    drawFilledTerrain(p);
    m_fuelSys.drawWorldFuel(p, m_cameraX, m_cameraY);
    m_coinSys.drawWorldCoins(p, m_cameraX, m_cameraY, gridW(), gridH());
    m_nitroSys.drawFlame(p, m_wheels, m_cameraX, m_cameraY, width(), height());

    for (const Wheel* wheel : m_wheels) {
        if (auto info = wheel->get(0, 0, width(), height(), -m_cameraX, m_cameraY)) {
            const int cx = (*info)[0];
            const int cy = (*info)[1];
            const int r  = (*info)[2];
            if(r == 0) continue;
            const int gcx = cx / Constants::PIXEL_SIZE;
            const int gcy = cy / Constants::PIXEL_SIZE;
            const int gr  = r  / Constants::PIXEL_SIZE;
            drawCircleFilledMidpointGrid(p, gcx, gcy, gr, Constants::WHEEL_COLOR_OUTER);
            const int tyreCells = std::max(1, Constants::TYRE_THICKNESS / Constants::PIXEL_SIZE);
            const int innerR = std::max(1, gr - tyreCells);
            drawCircleFilledMidpointGrid(p, gcx, gcy, innerR, Constants::WHEEL_COLOR_INNER);
        }
    }

    for(CarBody* body : m_bodies){
        auto pts = body->get(-m_cameraX, m_cameraY);
        QVector<QPoint> normalisedPoints;
        normalisedPoints.reserve(pts.size());
        for(auto p2 : pts){
            normalisedPoints.append(QPoint(p2.x() / Constants::PIXEL_SIZE, p2.y() / Constants::PIXEL_SIZE));
        }
        fillPolygon(p, normalisedPoints, Constants::CAR_COLOR);

        for(CarBody* body : m_bodies){
            auto pts = body->get(-m_cameraX, m_cameraY);
            QVector<QPoint> normalisedPoints;
            normalisedPoints.reserve(pts.size());
            for(auto p2 : pts){
                normalisedPoints.append(QPoint(p2.x() / Constants::PIXEL_SIZE, p2.y() / Constants::PIXEL_SIZE));
            }
            fillPolygon(p, normalisedPoints, Constants::CAR_COLOR);

            auto attach = body->getAttachments(-m_cameraX, m_cameraY);
            for (const auto& ap : attach) {
                QVector<QPoint> norm;
                norm.reserve(ap.first.size());
                for (const QPoint& q : ap.first) {
                    norm.append(QPoint(q.x() / Constants::PIXEL_SIZE, q.y() / Constants::PIXEL_SIZE));
                }
                fillPolygon(p, norm, ap.second);
            }
        }
    }
    m_flip.drawWorldPopups(p, m_cameraX, m_cameraY, level_index);

    p.restore();

    drawHUDFuel(p);
    drawHUDCoins(p);
    m_nitroSys.drawHUD(p, m_elapsedSeconds, level_index);
    m_flip.drawHUD(p, level_index);
    drawHUDDistance(p);
    drawHUDScore(p);
    m_keylog.draw(p, width(), height(), Constants::PIXEL_SIZE);
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

void MainWindow::drawGridOverlay(QPainter& p) {
    p.save();
    QPen pen(QColor(140,140,140));
    pen.setWidth(1);
    p.setPen(pen);
    p.setBrush(Qt::NoBrush);
    for (int x = 0; x <= width(); x += Constants::PIXEL_SIZE) p.drawLine(x, 0, x, height());
    for (int y = 0; y <= height(); y += Constants::PIXEL_SIZE) p.drawLine(0, y, width(), y);
    p.restore();
}

void MainWindow::plotGridPixel(QPainter& p, int gx, int gy, const QColor& c) {
    if (gx < 0 || gy < 0 || gx >= gridW() + 1 || gy >= gridH() + 1) return;
    p.fillRect(gx * Constants::PIXEL_SIZE, gy * Constants::PIXEL_SIZE, Constants::PIXEL_SIZE, Constants::PIXEL_SIZE, c);
}

void MainWindow::drawCircleFilledMidpointGrid(QPainter& p, int gcx, int gcy, int gr, const QColor& c)
{
    int x = 0;
    int y = gr;
    int d = 1 - gr;
    auto span = [&](int cy, int xl, int xr) { for (int xg = xl; xg <= xr; ++xg) plotGridPixel(p, xg, cy, c); };
    while (y >= x) {
        span(gcy + y, gcx - x, gcx + x);
        span(gcy - y, gcx - x, gcx + x);
        span(gcy + x, gcx - y, gcx + y);
        span(gcy - x, gcx - y, gcx + y);
        ++x;
        if (d < 0) d += 2 * x + 1;
        else { --y; d += 2 * (x - y) + 1; }
    }
}

void MainWindow::fillPolygon(QPainter& p, QVector<QPoint> points, const QColor& c)
{
    if (points.size() < 3) return;

    struct EdgeEntry { int y_max; double x_at_min; double inv_slope; EdgeEntry(int ymax, double x_min, double inv_s) : y_max(ymax), x_at_min(x_min), inv_slope(inv_s) {} };
    struct ActiveEdge { int y_max; double x_curr; double inv_slope; ActiveEdge(const EdgeEntry& e) : y_max(e.y_max), x_curr(e.x_at_min), inv_slope(e.inv_slope) {} bool operator<(const ActiveEdge& other) const { return x_curr < other.x_curr; } };

    std::map<int, std::list<EdgeEntry>> edgeTable;
    int global_y_min = std::numeric_limits<int>::max();
    int global_y_max = std::numeric_limits<int>::min();

    for (int i = 0; i < points.size(); ++i) {
        const QPoint& p1 = points[i];
        const QPoint& p2 = points[(i + 1) % points.size()];

        global_y_min = std::min({global_y_min, p1.y(), p2.y()});
        global_y_max = std::max({global_y_max, p1.y(), p2.y()});

        const QPoint *v1 = &p1, *v2 = &p2;
        if (v1->y() > v2->y()) std::swap(v1, v2);
        if (v1->y() == v2->y()) continue;

        double invSlope = static_cast<double>(v2->x() - v1->x()) / (v2->y() - v1->y());
        edgeTable[v1->y()].emplace_back(v2->y(), v1->x(), invSlope);
    }

    std::list<ActiveEdge> activeEdgeTable;

    for (int y = global_y_min; y < global_y_max; ++y) {
        if (edgeTable.count(y)) for (const auto& edge : edgeTable[y]) activeEdgeTable.emplace_back(edge);
        activeEdgeTable.remove_if([y](const ActiveEdge& e){ return e.y_max == y; });
        activeEdgeTable.sort();

        for (auto it = activeEdgeTable.begin(); it != activeEdgeTable.end(); it++) {
            auto it_next = std::next(it);
            if (it_next == activeEdgeTable.end()) break;
            int x_start = static_cast<int>(std::ceil(it->x_curr));
            int x_end = static_cast<int>(std::floor(it_next->x_curr));
            for (int xg = x_start; xg <= x_end; ++xg) plotGridPixel(p, xg, y, c);
            ++it;
        }

        for (auto& edge : activeEdgeTable) edge.x_curr += edge.inv_slope;
    }
}

void MainWindow::rasterizeSegmentToHeightMapWorld(int x1,int y1,int x2,int y2){
    if (x2 < x1) { std::swap(x1,x2); std::swap(y1,y2); }

    const int gx1 = x1 / Constants::PIXEL_SIZE;
    const int gx2 = x2 / Constants::PIXEL_SIZE;

    if (x2 == x1) {
        const int gy = static_cast<int>(std::floor(y1 / double(Constants::PIXEL_SIZE) + 0.5));
        m_heightAtGX.insert(gx1, gy);
        return;
    }

    const double dx = double(x2 - x1);
    const double dy = double(y2 - y1);

    for (int gx = gx1; gx <= gx2; ++gx) {
        const double wx = gx * double(Constants::PIXEL_SIZE);
        double t = (wx - x1) / dx;
        t = std::clamp(t, 0.0, 1.0);

        const double wy = y1 + t * dy;
        const int gy = static_cast<int>(std::floor(wy / double(Constants::PIXEL_SIZE) + 0.5));

        m_heightAtGX.insert(gx, gy);
    }
}

void MainWindow::pruneHeightMap() {
    if (m_lines.isEmpty()) return;

    const int keepFromGX = (m_lines.first().getX1() / Constants::PIXEL_SIZE) - 4;
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
    case Qt::Key_Right:
        if (!m_accelerating) m_media->startAccelLoop();
        m_accelerating = true;
        m_keylog.setPressed(Qt::Key_D, true);
        break;

    case Qt::Key_A:
    case Qt::Key_Left:
        m_braking = true;
        m_keylog.setPressed(Qt::Key_A, true);
        break;

    case Qt::Key_W:
    case Qt::Key_Up:
        m_media->playNitroOnce();
        m_nitroKey = true;
        m_keylog.setPressed(Qt::Key_W, true);
        break;

    case Qt::Key_G:
        m_showGrid = !m_showGrid;
        break;

    case Qt::Key_P:
        if (!m_intro && !m_outro && m_timer && m_timer->isActive()) {
            m_timer->stop();
            if (m_pause) {
                m_pause->setLevelIndex(level_index);
                m_pause->showPaused();
            }
        }
        break;

    case Qt::Key_Escape:
        close();
        break;
    default:
        QWidget::keyPressEvent(event);
    }
}

void MainWindow::keyReleaseEvent(QKeyEvent *event) {
    if (event->isAutoRepeat()) return;

    switch (event->key()) {
    case Qt::Key_D:
    case Qt::Key_Right:
        m_accelerating = false;
        m_keylog.setPressed(Qt::Key_D, false);
        m_media->stopAccelLoop();
        break;
    case Qt::Key_A:
    case Qt::Key_Left:
        m_braking = false;
        m_keylog.setPressed(Qt::Key_A, false);
        break;
    case Qt::Key_W:
    case Qt::Key_Up:
        m_nitroKey = false;
        m_keylog.setPressed(Qt::Key_W, false);
        break;
    default:
        QWidget::keyReleaseEvent(event);
    }
}


double MainWindow::averageSpeed() const {
    if (m_wheels.isEmpty()) return 0.0;
    double s = 0.0;
    for (const Wheel* w : m_wheels) s += std::sqrt(w->m_vx*w->m_vx + w->m_vy*w->m_vy);
    return s / m_wheels.size();
}

void MainWindow::ensureAheadTerrain(int worldX) {
    while (m_lastX < worldX) {
        m_slope += (m_dist(m_rng) - static_cast<float>(m_lastY) / height()) * m_difficulty;
        m_slope = std::clamp(m_slope, -1.0f, 1.0f);

        const int newY = m_lastY + std::lround(m_slope * std::pow(std::abs(m_slope), m_irregularity) * m_step);

        Line seg(m_lastX, m_lastY, m_lastX + m_step, newY);
        m_lines.append(seg);
        rasterizeSegmentToHeightMapWorld(seg.getX1(), m_lastY, seg.getX2(), newY);

        m_lastY = newY;
        m_lastX += m_step;

        if (m_lines.size() > (width() / m_step) * 3) { m_lines.removeFirst(); pruneHeightMap(); }

        m_difficulty += m_difficultyIncrement;
        m_fuelSys.maybePlaceFuelAtEdge(m_lastX, m_heightAtGX, m_difficulty, m_elapsedSeconds);
        maybeSpawnCloud();
    }
}

void MainWindow::maybeSpawnCloud() {
    if (m_lastX - m_lastCloudSpawnX < Constants::CLOUD_SPACING_PX) return;
    if (m_dist(m_rng) > Constants::CLOUD_PROBABILITY[level_index]) return;

    int gx = m_lastX / Constants::PIXEL_SIZE;
    auto it = m_heightAtGX.constFind(gx);
    if (it == m_heightAtGX.constEnd()) return;

    int gyGround = it.value();

    std::uniform_int_distribution<int> wdist(Constants::CLOUD_MIN_W_CELLS, Constants::CLOUD_MAX_W_CELLS);
    std::uniform_int_distribution<int> hdist(Constants::CLOUD_MIN_H_CELLS, Constants::CLOUD_MAX_H_CELLS);

    int wCells = wdist(m_rng);
    int hCells = hdist(m_rng);

    int skyLift = Constants::CLOUD_SKY_OFFSET_CELLS + int(m_dist(m_rng) * 200);
    int cloudTopCells = gyGround - skyLift;

    Cloud cl;
    cl.wx = m_lastX;
    cl.wyCells = cloudTopCells;
    cl.wCells  = wCells;
    cl.hCells  = hCells;
    cl.seed = m_rng();

    m_clouds.append(cl);
    m_lastCloudSpawnX = m_lastX;

    int leftLimit = leftmostTerrainX() - width()*2;
    for (int i = 0; i < m_clouds.size(); ) {
        if (m_clouds[i].wx < leftLimit) m_clouds.removeAt(i);
        else ++i;
    }
}

void MainWindow::drawClouds(QPainter& p) {
    int camGX = m_cameraX / Constants::PIXEL_SIZE;
    int camGY = m_cameraY / Constants::PIXEL_SIZE;

    auto plotGridPixelLocal = [&](int gx, int gy, const QColor& c) {
        p.fillRect(gx * Constants::PIXEL_SIZE, gy * Constants::PIXEL_SIZE, Constants::PIXEL_SIZE, Constants::PIXEL_SIZE, c);
    };

    for (const Cloud& cl : m_clouds) {
        int baseGX = (cl.wx / Constants::PIXEL_SIZE) - camGX;
        int baseGY = cl.wyCells + camGY;

        for (int yy = 0; yy < cl.hCells; ++yy) {
            for (int xx = 0; xx < cl.wCells; ++xx) {
                double nx = ((xx + 0.5) - cl.wCells  / 2.0) / (cl.wCells  / 2.0);
                double ny = ((yy + 0.5) - cl.hCells / 2.0) / (cl.hCells / 2.0);
                double r2 = nx*nx + ny*ny;

                quint32 h = hash2D(int(cl.seed) + xx, yy);
                double fuzz = (h % 100) / 400.0;

                if (r2 <= 1.0 + fuzz) {
                    QColor cMain(255,255,255);
                    QColor cSoft(240,240,240);
                    QColor pix = ((h >> 3) & 1) ? cMain : cSoft;
                    plotGridPixelLocal(baseGX + xx, baseGY + yy, pix);
                }
            }
        }
    }
}

void MainWindow::drawFilledTerrain(QPainter& p) {
    const int camGX = m_cameraX / Constants::PIXEL_SIZE;
    const int camGY = m_cameraY / Constants::PIXEL_SIZE;

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
            bool topZone = (sGY < groundWorldGY + camGY + 3*Constants::SHADING_BLOCK);
            const QColor shade = grassShadeForBlock(worldGX, worldGY, topZone);
            plotGridPixel(p, sgx, sGY, shade);
        }

        const QColor edge = grassShadeForBlock(worldGX, groundWorldGY, true).darker(115);
        plotGridPixel(p, sgx, groundWorldGY + camGY, edge);
    }
}

QColor MainWindow::grassShadeForBlock(int worldGX, int worldGY, bool greenify) const {
    const int bx = worldGX / Constants::SHADING_BLOCK;
    const int by = worldGY / Constants::SHADING_BLOCK;
    const quint32 h = hash2D(bx, by);

    if (greenify) {
        const int idxG = int(h % m_grassPalette.size());
        return m_grassPalette[level_index][idxG];
    } else {
        const int idxD = int(h % m_dirtPalette.size());
        return m_dirtPalette[level_index][idxD];
    }
}

void MainWindow::drawHUDFuel(QPainter& p) {
    int gy = Constants::HUD_TOP_MARGIN;
    int wcells = std::min(std::max(gridW()/4, 24), 48);
    int gx = (gridW() - wcells)/2;
    int barH = 3;

    double frac = std::clamp(m_fuel / Constants::FUEL_MAX, 0.0, 1.0);
    int filled = int(std::floor(wcells * frac));

    auto lerp = [](const QColor& c1, const QColor& c2, double t)->QColor {
        int r = int((1-t)*c1.red()   + t*c2.red());
        int g = int((1-t)*c1.green() + t*c2.green());
        int b = int((1-t)*c1.blue()  + t*c2.blue());
        return QColor(r,g,b);
    };

    QColor startC(40,230,55), midC(250,230,80), endC(230,50,40);

    for (int x=0; x<wcells; ++x)
        for (int y=0; y<barH; ++y)
            plotGridPixel(p, gx+x, gy+y, QColor(20,14,24));

    for (int x=0; x<filled; ++x) {
        double t = double(x)/std::max(wcells-1,1);
        QColor c = (t<0.5) ? lerp(startC, midC, t*2) : lerp(midC, endC, (t-0.5)*2);
        for (int y=0; y<barH; ++y)
            plotGridPixel(p, gx+x, gy+y, c);
    }

    for (int y=0; y<barH; ++y) {
        plotGridPixel(p,gx-1,gy+y,QColor(35,35,48));
        plotGridPixel(p,gx+wcells,gy+y,QColor(35,35,48));
    }

    for (int x=-1; x<=wcells; ++x) {
        plotGridPixel(p,gx+x,gy-1,QColor(35,35,48));
        plotGridPixel(p,gx+x,gy+barH,QColor(35,35,48));
    }

    int tickEvery = std::max(6, wcells/6);

    for (int x=tickEvery; x<wcells; x+=tickEvery)
        plotGridPixel(p, gx+x, gy+barH, QColor(80,80,70));
    const double lowFuelThreshold = Constants::FUEL_MAX * 0.25;

    // 2. Check if fuel is low
    const bool isLow = (m_fuel <= lowFuelThreshold);

    // 3. Check flashing state (flashes every 1 second: 0.5s on, 0.5s off)
    const bool isFlashingOn = (std::fmod(m_elapsedSeconds, 1.0) < 0.5);

    // 4. Only draw the warning if fuel is low and it's the "on" part of the flash
    if (isLow && isFlashingOn) {
        const QColor red(230, 50, 40);
        const QColor white(255, 255, 255);

        // 5. Position the warning sign
        // (gy, barH, and gridW() are available from the start of this function)
        const int warningGap = 3; // 3 cells below the fuel bar's border
        const int warningTopGY = gy + barH + 1 + warningGap;

        // 6. Define sign layout and find centered start position
        // (5px tri + 2px space + 3px L + 1px space + 3px O + 1px space + 5px W)
        const int totalWidth = 5 + 2 + 3 + 1 + 3 + 1 + 5;
        int currentGX = (gridW() - totalWidth) / 2;

        // --- Draw Triangle (5 wide, 5 high) ---
        const int triTopGY = warningTopGY;
        //   R
        //  RWR
        // RRWRR
        //
        //   W
        plotGridPixel(p, currentGX + 2, triTopGY,     red);
        plotGridPixel(p, currentGX + 1, triTopGY + 1, red);
        plotGridPixel(p, currentGX + 2, triTopGY + 1, white); // !
        plotGridPixel(p, currentGX + 3, triTopGY + 1, red);
        plotGridPixel(p, currentGX,     triTopGY + 2, red);
        plotGridPixel(p, currentGX + 1, triTopGY + 2, red);
        plotGridPixel(p, currentGX + 2, triTopGY + 2, white); // !
        plotGridPixel(p, currentGX + 3, triTopGY + 2, red);
        plotGridPixel(p, currentGX + 4, triTopGY + 2, red);
        plotGridPixel(p, currentGX + 2, triTopGY + 4, white); // . (dot)

        currentGX += 5 + 2; // Advance past triangle and 2-cell space

        // --- Draw "LOW" (3-wide L, 3-wide O, 5-wide W) ---
        const int textTopGY = warningTopGY;

        // Draw "L" (3 wide)
        for(int y=0; y<5; ++y) plotGridPixel(p, currentGX, textTopGY+y, red);
        plotGridPixel(p, currentGX+1, textTopGY+4, red);
        plotGridPixel(p, currentGX+2, textTopGY+4, red);
        currentGX += 3 + 1; // Advance past L and 1-cell space

        // Draw "O" (3 wide)
        for(int y=0; y<5; ++y) {
            plotGridPixel(p, currentGX, textTopGY+y, red);
            plotGridPixel(p, currentGX+2, textTopGY+y, red);
        }
        plotGridPixel(p, currentGX+1, textTopGY, red);
        plotGridPixel(p, currentGX+1, textTopGY+4, red);
        currentGX += 3 + 1; // Advance past O and 1-cell space

        // Draw "W" (5 wide)
        for(int y=0; y<5; ++y) {
            plotGridPixel(p, currentGX, textTopGY+y, red);
            plotGridPixel(p, currentGX+4, textTopGY+y, red);
        }
        plotGridPixel(p, currentGX+1, textTopGY+3, red);
        plotGridPixel(p, currentGX+2, textTopGY+2, red);
        plotGridPixel(p, currentGX+3, textTopGY+3, red);
    }
}

void MainWindow::drawHUDCoins(QPainter& p) {
    int iconGX = Constants::HUD_LEFT_MARGIN + Constants::COIN_RADIUS_CELLS + 1;
    int iconGY = Constants::HUD_TOP_MARGIN  + Constants::COIN_RADIUS_CELLS;
    drawCircleFilledMidpointGrid(p, iconGX, iconGY, Constants::COIN_RADIUS_CELLS, QColor(195,140,40));
    drawCircleFilledMidpointGrid(p, iconGX, iconGY, std::max(1, Constants::COIN_RADIUS_CELLS-1), QColor(250,204,77));
    plotGridPixel(p, iconGX-1, iconGY-Constants::COIN_RADIUS_CELLS+1, QColor(255,255,220));
    QFont f; f.setFamily("Monospace"); f.setBold(true); f.setPointSize(12);
    p.setFont(f);
    p.setPen(Constants::TEXT_COLOR[level_index]);
    int px = (Constants::HUD_LEFT_MARGIN + Constants::COIN_RADIUS_CELLS*2 + 3) * Constants::PIXEL_SIZE;
    int py = (Constants::HUD_TOP_MARGIN  + Constants::COIN_RADIUS_CELLS + 2) * Constants::PIXEL_SIZE;
    p.drawText(px, py, QString::number(m_coinCount));
}

void MainWindow::drawHUDDistance(QPainter& p) {
    double meters = (m_totalDistanceCells * Constants::PIXEL_SIZE) / 100.0;
    QString s = QString::number(meters, 'f', 1) + " m";
    QFont f; f.setFamily("Monospace"); f.setBold(true); f.setPointSize(12);
    p.setFont(f);
    p.setPen(Constants::TEXT_COLOR[level_index]);
    QFontMetrics fm(f);
    int px = width() - fm.horizontalAdvance(s) - 12;
    int py = (Constants::HUD_TOP_MARGIN + Constants::COIN_RADIUS_CELLS + 2) * Constants::PIXEL_SIZE;
    p.drawText(px, py, s);
}


void MainWindow::drawHUDScore(QPainter& p) {
    const QString s = QString::number(m_score);
    QFont f; f.setFamily("Monospace"); f.setBold(true); f.setPointSize(12);
    p.setFont(f);
    p.setPen(Constants::TEXT_COLOR[level_index]);
    QFontMetrics fm(f);
    const int rightPadPx = 12;
    const int px = width() - fm.horizontalAdvance(s) - rightPadPx;
    const int distancePy = (Constants::HUD_TOP_MARGIN + Constants::COIN_RADIUS_CELLS + 2) * Constants::PIXEL_SIZE;
    const int gapPx = 8;
    const int py = distancePy - fm.height() - gapPx;
    p.drawText(px, py, s);
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
    int gx = int(wx / Constants::PIXEL_SIZE);
    int gyL = groundGyNearestGX(gx - 1);
    int gyR = groundGyNearestGX(gx + 1);
    double dyCells = double(gyR - gyL);
    return std::atan2(-dyCells, 2.0);
}

void MainWindow::showGameOver() {
    if (m_media) m_media->playGameOverOnce();
    if (m_outro) return;
    if (m_timer) m_timer->stop();

    m_outro = new OutroScreen(this);
    m_outro->setStats(m_coinCount, m_nitroUses, m_score, (m_totalDistanceCells * Constants::PIXEL_SIZE) / 100.0);
    m_outro->setFlips(m_flip.total());
    m_outro->show();
    m_outro->raise();

    connect(m_outro, &OutroScreen::restartRequested, this, [this]{
        if (m_outro) {
            m_outro->hide();
            m_outro->deleteLater();
            m_outro = nullptr;
        }

        m_grandTotalCoins += m_coinCount;
        saveGrandCoins();

        resetGameRound();
        m_pause->hide();
        m_roofCrashLatched = false;
        m_gameOverArmed = false;
        if (m_timer) {
            m_clock.restart();
            m_timer->start(16);
        }
        setFocus();
    });

    connect(m_outro, &OutroScreen::exitRequested, this, [this]{ returnToIntro(); });
}

void MainWindow::returnToIntro() {
    loadGrandCoins();
    ++m_sessionId;
    m_gameOverArmed = false;
    m_roofCrashLatched = false;

    if (m_timer) m_timer->stop();

    m_grandTotalCoins += m_coinCount;
    saveGrandCoins();

    if (m_outro) {
        m_outro->hide();
        m_outro->deleteLater();
        m_outro = nullptr;
    }

    m_accelerating = m_braking = m_nitroKey = false;
    m_prevNitroActive = false;

    m_intro = new IntroScreen(this, level_index);
    m_intro->setGeometry(rect());
    m_intro->setGrandCoins(m_grandTotalCoins);
    m_intro->show();

    connect(m_intro, &IntroScreen::exitRequested, this, &QWidget::close);
    connect(m_intro, &IntroScreen::startRequested, this, [this](int levelIndex){
        if (m_intro) { m_intro->hide(); m_intro->deleteLater(); m_intro = nullptr; }
        level_index = levelIndex;
        resetGameRound();
        setFocus();
        if (m_timer) m_timer->start();
    });
}

bool MainWindow::isFullyUpsideDown() const {
    if (m_wheels.size() < 2) return false;
    const double dx = m_wheels[1]->x - m_wheels[0]->x;
    const double dy = m_wheels[1]->y - m_wheels[0]->y;
    const double len = std::hypot(dx, dy) + 1e-9;
    const double c = dx / len;
    const double s = dy / len;
    return (c <= Constants::FLIPPED_COS_MIN && std::abs(s) <= Constants::FLIPPED_SIN_MAX);
}

bool MainWindow::isRoofTouchingTerrain() const {
    if (m_bodies.isEmpty()) return false;
    const auto probes = m_bodies.front()->getKillSwitches(0, 0);
    const double tol = 0.5 * Constants::PIXEL_SIZE;
    int onGround = 0;
    for (const QPoint& pt : probes) {
        const int gx = int(std::lround(double(pt.x()) / Constants::PIXEL_SIZE));
        const int gyGround = groundGyNearestGX(gx);
        const double wyGround = gyGround * Constants::PIXEL_SIZE;
        if (pt.y() >= wyGround - tol) ++onGround;
    }
    return onGround >= 1;
}

void MainWindow::armGameOver() {
    if (m_gameOverArmed || m_outro) return;
    m_gameOverArmed = true;
    const int thisSession = m_sessionId;
    QTimer::singleShot(Constants::GAME_OVER_DELAY_MS, this, [this, thisSession]{
        if (!m_gameOverArmed) return;
        if (thisSession != m_sessionId) return;
        showGameOver();
    });
}

void MainWindow::disarmGameOver() {
    m_gameOverArmed = false;
}

void MainWindow::resetGameRound() {
    loadGrandCoins();
    QPalette pal = palette();
    pal.setColor(QPalette::Window, Constants::SKY_COLOR[level_index]);
    setAutoFillBackground(true);
    setPalette(pal);

    m_gameOverArmed = false;
    m_roofCrashLatched  = false;
    ++m_sessionId;

    m_nitroSys = NitroSystem();
    m_fuelSys  = FuelSystem();
    m_coinSys  = CoinSystem();

    m_fuel = Constants::FUEL_MAX;
    m_coinCount = 0;
    m_nitroUses = 0;
    m_elapsedSeconds= 0.0;

    m_camX = m_camY = m_camVX = m_camVY = 0.0;
    m_cameraX = 0; m_cameraY = 200; m_cameraXFarthest = 0;

    m_lines.clear();
    m_heightAtGX.clear();
    m_lastX = 0; m_lastY = 0; m_slope = 0.0f; m_difficulty = 0.005f;
    generateInitialTerrain();

    m_clouds.clear();
    m_lastCloudSpawnX = 0;

    qDeleteAll(m_wheels); m_wheels.clear();
    qDeleteAll(m_bodies); m_bodies.clear();

    Wheel* w1 = new Wheel(100, 300, 20);
    Wheel* w2 = new Wheel(220, 300, 20);
    Wheel* w3 = new Wheel(160, 300, 0);
    w1->attach(w2); w1->attach(w3); w2->attach(w3);
    m_wheels.append(w1); m_wheels.append(w2); m_wheels.append(w3);

    CarBody* body = new CarBody();
    auto bodyPoints = Constants::bodyPoints;
    body->addPoints(bodyPoints);
    auto hitboxPoints = Constants::hitboxPoints;
    body->addHitbox(hitboxPoints);
    auto killPoints = Constants::killPoints;
    body->addKillSwitches(killPoints);
    body->addWheel(w1);
    body->addWheel(w2);
    body->addWheel(w3);
    body->addAttachment(Constants::window, Constants::WINDOW_COLOR);
    body->addAttachment(Constants::handle, Constants::HANDLE_COLOR);
    body->finish();
    m_bodies.append(body);

    m_totalDistanceCells = 0.0;
    m_score = 0;

    {
        double ax = 0.0;
        for (const Wheel* w : m_wheels) ax += w->x;
        m_lastScoreX = m_wheels.isEmpty() ? 0.0 : ax / m_wheels.size();
    }

    m_accelerating = m_braking = m_nitroKey = false;
    m_prevNitroActive = false;
    m_flip.reset();

    m_clock.restart();
}

void MainWindow::saveGrandCoins() const {
    QSettings s("JU","F1PixelGrid");
    s.setValue("grandCoins", m_grandTotalCoins);
    s.sync();
}

void MainWindow::loadGrandCoins() {
    QSettings s("JU","F1PixelGrid");
    m_grandTotalCoins = s.value("grandCoins", 0).toInt();
}

void MainWindow::closeEvent(QCloseEvent* e) {
    saveGrandCoins();
    QWidget::closeEvent(e);
}

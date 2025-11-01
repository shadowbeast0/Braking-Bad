#include "mainwindow.h"
#include <QPainter>
#include <QKeyEvent>
#include <QPalette>
#include <QFont>
#include <cmath>
#include <algorithm>

MainWindow::MainWindow(QWidget *parent)
    : QWidget(parent),
    m_rng(std::random_device{}()),
    m_dist(0.0f, 1.0f)
{
    setWindowTitle("Driver (Pixel Grid)");
    setFocusPolicy(Qt::StrongFocus);

    // Background
    QPalette pal = palette();
    pal.setColor(QPalette::Window, Qt::black);
    setAutoFillBackground(true);
    setPalette(pal);

    m_grassPalette = {
        QColor( 92, 173,  83),
        QColor( 99, 181,  90),
        QColor(106, 189,  98),
        QColor( 83, 164,  75)
    };

    showFullScreen();

    // Centralized game setup into a restartable function
    restartGame();

    // Timer
    m_timer = new QTimer(this);
    connect(m_timer, &QTimer::timeout, this, &MainWindow::gameLoop);
    m_timer->start(5);

    m_clock.start();
}

MainWindow::~MainWindow() {
    // Ensure car is also deleted
    qDeleteAll(m_wheels);
    delete m_car;
}

void MainWindow::restartGame() {
    m_gameOver = false;
    m_lines.clear();
    m_heightAtGX.clear();
    qDeleteAll(m_wheels);
    m_wheels.clear();
    delete m_car;
    m_car = nullptr;

    m_slope = 0.0f;
    m_difficulty = 0.01f;
    m_cameraXFarthest = 0;

    generateInitialTerrain();

    const int wheelRadius = 20;
    const int worldBase = qRound(Car::IMG_WHEEL_BASE * (double(wheelRadius) / Car::IMG_WHEEL_RADIUS));
    Wheel* backWheel  = new Wheel(100, 300, wheelRadius);
    Wheel* frontWheel = new Wheel(100 + worldBase, 300, wheelRadius);
    backWheel->attach(frontWheel);
    m_wheels.append(backWheel);
    m_wheels.append(frontWheel);

    m_car = new Car(backWheel, frontWheel);

    m_camX = 100 - 200;
    m_camY = -300 + height() / 2.0;
    m_camVX = 0;
    m_camVY = 0;
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

    if (!m_gameOver) {
        double avgX = 0.0, avgY = 0.0;
        if (!m_wheels.isEmpty()) {
            for (const Wheel* w : m_wheels) { avgX += w->x; avgY += w->y; }
            avgX /= m_wheels.size(); avgY /= m_wheels.size();
        }
        const double targetX = avgX - 200.0;
        const double targetY = -avgY + height() / 2.0;

        updateCamera(targetX, targetY, dt);

        if (targetX > m_cameraXFarthest) {
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
        }

        for (Wheel* w : m_wheels) w->simulate(m_lines, m_accelerating, m_braking);

        if (m_car) {
            m_car->update(m_accelerating, m_braking);
            if (m_car->isFlipped()) {
                m_gameOver = true;
            }
        }
    }
    update();
}

void MainWindow::paintEvent(QPaintEvent *event) {
    Q_UNUSED(event);
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, false);
    p.setPen(Qt::NoPen);

    m_cameraX = int(std::lround(m_camX));
    m_cameraY = int(std::lround(m_camY));

    const int camGX = m_cameraX / PIXEL_SIZE;
    const int camGY = m_cameraY / PIXEL_SIZE;
    const int offX  = -(m_cameraX - camGX * PIXEL_SIZE);
    const int offY  =  (m_cameraY - camGY * PIXEL_SIZE);

    p.save();
    p.translate(offX, offY);
    if (m_showGrid) drawGridOverlay(p);
    drawFilledTerrain(p);
    p.restore();

    if (m_car) {
        m_car->draw(p, m_cameraX, m_cameraY);
    }

    if (m_gameOver) {
        p.setPen(Qt::red);
        p.setFont(QFont("Arial", 50, QFont::Bold));
        p.drawText(rect(), Qt::AlignCenter, "FLIPPED!");

        p.setPen(Qt::white);
        p.setFont(QFont("Arial", 20, QFont::Normal));
        p.drawText(rect().translated(0, 60), Qt::AlignCenter, "Press 'R' to Restart");
    }
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

    for (int sgx = 0; sgx < gridW(); ++sgx) {
        const int worldGX = sgx + camGX;
        auto it = m_heightAtGX.constFind(worldGX);
        if (it == m_heightAtGX.constEnd()) continue;

        const int groundWorldGY = it.value();
        int startScreenGY = groundWorldGY + camGY;

        if (startScreenGY < 0) startScreenGY = 0;
        if (startScreenGY >= gridH()) continue;

        for (int sGY = startScreenGY; sGY < gridH(); ++sGY) {
            const int worldGY = sGY - camGY;
            const QColor shade = grassShadeForBlock(worldGX, worldGY);
            plotGridPixel(p, sgx, sGY, shade);
        }

        const QColor edge = grassShadeForBlock(worldGX, groundWorldGY).darker(115);
        plotGridPixel(p, sgx, startScreenGY, edge);
    }
}

QColor MainWindow::grassShadeForBlock(int worldGX, int worldGY) const {
    const int bx = worldGX / SHADING_BLOCK;
    const int by = worldGY / SHADING_BLOCK;
    const quint32 h = hash2D(bx, by);
    const int idx = int(h % m_grassPalette.size());
    return m_grassPalette[idx];
}

void MainWindow::drawGridOverlay(QPainter& p) {
    p.save();
    QPen pen(QColor(40, 40, 40));
    pen.setWidth(1);
    p.setPen(pen);
    p.setBrush(Qt::NoBrush);
    for (int x = 0; x <= width(); x += PIXEL_SIZE) p.drawLine(x, 0, x, height());
    for (int y = 0; y <= height(); y += PIXEL_SIZE) p.drawLine(0, y, width(), y);
    p.restore();
}

void MainWindow::plotGridPixel(QPainter& p, int gx, int gy, const QColor& c) {
    if (gx < 0 || gy < 0 || gx >= gridW() || gy >= gridH()) return;
    p.fillRect(gx * PIXEL_SIZE, gy * PIXEL_SIZE, PIXEL_SIZE, PIXEL_SIZE, c);
}

void MainWindow::drawCircleFilledMidpointGrid(QPainter& p, int gcx, int gcy, int gr, const QColor& c) {
    int x = 0;
    int y = gr;
    int d = 1 - gr;
    auto span = [&](int cy, int xl, int xr) {
        for (int xg = xl; xg <= xr; ++xg) plotGridPixel(p, xg, cy, c);
    };
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
    const double dy = double(y2 - y1); // MODIFIED: Corrected the incomplete line

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

    if (event->key() == Qt::Key_R && m_gameOver) {
        restartGame();
        return;
    }

    if (!m_gameOver) {
        switch (event->key()) {
        case Qt::Key_D: m_accelerating = true; break;
        case Qt::Key_A: m_braking = true;      break;
        }
    }

    switch (event->key()) {
    case Qt::Key_G: m_showGrid = !m_showGrid; break;
    case Qt::Key_Escape: close(); break;
    default: QWidget::keyPressEvent(event);
    }
}

void MainWindow::keyReleaseEvent(QKeyEvent *event) {
    if (event->isAutoRepeat()) return;
    switch (event->key()) {
    case Qt::Key_D: m_accelerating = false; break;
    case Qt::Key_A: m_braking = false;      break;
    default: QWidget::keyReleaseEvent(event);
    }
}

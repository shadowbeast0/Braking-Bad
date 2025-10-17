#include "mainwindow.h"
#include <QPainter>
#include <QKeyEvent>
#include <QPalette>
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

    // Pleasant “Minecraft-ish” grass palette
    m_grassPalette = {
        QColor( 92, 173,  83),  // base
        QColor( 99, 181,  90),  // brighter
        QColor(106, 189,  98),  // brighter+
        QColor( 83, 164,  75)   // darker
    };

    showFullScreen();

    generateInitialTerrain();

    // Two sprung wheels (as before)
    Wheel* w1 = new Wheel(100, 300, 20);
    Wheel* w2 = new Wheel(200, 300, 20);
    w1->attach(w2);
    m_wheels.append(w1);
    m_wheels.append(w2);

    // Timer
    m_timer = new QTimer(this);
    connect(m_timer, &QTimer::timeout, this, &MainWindow::gameLoop);
    m_timer->start(5);

    // Seed smoothed camera so we don’t “jump” on first frame
    m_camX = m_cameraX;
    m_camY = m_cameraY;
    m_clock.start();
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
    // --- dt (seconds) from high-res timer ---
    const qint64 now = m_clock.nsecsElapsed();
    static qint64 prev = now;
    const qint64 dtns = now - prev;
    prev = now;
    const double dt = std::clamp(dtns / 1e9, 0.001, 0.033); // clamp to 1–33 ms

    // --- Raw camera target from wheels (unchanged logic) ---
    double avgX = 0.0, avgY = 0.0;
    if (!m_wheels.isEmpty()) {
        for (const Wheel* w : m_wheels) { avgX += w->x; avgY += w->y; }
        avgX /= m_wheels.size(); avgY /= m_wheels.size();
    }
    const double targetX = avgX - 200.0;
    const double targetY = -avgY + height() / 2.0;

    // --- Smoothed camera follows target (critically-damped spring) ---
    updateCamera(targetX, targetY, dt);
    m_cameraX = int(std::lround(m_camX));
    m_cameraY = int(std::lround(m_camY));

    // --- Terrain extension uses RAW target to avoid lag ---
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

        // prune old
        if (m_lines.size() > (width() / m_step) * 3) {
            m_lines.removeFirst();
            pruneHeightMap();
        }
        m_difficulty += m_difficultyIncrement;
    }

    // --- Physics step (unchanged) ---
    for (Wheel* w : m_wheels) w->simulate(m_lines, m_accelerating, m_braking);

    update();
}

void MainWindow::paintEvent(QPaintEvent *event) {
    Q_UNUSED(event);
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, false);
    p.setPen(Qt::NoPen);

    // ===== Smooth scroll over grid steps =====
    // Grid indices
    const int camGX = m_cameraX / PIXEL_SIZE;
    const int camGY = m_cameraY / PIXEL_SIZE;
    // Residual pixel offset inside the current grid cell (screen pixels)
    const int offX  = -(m_cameraX - camGX * PIXEL_SIZE);
    const int offY  =  (m_cameraY - camGY * PIXEL_SIZE);

    p.save();
    p.translate(offX, offY); // shift world by residual → smooth 1-px motion

    if (m_showGrid) drawGridOverlay(p);

    // Filled ground
    drawFilledTerrain(p);

    // Wheels above terrain (use optional return from Wheel::get)
    const QColor wheelColor(255, 255, 255);
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
}

/* =========================
   Camera smoothing helper
   ========================= */
void MainWindow::updateCamera(double tx, double ty, double dt) {
    // 2nd-order critically damped spring:
    // x'' + 2 ζ ω x' + ω² x = ω² target
    const double wn = m_camWN;
    const double z  = m_camZeta;

    // X axis
    const double ax = wn*wn * (tx - m_camX) - 2.0*z*wn * m_camVX;
    m_camVX += ax * dt;
    m_camX  += m_camVX * dt;

    // Y axis
    const double ay = wn*wn * (ty - m_camY) - 2.0*z*wn * m_camVY;
    m_camVY += ay * dt;
    m_camY  += m_camVY * dt;
}

/* =========================
   Filled terrain rendering
   ========================= */
void MainWindow::drawFilledTerrain(QPainter& p) {
    const int camGX = m_cameraX / PIXEL_SIZE;
    const int camGY = m_cameraY / PIXEL_SIZE;

    for (int sgx = 0; sgx < gridW(); ++sgx) {
        const int worldGX = sgx + camGX;
        auto it = m_heightAtGX.constFind(worldGX);
        if (it == m_heightAtGX.constEnd()) continue;

        const int groundWorldGY = it.value();      // world-grid y at surface
        int startScreenGY = groundWorldGY + camGY; // screen-grid y

        if (startScreenGY < 0) startScreenGY = 0;
        if (startScreenGY >= gridH()) continue;

        // Fill from ground down to bottom with uniform shade blocks
        for (int sGY = startScreenGY; sGY < gridH(); ++sGY) {
            const int worldGY = sGY - camGY;
            const QColor shade = grassShadeForBlock(worldGX, worldGY);
            plotGridPixel(p, sgx, sGY, shade);
        }

        // 1-cell darker “lip” at the surface
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

/* =========================
   Pixel grid helpers
   ========================= */
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

/* =========================
   Height-map build/maintenance
   ========================= */
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

/* =========================
   Input
   ========================= */
void MainWindow::keyPressEvent(QKeyEvent *event) {
    if (event->isAutoRepeat()) return;
    switch (event->key()) {
    case Qt::Key_D: m_accelerating = true; break; // swapped controls
    case Qt::Key_A: m_braking = true;     break;
    case Qt::Key_G: m_showGrid = !m_showGrid; break;
    case Qt::Key_Escape: close(); break;
    default: QWidget::keyPressEvent(event);
    }
}

void MainWindow::keyReleaseEvent(QKeyEvent *event) {
    if (event->isAutoRepeat()) return;
    switch (event->key()) {
    case Qt::Key_D: m_accelerating = false; break;
    case Qt::Key_A: m_braking = false;     break;
    default: QWidget::keyReleaseEvent(event);
    }
}

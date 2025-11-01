#include "intro.h"
#include <QPainter>
#include <QMouseEvent>
#include <QImage>
#include <array>
#include <cmath>
#include <algorithm>

static const QHash<QChar, std::array<uint8_t,7>> k5x7 = {
    {'A',{0x1E,0x11,0x11,0x1F,0x11,0x11,0x11}},
    {'C',{0x0E,0x11,0x10,0x10,0x10,0x11,0x0E}},
    {'E',{0x1F,0x10,0x10,0x1E,0x10,0x10,0x1F}},
    {'I',{0x1F,0x04,0x04,0x04,0x04,0x04,0x1F}},
    {'N',{0x11,0x19,0x15,0x13,0x11,0x11,0x11}},
    {'R',{0x1E,0x11,0x11,0x1E,0x14,0x12,0x11}},
    {'S',{0x0F,0x10,0x10,0x0E,0x01,0x01,0x1E}},
    {'T',{0x1F,0x04,0x04,0x04,0x04,0x04,0x04}},
    {'X',{0x11,0x0A,0x04,0x04,0x0A,0x11,0x11}},
    {' ',{0x00,0x00,0x00,0x00,0x00,0x00,0x00}}
};

IntroScreen::IntroScreen(QWidget* parent) : QWidget(parent) {
    setAttribute(Qt::WA_OpaquePaintEvent);
    m_grassPalette = {
        QColor(100,140,50), QColor(115,155,60), QColor(130,170,80),
        QColor(85,120,40),  QColor(100,140,50), QColor(115,155,60),
        QColor(130,170,80), QColor(115,155,60), QColor(130,170,80),
        QColor(85,120,40)
    };
    m_dirtPalette = {
        QColor(125,105,40), QColor(135,117,50), QColor(145,125,60),
        QColor(150,120,50), QColor(120,100,30), QColor(145,125,60),
        QColor(150,120,50), QColor(125,105,40), QColor(135,117,50),
        QColor(120,100,30)
    };

    connect(&m_timer, &QTimer::timeout, this, [this]{
        m_scrollX += 2.0;
        m_camX = int(m_scrollX);
        while ((m_camX + width()) > m_camXFarthest) {
            m_camXFarthest += STEP;
            m_slope += (0.5f - float(m_lastY) / std::max(1,height())) * m_difficulty;
            m_slope = std::clamp(m_slope, -1.0f, 1.0f);
            const int newY = m_lastY + std::lround(m_slope * std::pow(std::abs(m_slope), 0.02f) * STEP);
            Line seg(m_lastX, m_lastY, m_lastX + STEP, newY);
            m_lines.append(seg);
            rasterizeSegmentToHeightMapWorld(seg.getX1(), m_lastY, seg.getX2(), newY);
            m_lastY = newY;
            m_lastX += STEP;
            if (m_lines.size() > (width() / STEP) * 3) { m_lines.removeFirst(); pruneHeightMap(); }
            m_difficulty += DIFF_INC;
        }
        update();
    });

    m_lastY = height() / 2;
    for (int i = STEP; i <= width() + STEP; i += STEP) {
        m_slope += (0.5f - float(m_lastY) / std::max(1,height())) * m_difficulty;
        m_slope = std::clamp(m_slope, -1.0f, 1.0f);
        const int newY = m_lastY + std::lround(m_slope * std::pow(std::abs(m_slope), 0.02f) * STEP);
        Line seg(i - STEP, m_lastY, i, newY);
        m_lines.append(seg);
        rasterizeSegmentToHeightMapWorld(seg.getX1(), m_lastY, seg.getX2(), newY);
        m_lastY = newY;
        m_difficulty += DIFF_INC;
    }
    m_lastX = STEP * m_lines.size();
    m_timer.start(16);
}

int IntroScreen::titleScale() const {
    int s = std::clamp(gridH() / (7 * 6), 2, 4);
    return s;
}

int IntroScreen::titleYCells() const {
    int ts = titleScale();
    return (gridH() - 7 * ts) / 2;
}

int IntroScreen::startTopCells(int tScale) const {
    int titleH = 7 * tScale;
    int gap = std::max(6, gridH() / 24);
    return titleYCells() + titleH + gap;
}

int IntroScreen::exitTopCells(int btnHCells) const {
    int gap = std::max(5, gridH() / 28);
    return startTopCells(titleScale()) + btnHCells + gap;
}

void IntroScreen::resizeEvent(QResizeEvent*) {
    m_camXFarthest = m_camX;
}

int IntroScreen::textWidthCells(const QString& s, int scale) const {
    if (s.isEmpty()) return 0;
    return (int(s.size()) - 1) * 6 * scale + 5 * scale;
}

int IntroScreen::fitTextScaleToRect(int wCells, int hCells, const QString& s) const {
    int padX = 4;
    int padY = 2;
    int maxScale = 10;
    int wcap = std::max(1, (wCells - padX) / std::max(1, textWidthCells(s, 1)));
    int hcap = std::max(1, (hCells - padY) / 7);
    int sc   = std::min(wcap, hcap);
    sc = std::clamp(sc, 1, maxScale);
    return sc;
}

void IntroScreen::paintEvent(QPaintEvent*) {
    QImage bg(size(), QImage::Format_ARGB32_Premultiplied);
    bg.fill(QColor(100,210,255));
    {
        QPainter pb(&bg);
        drawBackground(pb);
    }
    int sw = std::max(1, int(width() * m_blurScale));
    int sh = std::max(1, int(height() * m_blurScale));
    QImage small = bg.scaled(sw, sh, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    QPainter p(this);
    p.drawImage(rect(), small, small.rect());

    const QString title = "TERRAIN RACER";
    int ts = titleScale();
    int titleWCells = textWidthCells(title, ts);
    int tgx = (gridW() - titleWCells) / 2;
    int tgy = titleYCells();
    drawPixelText(p, title, tgx, tgy, ts, QColor(20,24,28), true);

    QRect rStart = buttonRectStart();
    QRect rExit  = buttonRectExit();

    p.setPen(Qt::NoPen);
    p.setBrush(QColor(0,0,0,160));
    p.drawRect(rStart.translated(3*PIXEL_SIZE,3*PIXEL_SIZE));
    p.drawRect(rExit.translated(3*PIXEL_SIZE,3*PIXEL_SIZE));

    p.setBrush(QColor(240,190,60));
    p.drawRect(rStart);
    p.setBrush(QColor(200,80,90));
    p.drawRect(rExit);

    QString sStart = "START";
    QString sExit  = "EXIT";

    int rStartWc = rStart.width()  / PIXEL_SIZE;
    int rStartHc = rStart.height() / PIXEL_SIZE;
    int rExitWc  = rExit.width()   / PIXEL_SIZE;
    int rExitHc  = rExit.height()  / PIXEL_SIZE;

    int bsStart = fitTextScaleToRect(rStartWc, rStartHc, sStart);
    int bsExit  = fitTextScaleToRect(rExitWc,  rExitHc,  sExit);

    int sWCells = textWidthCells(sStart, bsStart);
    int eWCells = textWidthCells(sExit,  bsExit);

    int sGX = rStart.left()/PIXEL_SIZE + (rStartWc - sWCells)/2;
    int sGY = rStart.top()/PIXEL_SIZE  + (rStartHc - 7*bsStart)/2;
    int eGX = rExit.left()/PIXEL_SIZE  + (rExitWc  - eWCells)/2;
    int eGY = rExit.top()/PIXEL_SIZE   + (rExitHc  - 7*bsExit)/2;

    drawPixelText(p, sStart, sGX, sGY, bsStart, QColor(25,20,24), false);
    drawPixelText(p, sExit,  eGX, eGY, bsExit,  QColor(25,20,24), false);
}

void IntroScreen::mousePressEvent(QMouseEvent* e) {
    if (buttonRectStart().contains(e->pos())) { emit startRequested(); return; }
    if (buttonRectExit().contains(e->pos()))  { emit exitRequested();  return; }
}

QRect IntroScreen::buttonRectStart() const {
    int wCells = std::min(std::max(gridW()/6, 30), 50);
    int hCells = std::max(10, gridH()/18);
    int gx = (gridW() - wCells) / 2;
    int gy = startTopCells(titleScale());
    return QRect(gx*PIXEL_SIZE, gy*PIXEL_SIZE, wCells*PIXEL_SIZE, hCells*PIXEL_SIZE);
}

QRect IntroScreen::buttonRectExit() const {
    int wCells = std::min(std::max(gridW()/6, 30), 50);
    int hCells = std::max(10, gridH()/18);
    int gx = (gridW() - wCells) / 2;
    int gy = exitTopCells(hCells);
    return QRect(gx*PIXEL_SIZE, gy*PIXEL_SIZE, wCells*PIXEL_SIZE, hCells*PIXEL_SIZE);
}

void IntroScreen::drawBackground(QPainter& p) {
    p.fillRect(rect(), QColor(100,210,255));
    drawFilledTerrain(p);
}

void IntroScreen::drawFilledTerrain(QPainter& p) {
    const int camGX = m_camX / PIXEL_SIZE;
    const int camGY = m_camY / PIXEL_SIZE;
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
            bool topZone = (sGY < groundWorldGY + camGY + 3*SHADING_BLOCK);
            const QColor shade = grassShadeForBlock(worldGX, worldGY, topZone);
            plotGridPixel(p, sgx, sGY, shade);
        }
        const QColor edge = grassShadeForBlock(worldGX, groundWorldGY, true).darker(115);
        plotGridPixel(p, sgx, groundWorldGY + camGY, edge);
    }
}

QColor IntroScreen::grassShadeForBlock(int worldGX, int worldGY, bool greenify) const {
    auto hash2D = [](int x, int y)->quint32{
        quint32 h = 120003212u;
        h ^= quint32(x); h *= 16777619u;
        h ^= quint32(y); h *= 16777619u;
        return (h ^ x) / (h ^ y) + (x * y) - (3 * x*x + 4 * y*y);
    };
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

void IntroScreen::plotGridPixel(QPainter& p, int gx, int gy, const QColor& c) {
    if (gx < 0 || gy < 0 || gx >= gridW()+1 || gy >= gridH()+1) return;
    p.fillRect(gx * PIXEL_SIZE, gy * PIXEL_SIZE, PIXEL_SIZE, PIXEL_SIZE, c);
}

void IntroScreen::rasterizeSegmentToHeightMapWorld(int x1,int y1,int x2,int y2){
    if (x2 < x1) { std::swap(x1,x2); std::swap(y1,y2); }
    const int gx1 = x1 / PIXEL_SIZE;
    const int gx2 = x2 / PIXEL_SIZE;
    if (x2 == x1) {
        const int gy = int(std::floor(y1 / double(PIXEL_SIZE) + 0.5));
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
        const int gy = int(std::floor(wy / double(PIXEL_SIZE) + 0.5));
        m_heightAtGX.insert(gx, gy);
    }
}

void IntroScreen::pruneHeightMap() {
    if (m_lines.isEmpty()) return;
    const int keepFromGX = (m_lines.first().getX1() / PIXEL_SIZE) - 4;
    QList<int> toRemove;
    toRemove.reserve(m_heightAtGX.size());
    for (auto it = m_heightAtGX.constBegin(); it != m_heightAtGX.constEnd(); ++it) {
        if (it.key() < keepFromGX) toRemove.append(it.key());
    }
    for (int k : toRemove) m_heightAtGX.remove(k);
}

void IntroScreen::ensureAheadTerrain(int worldX) {
    while (m_lastX < worldX) {
        m_slope += (0.5f - float(m_lastY) / std::max(1,height())) * m_difficulty;
        m_slope = std::clamp(m_slope, -1.0f, 1.0f);
        const int newY = m_lastY + std::lround(m_slope * std::pow(std::abs(m_slope), 0.02f) * STEP);
        Line seg(m_lastX, m_lastY, m_lastX + STEP, newY);
        m_lines.append(seg);
        rasterizeSegmentToHeightMapWorld(seg.getX1(), m_lastY, seg.getX2(), newY);
        m_lastY = newY;
        m_lastX += STEP;
        if (m_lines.size() > (width() / STEP) * 3) { m_lines.removeFirst(); pruneHeightMap(); }
        m_difficulty += DIFF_INC;
    }
}

void IntroScreen::drawPixelText(QPainter& p, const QString& s, int gx, int gy, int scale, const QColor& c, bool bold) {
    auto plot = [&](int x,int y,const QColor& col){ plotGridPixel(p, gx + x, gy + y, col); };
    for (int i = 0; i < s.size(); ++i) {
        const QChar ch = s.at(i).toUpper();
        const auto rows = k5x7.value(k5x7.contains(ch) ? ch : QChar(' '));
        for (int ry=0; ry<7; ++ry) {
            uint8_t row = rows[ry];
            for (int rx=0; rx<5; ++rx) {
                if (row & (1<<(4-rx))) {
                    for (int sy=0; sy<scale; ++sy)
                        for (int sx=0; sx<scale; ++sx) {
                            if (bold) {
                                plot((i*6*scale)+rx*scale+sx-1, ry*scale+sy, QColor(20,20,22));
                                plot((i*6*scale)+rx*scale+sx+1, ry*scale+sy, QColor(20,20,22));
                                plot((i*6*scale)+rx*scale+sx, ry*scale+sy-1, QColor(20,20,22));
                                plot((i*6*scale)+rx*scale+sx, ry*scale+sy+1, QColor(20,20,22));
                            }
                            plot((i*6*scale)+rx*scale+sx, ry*scale+sy, c);
                        }
                }
            }
        }
    }
}

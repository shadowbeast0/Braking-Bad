// intro.cpp
#include "intro.h"
#include <QPainter>
#include <QMouseEvent>
#include <QImage>
#include <array>
#include <cmath>
#include <algorithm>

constexpr int TITLE_STAGE_GAP_PX = 30;

IntroScreen::IntroScreen(QWidget* parent) : QWidget(parent) {
    setAttribute(Qt::WA_OpaquePaintEvent);

    level_index = 0;

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

            if (m_lines.size() > (width() / STEP) * 3) {
                m_lines.removeFirst();
                pruneHeightMap();
            }

            m_difficulty += DIFF_INC;
            maybeSpawnCloud();
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

void IntroScreen::setGrandCoins(int v){
    m_grandCoins = v;
    update();
}

int IntroScreen::titleScale() const {
    return std::clamp(gridH() / (7 * 12), 2, 4);
}

int IntroScreen::titleYCells() const {
    return std::max(2, gridH() / 4);
}

int IntroScreen::startTopCells(int) const {
    int btnH = std::max(10, gridH()/18);
    int gap  = std::max(10,  gridH()/40);
    int bottom = std::max(30, gridH()/24);
    return gridH() - (2*btnH + gap + bottom);
}

int IntroScreen::exitTopCells(int btnHCells) const {
    int bottom = std::max(30, gridH()/24);
    return gridH() - (btnHCells + bottom);
}

void IntroScreen::maybeSpawnCloud() {
    if (m_lastX - m_lastCloudSpawnX < CLOUD_SPACING_PX) return;

    int gx = m_lastX / PIXEL_SIZE;
    auto it = m_heightAtGX.constFind(gx);
    if (it == m_heightAtGX.constEnd()) return;

    int gyGround = it.value();

    auto mix = [](quint32 v, int shift, int span, int base){
        return base + int(((v >> shift) % quint32(span)));
    };
    quint32 h = 120003212u ^ quint32(m_lastX * 2654435761u);

    int wCells = mix(h, 0,  CLOUD_MAX_W_CELLS - CLOUD_MIN_W_CELLS + 1, CLOUD_MIN_W_CELLS);
    int hCells = mix(h, 8,  CLOUD_MAX_H_CELLS - CLOUD_MIN_H_CELLS + 1, CLOUD_MIN_H_CELLS);
    int skyLift = CLOUD_SKY_OFFSET_CELLS + mix(h, 16, 11, 0);

    int cloudTopCells = gyGround - skyLift;
    if (cloudTopCells < 0) cloudTopCells = 0;

    Cloud cl;
    cl.wx = m_lastX;
    cl.wyCells = cloudTopCells;
    cl.wCells = wCells;
    cl.hCells = hCells;
    cl.seed = h;
    m_clouds.append(cl);

    m_lastCloudSpawnX = m_lastX;

    int leftLimit = (m_lines.isEmpty() ? 0 : m_lines.first().getX1()) - width()*2;
    for (int i = 0; i < m_clouds.size(); ) {
        if (m_clouds[i].wx < leftLimit) m_clouds.removeAt(i);
        else ++i;
    }
}

void IntroScreen::drawClouds(QPainter& p) {
    int camGX = m_camX / PIXEL_SIZE;
    int camGY = m_camY / PIXEL_SIZE;

    auto hash2D = [](int x, int y)->quint32{
        quint32 h = 120003212u;
        h ^= quint32(x); h *= 16777619u;
        h ^= quint32(y); h *= 16777619u;
        return h;
    };

    for (const Cloud& cl : m_clouds) {
        int baseGX = (cl.wx / PIXEL_SIZE) - camGX;
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
                    plotGridPixel(p, baseGX + xx, baseGY + yy, pix);
                }
            }
        }
    }
}

void IntroScreen::resizeEvent(QResizeEvent*) {
    m_camXFarthest = m_camX;
}

int IntroScreen::textWidthCells(const QString& s, int scale) const {
    if (s.isEmpty()) return 0;
    return (int(s.size()) - 1) * CHAR_ADV * scale + 5 * scale;
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

void IntroScreen::drawCircleFilledMidpointGrid(QPainter& p, int gcx, int gcy, int gr, const QColor& c) {
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

void IntroScreen::paintEvent(QPaintEvent*) {
    QImage bg(size(), QImage::Format_ARGB32_Premultiplied);
    bg.fill(Constants::SKY_COLOR[level_index]);
    {
        QPainter pb(&bg);
        drawBackground(pb);
    }

    int sw = std::max(1, int(width() * m_blurScale));
    int sh = std::max(1, int(height() * m_blurScale));
    QImage small = bg.scaled(sw, sh, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);

    QPainter p(this);
    p.drawImage(rect(), small, small.rect());

    const int r = 3;
    int iconGX = 2 + r;
    int iconGY = 2 + r;

    drawCircleFilledMidpointGrid(p, iconGX, iconGY, r, QColor(195,140,40));
    drawCircleFilledMidpointGrid(p, iconGX, iconGY, std::max(1, r-1), QColor(250,204,77));
    plotGridPixel(p, iconGX-1, iconGY-r+1, QColor(255,255,220));

    double scale = 1;
    QString total = QString("%1").arg(m_grandCoins);
    int labelGX = iconGX + r*2 + 2;
    int labelGY = iconGY - (7*scale)/3;
    drawPixelText(p, total, labelGX, labelGY, (double)scale, QColor(20,24,28), false);

    const QString title = "D.U.I.";
    int ts = titleScale();
    int titleWCells = textWidthCells(title, ts);
    int tgx = (gridW() - titleWCells) / 2;
    int tgy = titleYCells();
    drawPixelText(p, title, tgx, tgy, ts, Constants::INTRO_TEXT_COLOR, true);

    // --- Level selector ---
    QRect rLevelPrev = buttonRectLevelPrev();
    QRect rLevelNext = buttonRectLevelNext();

    p.setPen(Qt::NoPen);
    p.setBrush(QColor(0,0,0,160));
    p.drawRect(rLevelPrev.translated(3*PIXEL_SIZE,3*PIXEL_SIZE));
    p.drawRect(rLevelNext.translated(3*PIXEL_SIZE,3*PIXEL_SIZE));

    p.setBrush(QColor(150, 150, 160));
    p.drawRect(rLevelPrev);
    p.drawRect(rLevelNext);

    QString sPrev = "<";
    QString sNext = ">";
    int prevScale = fitTextScaleToRect(rLevelPrev.width()/PIXEL_SIZE, rLevelPrev.height()/PIXEL_SIZE, sPrev);
    int nextScale = fitTextScaleToRect(rLevelNext.width()/PIXEL_SIZE, rLevelNext.height()/PIXEL_SIZE, sNext);

    drawPixelText(p, sPrev,
                  rLevelPrev.left()/PIXEL_SIZE + (rLevelPrev.width()/PIXEL_SIZE - textWidthCells(sPrev, prevScale))/2,
                  rLevelPrev.top()/PIXEL_SIZE  + (rLevelPrev.height()/PIXEL_SIZE - 7*prevScale)/2,
                  prevScale, QColor(25,20,24), false);

    drawPixelText(p, sNext,
                  rLevelNext.left()/PIXEL_SIZE + (rLevelNext.width()/PIXEL_SIZE - textWidthCells(sNext, nextScale))/2,
                  rLevelNext.top()/PIXEL_SIZE  + (rLevelNext.height()/PIXEL_SIZE - 7*nextScale)/2,
                  nextScale, QColor(25,20,24), false);

    // Stage name (uses same geometry the helper replicates)
    QString levelName = m_levelNames[level_index];
    int levelScale = 2;
    int levelWCells = textWidthCells(levelName, levelScale);
    int levelGX = (gridW() - levelWCells) / 2;
    const int stageGapCells = (TITLE_STAGE_GAP_PX + PIXEL_SIZE - 1) / PIXEL_SIZE;

    int levelGY = rLevelPrev.top()/PIXEL_SIZE
                  + (rLevelPrev.height()/PIXEL_SIZE - 7*levelScale)/2
                  + stageGapCells;

    drawPixelText(p, levelName, levelGX, levelGY, levelScale, Constants::INTRO_TEXT_COLOR, true);
    // --- End selector ---

    // Buttons (now anchored 5px below stage label)
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

    QString sStart = "PLAY";
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
    drawPixelText(p, sExit,  eGX, eGY,  bsExit,  QColor(25,20,24), false);
}

void IntroScreen::mousePressEvent(QMouseEvent* e) {
    if (buttonRectLevelPrev().contains(e->pos())) {
        level_index--;
        if (level_index < 0) level_index = m_levelNames.size() - 1;
        update();
        return;
    }

    if (buttonRectLevelNext().contains(e->pos())) {
        level_index++;
        if (level_index >= m_levelNames.size()) level_index = 0;
        update();
        return;
    }
    if (buttonRectStart().contains(e->pos())) { emit startRequested(level_index); return; }
    if (buttonRectExit().contains(e->pos()))  { emit exitRequested();             return; }
}

QRect IntroScreen::buttonRectLevelPrev() const {
    int hCells = std::max(10, gridH()/18);
    int wCells = hCells;
    const int stageGapCells = (TITLE_STAGE_GAP_PX + PIXEL_SIZE - 1) / PIXEL_SIZE;

    int yCells = startTopCells(0) - hCells - std::max(10, gridH()/40)
                 + stageGapCells;               // <— was std::max(3, gridH()/40)


    const int fixedOffset = 80;
    int gx = (gridW() / 2) - fixedOffset - wCells;

    return QRect(gx*PIXEL_SIZE, yCells*PIXEL_SIZE, wCells*PIXEL_SIZE, hCells*PIXEL_SIZE);
}

QRect IntroScreen::buttonRectLevelNext() const {
    int hCells = std::max(10, gridH()/18);
    int wCells = hCells;
    const int stageGapCells = (TITLE_STAGE_GAP_PX + PIXEL_SIZE - 1) / PIXEL_SIZE;

    int yCells = startTopCells(0) - hCells - std::max(10, gridH()/40)
                 + stageGapCells;               // <— was std::max(3, gridH()/40)


    const int fixedOffset = 80;
    int gx = (gridW() / 2) + fixedOffset;

    return QRect(gx*PIXEL_SIZE, yCells*PIXEL_SIZE, wCells*PIXEL_SIZE, hCells*PIXEL_SIZE);
}

// --- New: bottom pixel of stage name (for 5px anchoring)
int IntroScreen::stageLabelBottomPx() const {
    QRect rLevelPrev = buttonRectLevelPrev();
    const int levelScale = 2; // same scale used when drawing stage name
    int levelTopCells = rLevelPrev.top()/PIXEL_SIZE
                        + (rLevelPrev.height()/PIXEL_SIZE - 7*levelScale)/2
                        + std::max(3, gridH()/40);
    int bottomCells = levelTopCells + 7*levelScale;
    return bottomCells * PIXEL_SIZE;
}

QRect IntroScreen::buttonRectStart() const {
    int wCells = std::min(std::max(gridW()/6, 30), 50);
    int hCells = std::max(10, gridH()/18);
    int gx     = (gridW() - wCells) / 2;

    // Play sits exactly 5 px under the stage label
    int topPx  = stageLabelBottomPx() + 100;
    int maxTop = gridH()*PIXEL_SIZE - hCells*PIXEL_SIZE - 1;
    if (topPx > maxTop) topPx = maxTop;

    return QRect(gx*PIXEL_SIZE, topPx, wCells*PIXEL_SIZE, hCells*PIXEL_SIZE);
}

QRect IntroScreen::buttonRectExit() const {
    int wCells = std::min(std::max(gridW()/6, 30), 50);
    int hCells = std::max(10, gridH()/18);
    int gx     = (gridW() - wCells) / 2;

    // Exit stacked under Play with normal gap
    int gapCells = std::max(10, gridH()/40);
    int topPx    = stageLabelBottomPx() + 50 + hCells*PIXEL_SIZE + gapCells*PIXEL_SIZE;
    int maxTop   = gridH()*PIXEL_SIZE - hCells*PIXEL_SIZE - 1;
    if (topPx > maxTop) topPx = maxTop;

    return QRect(gx*PIXEL_SIZE, topPx, wCells*PIXEL_SIZE, hCells*PIXEL_SIZE);
}

void IntroScreen::drawBackground(QPainter& p) {
    p.fillRect(rect(), Constants::SKY_COLOR[level_index]);
    drawClouds(p);
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
        return m_grassPalette[level_index][idxG];
    } else {
        const int idxD = int(h % m_dirtPalette.size());
        return m_dirtPalette[level_index][idxD];
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
    double dy = double(y2 - y1);

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

        if (m_lines.size() > (width() / STEP) * 3) {
            m_lines.removeFirst();
            pruneHeightMap();
        }

        m_difficulty += DIFF_INC;
    }
}

void IntroScreen::drawPixelText(QPainter& p, const QString& s, int gx, int gy, int scale, const QColor& c, bool bold)
{
    auto plot = [&](int x,int y,const QColor& col) {
        plotGridPixel(p, gx + x, gy + y, col);
    };

    for (int i = 0; i < s.size(); ++i) {
        const QChar ch = s.at(i).toUpper();
        const auto rows = font_map.value(font_map.contains(ch) ? ch : QChar(' '));
        int baseOff = i * CHAR_ADV * scale;
        for (int ry = 0; ry < 7; ++ry) {
            uint8_t row = rows[ry];
            for (int rx = 0; rx < 5; ++rx) {
                if (row & (1 << (4 - rx))) {
                    for (int sy = 0; sy < scale; ++sy) {
                        for (int sx = 0; sx < scale; ++sx) {
                            if (bold) {
                                plot(baseOff + rx*scale + sx - 1, ry*scale + sy, c);
                                plot(baseOff + rx*scale + sx + 1, ry*scale + sy, c);
                                plot(baseOff + rx*scale + sx, ry*scale + sy - 1, c);
                                plot(baseOff + rx*scale + sx, ry*scale + sy + 1, c);
                            }
                            plot(baseOff + rx*scale + sx, ry*scale + sy, c);
                        }
                    }
                }
            }
        }
    }
}

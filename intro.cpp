#include "intro.h"
#include <QPainter>
#include <QMouseEvent>
#include <QImage>
#include <array>
#include <cmath>
#include <algorithm>
#include "constants.h"

constexpr int TITLE_STAGE_GAP_PX = 30;

IntroScreen::IntroScreen(QWidget* parent, int levelIndex) : QWidget(parent) {
    setAttribute(Qt::WA_OpaquePaintEvent);

    level_index = levelIndex;

    loadGrandCoins();
    loadUnlocks();

    connect(&m_timer, &QTimer::timeout, this, [this]{
        m_scrollX += 2.0;
        m_camX = int(m_scrollX);

        while ((m_camX + width()) > m_camXFarthest) {
            m_camXFarthest += Constants::STEP;

            m_slope += (0.5f - float(m_lastY) / std::max(1,height())) * m_difficulty;
            m_slope = std::clamp(m_slope, -1.0f, 1.0f);

            const int newY = m_lastY + std::lround(m_slope * std::pow(std::abs(m_slope), 0.02f) * Constants::STEP);

            Line seg(m_lastX, m_lastY, m_lastX + Constants::STEP, newY);
            m_lines.append(seg);

            rasterizeSegmentToHeightMapWorld(seg.getX1(), m_lastY, seg.getX2(), newY);

            m_lastY = newY;
            m_lastX += Constants::STEP;

            if (m_lines.size() > (width() / Constants::STEP) * 3) {
                m_lines.removeFirst();
                pruneHeightMap();
            }

            // Using the value from the current level's data
            m_difficulty += Constants::LEVELS[level_index].difficultyIncrement;
            maybeSpawnCloud();
        }

        update();
    });

    m_lastY = height() / 2;
    for (int i = Constants::STEP; i <= width() + Constants::STEP; i += Constants::STEP) {
        m_slope += (0.5f - float(m_lastY) / std::max(1,height())) * m_difficulty;
        m_slope = std::clamp(m_slope, -1.0f, 1.0f);

        const int newY = m_lastY + std::lround(m_slope * std::pow(std::abs(m_slope), 0.02f) * Constants::STEP);

        Line seg(i - Constants::STEP, m_lastY, i, newY);
        m_lines.append(seg);

        rasterizeSegmentToHeightMapWorld(seg.getX1(), m_lastY, seg.getX2(), newY);

        m_lastY = newY;
        m_difficulty += Constants::LEVELS[level_index].difficultyIncrement;
    }
    m_lastX = Constants::STEP * m_lines.size();

    m_timer.start(16);
}

void IntroScreen::setGrandCoins(int v){
    m_grandTotalCoins = v;
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
    if (m_lastX - m_lastCloudSpawnX < Constants::CLOUD_SPACING_PX) return;

    std::uniform_real_distribution<double> dist(0.0, 1.0);
    if (dist(m_rng) > Constants::LEVELS[level_index].cloudProbability) return;

    int gx = m_lastX / Constants::PIXEL_SIZE;
    auto it = m_heightAtGX.constFind(gx);
    if (it == m_heightAtGX.constEnd()) return;

    int gyGround = it.value();

    auto mix = [](quint32 v, int shift, int span, int base){
        return base + int(((v >> shift) % quint32(span)));
    };
    quint32 h = 120003212u ^ quint32(m_lastX * 2654435761u);

    int wCells = mix(h, 0,  Constants::CLOUD_MAX_W_CELLS - Constants::CLOUD_MIN_W_CELLS + 1, Constants::CLOUD_MIN_W_CELLS);
    int hCells = mix(h, 8,  Constants::CLOUD_MAX_H_CELLS - Constants::CLOUD_MIN_H_CELLS + 1, Constants::CLOUD_MIN_H_CELLS);
    int skyLift = Constants::CLOUD_SKY_OFFSET_CELLS + mix(h, 16, 11, 0);

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
    if (Constants::LEVELS[level_index].cloudProbability <= 0.001) return;

    int camGX = m_camX / Constants::PIXEL_SIZE;
    int camGY = m_camY / Constants::PIXEL_SIZE;

    auto hash2D = [](int x, int y)->quint32{
        quint32 h = 120003212u;
        h ^= quint32(x); h *= 16777619u;
        h ^= quint32(y); h *= 16777619u;
        return h;
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
                    QColor cMain = Constants::LEVELS[level_index].cloudColor;
                    QColor cSoft(cMain.red() * 0.9, cMain.green() * 0.9,
                                 cMain.blue() * 0.9);
                    QColor pix = ((h >> 3) & 1) ? cMain : cSoft;
                    plotGridPixel(p, baseGX + xx, baseGY + yy, pix);
                }
            }
        }
    }
}

void IntroScreen::drawStars(QPainter& p) {
    if (Constants::LEVELS[level_index].starProbability <= 0.001) return;

    const int BLOCK = 20;
    const int camGX = m_camX / Constants::PIXEL_SIZE;
    const int camGY = m_camY / Constants::PIXEL_SIZE;

    const int startBX = (camGX) / BLOCK - 1;
    const int endBX   = (camGX + gridW()) / BLOCK + 1;
    const int startBY = (-camGY) / BLOCK - 1;
    const int endBY   = (-camGY + gridH()) / BLOCK + 1;

    for (int bx = startBX; bx <= endBX; ++bx) {
        for (int by = startBY; by <= endBY; ++by) {
            quint32 h = 120003212u;
            h ^= quint32(bx); h *= 16777619u;
            h ^= quint32(by); h *= 16777619u;
            h = (h ^ bx) / (h ^ by) + (bx * by) - (3 * bx*bx + 4 * by*by);

            std::mt19937 rng(h);
            std::uniform_real_distribution<float> fdist(0.0f, 1.0f);

            if (fdist(rng) < Constants::LEVELS[level_index].starProbability * 0.4) {
                std::uniform_int_distribution<int> idist(0, BLOCK - 1);
                int wgx = bx * BLOCK + idist(rng);
                int wgy = by * BLOCK + idist(rng);

                int groundGy = 0;
                auto it = m_heightAtGX.constFind(wgx);
                if (it != m_heightAtGX.constEnd()) groundGy = it.value();
                else groundGy = 10000;

                if (wgy < groundGy - 8) {
                    int sgx = wgx - camGX;
                    int sgy = wgy + camGY;
                    int alpha = std::uniform_int_distribution<int>(100, 255)(rng);
                    plotGridPixel(p, sgx, sgy, QColor(255, 255, 255, alpha));
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
    bg.fill(Constants::LEVELS[level_index].skyColor);
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
    QString total = QString("%1").arg(m_grandTotalCoins);
    int labelGX = iconGX + r*2 + 2;
    int labelGY = iconGY - (7*scale)/3;
    drawPixelText(p, total, labelGX, labelGY, (double)scale, Constants::LEVELS[level_index].textColor, false);

    const QString title = "Braking Bad";
    int ts = titleScale();
    int titleWCells = textWidthCells(title, ts);
    int tgx = (gridW() - titleWCells) / 2;
    int tgy = titleYCells();
    drawPixelText(p, title, tgx, tgy, ts, Constants::LEVELS[level_index].textColor, true);

    // --- Level selector ---
    QRect rLevelPrev = buttonRectLevelPrev();
    QRect rLevelNext = buttonRectLevelNext();

    p.setPen(Qt::NoPen);
    p.setBrush(QColor(0,0,0,160));
    p.drawRect(rLevelPrev.translated(3*Constants::PIXEL_SIZE,3*Constants::PIXEL_SIZE));
    p.drawRect(rLevelNext.translated(3*Constants::PIXEL_SIZE,3*Constants::PIXEL_SIZE));

    p.setBrush(QColor(150, 150, 160));
    p.drawRect(rLevelPrev);
    p.drawRect(rLevelNext);

    QString sPrev = "<";
    QString sNext = ">";
    int prevScale = fitTextScaleToRect(rLevelPrev.width()/Constants::PIXEL_SIZE, rLevelPrev.height()/Constants::PIXEL_SIZE, sPrev);
    int nextScale = fitTextScaleToRect(rLevelNext.width()/Constants::PIXEL_SIZE, rLevelNext.height()/Constants::PIXEL_SIZE, sNext);

    drawPixelText(p, sPrev,
                  rLevelPrev.left()/Constants::PIXEL_SIZE + (rLevelPrev.width()/Constants::PIXEL_SIZE - textWidthCells(sPrev, prevScale))/2,
                  rLevelPrev.top()/Constants::PIXEL_SIZE  + (rLevelPrev.height()/Constants::PIXEL_SIZE - 7*prevScale)/2,
                  prevScale, QColor(25,20,24), false);

    drawPixelText(p, sNext,
                  rLevelNext.left()/Constants::PIXEL_SIZE + (rLevelNext.width()/Constants::PIXEL_SIZE - textWidthCells(sNext, nextScale))/2,
                  rLevelNext.top()/Constants::PIXEL_SIZE  + (rLevelNext.height()/Constants::PIXEL_SIZE - 7*nextScale)/2,
                  nextScale, QColor(25,20,24), false);

    QString levelName = Constants::LEVELS[level_index].name;
    int levelScale = 2;
    int levelWCells = textWidthCells(levelName, levelScale);
    int levelGX = (gridW() - levelWCells) / 2;
    const int stageGapCells = (TITLE_STAGE_GAP_PX + Constants::PIXEL_SIZE - 1) / Constants::PIXEL_SIZE;

    int levelGY = rLevelPrev.top()/Constants::PIXEL_SIZE
                  + (rLevelPrev.height()/Constants::PIXEL_SIZE - 7*levelScale)/2
                  + stageGapCells;

    drawPixelText(p, levelName, levelGX, levelGY, levelScale, Constants::LEVELS[level_index].textColor, true);

    QRect rStart;
    QRect rExit  = buttonRectExit();

    p.setPen(Qt::NoPen);

    if (levels_unlocked[level_index])
    {
        rStart = buttonRectStart();
        p.setBrush(QColor(0,0,0,160));
        p.drawRect(rStart.translated(3*Constants::PIXEL_SIZE,3*Constants::PIXEL_SIZE));
        p.setBrush(QColor(240,190,60));
        p.drawRect(rStart);

        QString sStart = "PLAY";
        int rStartWc = rStart.width()  / Constants::PIXEL_SIZE;
        int rStartHc = rStart.height() / Constants::PIXEL_SIZE;
        int bsStart = fitTextScaleToRect(rStartWc, rStartHc, sStart);
        int sWCells = textWidthCells(sStart, bsStart);
        int sGX = rStart.left()/Constants::PIXEL_SIZE + (rStartWc - sWCells)/2;
        int sGY = rStart.top()/Constants::PIXEL_SIZE  + (rStartHc - 7*bsStart)/2;
        drawPixelText(p, sStart, sGX, sGY, bsStart,  QColor(20,20,20), false);
    }
    else
    {
        rStart = buttonRectUnlock();

        int cost = Constants::LEVELS[level_index].cost;
        bool canAfford = (m_grandTotalCoins >= (quint64)cost);

        // Draw shadow
        p.setBrush(QColor(0,0,0,160));
        p.drawRect(rStart.translated(3*Constants::PIXEL_SIZE,3*Constants::PIXEL_SIZE));

        // Draw button (gray)
        p.setBrush(QColor(100, 100, 110));
        p.drawRect(rStart);

        // Draw text
        QString sStart = QString("UNLOCK: %1").arg(cost);
        int rStartWc = rStart.width()  / Constants::PIXEL_SIZE;
        int rStartHc = rStart.height() / Constants::PIXEL_SIZE;

        int bsStart = fitTextScaleToRect(rStartWc, rStartHc, sStart);

        int sWCells = textWidthCells(sStart, bsStart);
        int sGX = rStart.left()/Constants::PIXEL_SIZE + (rStartWc - sWCells)/2;
        int sGY = rStart.top()/Constants::PIXEL_SIZE  + (rStartHc - 7*bsStart)/2;
        QColor textColor = canAfford ? QColor(20, 20, 20) : QColor(255, 80, 80);
        drawPixelText(p, sStart, sGX, sGY, bsStart, textColor, false);
    }

    p.setBrush(QColor(0,0,0,160));
    p.drawRect(rExit.translated(3*Constants::PIXEL_SIZE,3*Constants::PIXEL_SIZE));
    p.setBrush(QColor(200,80,90));
    p.drawRect(rExit);

    QString sExit  = "EXIT";

    int rExitWc  = rExit.width()   / Constants::PIXEL_SIZE;
    int rExitHc  = rExit.height()  / Constants::PIXEL_SIZE;

    int bsExit  = fitTextScaleToRect(rExitWc,  rExitHc,  sExit);

    int eWCells = textWidthCells(sExit,  bsExit);

    int eGX = rExit.left()/Constants::PIXEL_SIZE  + (rExitWc  - eWCells)/2;
    int eGY = rExit.top()/Constants::PIXEL_SIZE   + (rExitHc  - 7*bsExit)/2;

    drawPixelText(p, sExit,  eGX, eGY,  bsExit,  QColor(20,20,20), false);
}

void IntroScreen::mousePressEvent(QMouseEvent* e) {
    if (buttonRectLevelPrev().contains(e->pos())) {
        level_index--;
        if (level_index < 0) level_index = Constants::LEVELS.size() - 1;
        update();
        return;
    }

    if (buttonRectLevelNext().contains(e->pos())) {
        level_index++;
        if (level_index >= Constants::LEVELS.size()) level_index = 0;
        update();
        return;
    }

    if (buttonRectStart().contains(e->pos()) && levels_unlocked[level_index]) {
        emit startRequested(level_index);
        return;
    }

    if (buttonRectUnlock().contains(e->pos())){
        int cost = Constants::LEVELS[level_index].cost;
        bool canAfford = (m_grandTotalCoins >= (quint64)cost);
        if(canAfford){
            m_grandTotalCoins -= cost;
            levels_unlocked[level_index] = true;
            saveGrandCoins();
            saveUnlocks();
            update();
        }
        return;
    }
    if (buttonRectExit().contains(e->pos()))  { emit exitRequested();             return; }
}

QRect IntroScreen::buttonRectLevelPrev() const {
    int hCells = std::max(10, gridH()/18);
    int wCells = hCells;
    const int stageGapCells = (TITLE_STAGE_GAP_PX + Constants::PIXEL_SIZE - 1) / Constants::PIXEL_SIZE;

    int yCells = startTopCells(0) - hCells - std::max(10, gridH()/40) + stageGapCells;

    const int fixedOffset = 80;
    int gx = (gridW() / 2) - fixedOffset - wCells;

    return QRect(gx*Constants::PIXEL_SIZE, yCells*Constants::PIXEL_SIZE, wCells*Constants::PIXEL_SIZE, hCells*Constants::PIXEL_SIZE);
}

QRect IntroScreen::buttonRectLevelNext() const {
    int hCells = std::max(10, gridH()/18);
    int wCells = hCells;
    const int stageGapCells = (TITLE_STAGE_GAP_PX + Constants::PIXEL_SIZE - 1) / Constants::PIXEL_SIZE;

    int yCells = startTopCells(0) - hCells - std::max(10, gridH()/40) + stageGapCells;


    const int fixedOffset = 80;
    int gx = (gridW() / 2) + fixedOffset;

    return QRect(gx*Constants::PIXEL_SIZE, yCells*Constants::PIXEL_SIZE, wCells*Constants::PIXEL_SIZE, hCells*Constants::PIXEL_SIZE);
}

int IntroScreen::stageLabelBottomPx() const {
    QRect rLevelPrev = buttonRectLevelPrev();
    const int levelScale = 2;
    int levelTopCells = rLevelPrev.top()/Constants::PIXEL_SIZE
                        + (rLevelPrev.height()/Constants::PIXEL_SIZE - 7*levelScale)/2
                        + std::max(3, gridH()/40);
    int bottomCells = levelTopCells + 7*levelScale;
    return bottomCells * Constants::PIXEL_SIZE;
}

QRect IntroScreen::buttonRectStart() const {
    int wCells = std::min(std::max(gridW()/6, 30), 50);
    int hCells = std::max(10, gridH()/18);
    int gx     = (gridW() - wCells) / 2;

    int topPx  = stageLabelBottomPx() + 100;
    int maxTop = gridH()*Constants::PIXEL_SIZE - hCells*Constants::PIXEL_SIZE - 1;
    if (topPx > maxTop) topPx = maxTop;

    return QRect(gx*Constants::PIXEL_SIZE, topPx, wCells*Constants::PIXEL_SIZE, hCells*Constants::PIXEL_SIZE);
}

QRect IntroScreen::buttonRectExit() const {
    int wCells = std::min(std::max(gridW()/6, 30), 50);
    int hCells = std::max(10, gridH()/18);
    int gx     = (gridW() - wCells) / 2;

    int gapCells = std::max(10, gridH()/40);
    int topPx    = stageLabelBottomPx() + 50 + hCells*Constants::PIXEL_SIZE + gapCells*Constants::PIXEL_SIZE;
    int maxTop   = gridH()*Constants::PIXEL_SIZE - hCells*Constants::PIXEL_SIZE - 1;
    if (topPx > maxTop) topPx = maxTop;

    return QRect(gx*Constants::PIXEL_SIZE, topPx, wCells*Constants::PIXEL_SIZE, hCells*Constants::PIXEL_SIZE);
}

QRect IntroScreen::buttonRectUnlock() const{
    int wCells = std::min(std::max(gridW()/2, 80), 120);
    int hCells = std::max(10, gridH()/18);
    int gx     = (gridW() - wCells) / 2;

    int topPx  = stageLabelBottomPx() + 100;
    int maxTop = gridH()*Constants::PIXEL_SIZE - hCells*Constants::PIXEL_SIZE - 1;
    if (topPx > maxTop) topPx = maxTop;

    return QRect(gx*Constants::PIXEL_SIZE, topPx, wCells*Constants::PIXEL_SIZE, hCells*Constants::PIXEL_SIZE);
}

void IntroScreen::drawBackground(QPainter& p) {
    p.fillRect(rect(), Constants::LEVELS[level_index].skyColor);
    drawStars(p);
    drawClouds(p);
    drawFilledTerrain(p);
}

void IntroScreen::drawFilledTerrain(QPainter& p) {
    const int camGX = m_camX / Constants::PIXEL_SIZE;
    const int camGY = m_camY / Constants::PIXEL_SIZE;

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

            int depth = sGY - startScreenGY;
            if (level_index == 5) {
                QColor c;
                if (depth < 14) {
                    if (depth == 0) c = QColor(80, 80, 85);
                    else if (depth >= 6 && depth <= 7 && (worldGX % 20 < 10)) {
                        c = QColor(240, 190, 40);
                    }
                    else c = QColor(50, 50, 55);

                    plotGridPixel(p, sgx, sGY, c);
                    continue;
                }
            }

            bool topZone = (sGY < groundWorldGY + camGY + 3*Constants::SHADING_BLOCK);
            const QColor shade = grassShadeForBlock(worldGX, worldGY, topZone);
            plotGridPixel(p, sgx, sGY, shade);

            if (level_index != 5) {
                const QColor edge = grassShadeForBlock(worldGX, groundWorldGY, true).darker(115);
                plotGridPixel(p, sgx, groundWorldGY + camGY, edge);
            }
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

    const int bx = worldGX / Constants::SHADING_BLOCK;
    const int by = worldGY / Constants::SHADING_BLOCK;
    const quint32 h = hash2D(bx, by);

    // Access palettes through the new structure
    if (greenify) {
        const auto& palette = Constants::LEVELS[level_index].grassPalette;
        const int idxG = int(h % palette.size());
        return palette[idxG];
    } else {
        const auto& palette = Constants::LEVELS[level_index].dirtPalette;
        const int idxD = int(h % palette.size());
        return palette[idxD];
    }
}

void IntroScreen::plotGridPixel(QPainter& p, int gx, int gy, const QColor& c) {
    if (gx < 0 || gy < 0 || gx >= gridW()+1 || gy >= gridH()+1) return;
    p.fillRect(gx * Constants::PIXEL_SIZE, gy * Constants::PIXEL_SIZE, Constants::PIXEL_SIZE, Constants::PIXEL_SIZE, c);
}

void IntroScreen::rasterizeSegmentToHeightMapWorld(int x1,int y1,int x2,int y2){
    if (x2 < x1) { std::swap(x1,x2); std::swap(y1,y2); }

    const int gx1 = x1 / Constants::PIXEL_SIZE;
    const int gx2 = x2 / Constants::PIXEL_SIZE;

    if (x2 == x1) {
        const int gy = int(std::floor(y1 / double(Constants::PIXEL_SIZE) + 0.5));
        m_heightAtGX.insert(gx1, gy);
        return;
    }

    const double dx = double(x2 - x1);
    double dy = double(y2 - y1);

    for (int gx = gx1; gx <= gx2; ++gx) {
        const double wx = gx * double(Constants::PIXEL_SIZE);
        double t = (wx - x1) / dx;
        t = std::clamp(t, 0.0, 1.0);
        const double wy = y1 + t * dy;
        const int gy = int(std::floor(wy / double(Constants::PIXEL_SIZE) + 0.5));
        m_heightAtGX.insert(gx, gy);
    }
}

void IntroScreen::pruneHeightMap() {
    if (m_lines.isEmpty()) return;
    const int keepFromGX = (m_lines.first().getX1() / Constants::PIXEL_SIZE) - 4;

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

        const int newY = m_lastY + std::lround(m_slope * std::pow(std::abs(m_slope), 0.02f) * Constants::STEP);

        Line seg(m_lastX, m_lastY, m_lastX + Constants::STEP, newY);
        m_lines.append(seg);

        rasterizeSegmentToHeightMapWorld(seg.getX1(), m_lastY, seg.getX2(), newY);

        m_lastY = newY;
        m_lastX += Constants::STEP;

        if (m_lines.size() > (width() / Constants::STEP) * 3) {
            m_lines.removeFirst();
            pruneHeightMap();
        }

        m_difficulty += Constants::LEVELS[level_index].difficultyIncrement;
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


void IntroScreen::saveGrandCoins() const {
    QSettings s("JU","F1PixelGrid");
    s.setValue("grandCoins", m_grandTotalCoins);
    s.sync();
}

void IntroScreen::loadGrandCoins() {
    QSettings s("JU","F1PixelGrid");
    m_grandTotalCoins = s.value("grandCoins", 0).toInt();
}

void IntroScreen::saveUnlocks() const {
    QSettings s("JU","F1PixelGrid");

    QVariantList unlockList;

    for(bool unlocked : levels_unlocked) {
        unlockList.append(unlocked);
    }

    s.setValue("unlocks", unlockList);
    s.sync();
}

void IntroScreen::loadUnlocks() {
    QSettings s("JU","F1PixelGrid");
    QVariant unlockData = s.value("unlocks");

    if (unlockData.isValid()) {

        QVariantList savedList = unlockData.toList();

        levels_unlocked.clear();

        for(const QVariant& item : savedList) {
            levels_unlocked.append(item.toBool());
        }

        while(levels_unlocked.size() != Constants::LEVELS.size()) {
            levels_unlocked.append(false);
        }

        saveUnlocks();
    }
    else
    {
        levels_unlocked = {true, false, false, false, false, false};
    }
}

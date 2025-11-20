// intro.h
#ifndef INTRO_H
#define INTRO_H

#include <QWidget>
#include <QTimer>
#include <QHash>
#include <QColor>
#include <QVector>
#include <QList>
#include "line.h"
#include "constants.h"
#include <QSettings>
#include <random>

class QPainter;
class QMouseEvent;
class QResizeEvent;

struct Cloud {
    int wx;
    int wyCells;
    int wCells;
    int hCells;
    quint32 seed;
};

class IntroScreen : public QWidget {
    Q_OBJECT
public:
    explicit IntroScreen(QWidget* parent = nullptr, int levelIndex = 0);
    void setGrandCoins(int v);

signals:
    void startRequested(int levelIndex);
    void exitRequested();

protected:
    void paintEvent(QPaintEvent*) override;
    void mousePressEvent(QMouseEvent*) override;
    void resizeEvent(QResizeEvent*) override;

private:
    void drawStars(QPainter& p);
    void drawClouds(QPainter& p);
    void maybeSpawnCloud();
    void drawBackground(QPainter& p);
    void drawFilledTerrain(QPainter& p);
    void plotGridPixel(QPainter& p, int gx, int gy, const QColor& c);
    void rasterizeSegmentToHeightMapWorld(int x1, int y1, int x2, int y2);
    void pruneHeightMap();
    void ensureAheadTerrain(int worldX);
    QColor grassShadeForBlock(int worldGX, int worldGY, bool greenify) const;

    void drawPixelText(QPainter& p, const QString& s, int gx, int gy, int scale, const QColor& c, bool bold);
    int  textWidthCells(const QString& s, int scale) const;
    int  fitTextScaleToRect(int wCells, int hCells, const QString& s) const;

    // Buttons / rects
    QRect buttonRectStart() const;
    QRect buttonRectExit() const;
    QRect buttonRectUnlock() const;
    QRect buttonRectLevelPrev() const;
    QRect buttonRectLevelNext() const;

    void saveUnlocks() const;
    void loadUnlocks();
    void saveGrandCoins() const;
    void loadGrandCoins();

    int stageLabelBottomPx() const;

    // Grid helpers
    inline int gridW() const { return width()  / PIXEL_SIZE; }
    inline int gridH() const { return height() / PIXEL_SIZE; }
    void drawCircleFilledMidpointGrid(QPainter& p, int gcx, int gcy, int gr, const QColor& c);

    QTimer m_timer;
    double m_scrollX = 0.0;

    QList<Line> m_lines;
    QHash<int,int> m_heightAtGX;
    QVector<Cloud> m_clouds;

    int m_lastCloudSpawnX = 0;
    int m_lastX = 0;
    int m_lastY = 0;
    float m_slope = 0.0f;
    float m_difficulty = 0.005f;

    static constexpr int   PIXEL_SIZE = 6;
    static constexpr int   SHADING_BLOCK = 3;
    static constexpr int   STEP = 20;
    static constexpr float DIFF_INC = 0.0001f;
    static constexpr int CLOUD_SPACING_PX = 700;
    static constexpr int CLOUD_MIN_W_CELLS = 8;
    static constexpr int CLOUD_MAX_W_CELLS = 18;
    static constexpr int CLOUD_MIN_H_CELLS = 3;
    static constexpr int CLOUD_MAX_H_CELLS = 7;
    static constexpr int CLOUD_SKY_OFFSET_CELLS = 40;

    std::mt19937 m_rng;

    static constexpr int CHAR_ADV = 7;

    int m_camX = 0;
    int m_camY = 200;
    int m_camXFarthest = 0;

    qreal m_blurScale = 0.6;

    quint64 m_grandTotalCoins = 0;

    // Title helpers (existing)
    int titleScale() const;
    int titleYCells() const;
    int startTopCells(int titleScale) const;
    int exitTopCells(int btnHCells) const;

    int level_index;

    QVector<bool> levels_unlocked = {true, false, false, false, false};
};

#endif // INTRO_H

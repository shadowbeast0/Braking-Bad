// intro.h
#ifndef INTRO_H
#define INTRO_H

#include <QWidget>
#include <QTimer>
#include <QHash>
#include <QColor>
#include <QVector>
#include "line.h"

static const QHash<QChar, std::array<uint8_t,7>> k5x7 = {
    {'A',{0x1E,0x11,0x11,0x1F,0x11,0x11,0x11}},
    {'C',{0x0E,0x11,0x10,0x10,0x10,0x11,0x0E}},
    {'E',{0x1F,0x10,0x10,0x1E,0x10,0x10,0x1F}},
    {'F',{0x1F,0x10,0x10,0x1E,0x10,0x10,0x10}},
    {'G',{0x0E,0x11,0x10,0x10,0x13,0x11,0x0E}},
    {'H',{0x11,0x11,0x11,0x1F,0x11,0x11,0x11}},
    {'I',{0x1F,0x04,0x04,0x04,0x04,0x04,0x1F}},
    {'L',{0x10,0x10,0x10,0x10,0x10,0x10,0x1F}},
    {'M',{0x11,0x1B,0x15,0x15,0x11,0x11,0x11}},
    {'N',{0x11,0x19,0x15,0x13,0x11,0x11,0x11}},
    {'P',{0x1E,0x11,0x11,0x1E,0x10,0x10,0x10}},
    {'R',{0x1E,0x11,0x11,0x1E,0x14,0x12,0x11}},
    {'S',{0x0F,0x10,0x10,0x0E,0x01,0x01,0x1E}},
    {'T',{0x1F,0x04,0x04,0x04,0x04,0x04,0x04}},
    {'X',{0x11,0x0A,0x04,0x04,0x0A,0x11,0x11}},
    {'Y',{0x11,0x11,0x0A,0x04,0x04,0x04,0x04}},
    {'1',{0x04,0x0C,0x04,0x04,0x04,0x04,0x0E}},
    {':',{0x04,0x04,0x00,0x00,0x04,0x04,0x00}},
    {' ',{0x00,0x00,0x00,0x00,0x00,0x00,0x00}}
};

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
    explicit IntroScreen(QWidget* parent = nullptr);

signals:
    void startRequested();
    void exitRequested();

protected:
    void paintEvent(QPaintEvent*) override;
    void mousePressEvent(QMouseEvent*) override;
    void resizeEvent(QResizeEvent*) override;

private:

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
    QRect buttonRectStart() const;
    QRect buttonRectExit() const;
    inline int gridW() const { return width()  / PIXEL_SIZE; }
    inline int gridH() const { return height() / PIXEL_SIZE; }

    QTimer m_timer;
    double m_scrollX = 0.0;

    QList<Line> m_lines;
    QHash<int,int> m_heightAtGX;
    QVector<QColor> m_grassPalette;
    QVector<QColor> m_dirtPalette;
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

    static constexpr int CHAR_ADV = 7;

    int m_camX = 0;
    int m_camY = 200;
    int m_camXFarthest = 0;

    qreal m_blurScale = 0.6;

    int titleScale() const;
    int titleYCells() const;
    int startTopCells(int titleScale) const;
    int exitTopCells(int btnHCells) const;
};

#endif

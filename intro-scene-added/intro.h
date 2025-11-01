#ifndef INTRO_H
#define INTRO_H

#include <QWidget>
#include <QTimer>
#include <QHash>
#include <QColor>
#include <QVector>
#include "line.h"

class QPainter;
class QMouseEvent;
class QResizeEvent;

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

    int   m_lastX = 0;
    int   m_lastY = 0;
    float m_slope = 0.0f;
    float m_difficulty = 0.005f;

    static constexpr int   PIXEL_SIZE = 6;
    static constexpr int   SHADING_BLOCK = 3;
    static constexpr int   STEP = 20;
    static constexpr float DIFF_INC = 0.0001f;

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

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QWidget>
#include <QList>
#include <QVector>
#include <QTimer>
#include <QHash>
#include <QColor>
#include <QElapsedTimer>
#include <random>
#include "line.h"
#include "wheel.h"

class QKeyEvent;
class QPainter;

class MainWindow : public QWidget {
    Q_OBJECT
public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    void paintEvent(QPaintEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void keyReleaseEvent(QKeyEvent *event) override;

private slots:
    void gameLoop();

private:
    void generateInitialTerrain();

    // ===== Pixel grid helpers =====
    void drawGridOverlay(QPainter& p);
    inline int gridW() const { return width()  / PIXEL_SIZE; }
    inline int gridH() const { return height() / PIXEL_SIZE; }
    void plotGridPixel(QPainter& p, int gx, int gy, const QColor& c);
    void drawCircleFilledMidpointGrid(QPainter& p, int gcx, int gcy, int gr, const QColor& c);

    // ===== Filled terrain helpers =====
    void drawFilledTerrain(QPainter& p);
    QColor grassShadeForBlock(int worldGX, int worldGY) const;
    static inline quint32 hash2D(int x, int y) {
        // FNV-1a (deterministic)
        quint32 h = 2166136261u;
        h ^= quint32(x); h *= 16777619u;
        h ^= quint32(y); h *= 16777619u;
        return h;
    }

    // ===== Height-map (static terrain in world grid) =====
    void rasterizeSegmentToHeightMapWorld(int x1, int y1, int x2, int y2);
    void pruneHeightMap();

    // ===== Camera smoothing =====
    void updateCamera(double targetX, double targetY, double dtSeconds);
    QElapsedTimer m_clock;          // high-res timebase
    double m_camX = 0.0, m_camY = 0.0;   // smoothed camera (px)
    double m_camVX = 0.0, m_camVY = 0.0; // smoothed camera velocity
    double m_camWN  = 8.0;               // natural frequency (rad/s) → 6–12 is good
    double m_camZeta = 0.9;              // damping (0.7–1.0)

private:
    QTimer *m_timer = nullptr;
    QList<Line> m_lines;
    QList<Wheel*> m_wheels;

    int   m_lastX = 0;
    int   m_lastY = 0;
    float m_slope = 0.0f;
    const int   m_step = 20;
    float       m_difficulty = 0.01f;
    const float m_difficultyIncrement = 0.001f;
    const float m_irregularity = 0.5f;

    // integer camera used by drawing/heightmap
    int m_cameraX = 0;
    int m_cameraY = 200;
    int m_cameraXFarthest = 0;

    bool m_accelerating = false;
    bool m_braking = false;

    std::mt19937 m_rng;
    std::uniform_real_distribution<float> m_dist;

    // Pixelation controls
    static constexpr int PIXEL_SIZE = 8;   // size of one visual “pixel” in screen pixels
    bool m_showGrid = true;

    // World-grid ground height: column gx → row gy
    QHash<int,int> m_heightAtGX;

    // Grass shading (Minecraft-like): uniform patches per block of world grid
    static constexpr int SHADING_BLOCK = 6; // in grid cells
    QVector<QColor> m_grassPalette;         // filled in ctor
};

#endif // MAINWINDOW_H

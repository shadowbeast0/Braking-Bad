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
#include "car.h" // ADDED THIS INCLUDE

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
    void restartGame(); // ADDED THIS METHOD
    void generateInitialTerrain();
    void updateCamera(double targetX, double targetY, double dtSeconds);

    // ===== Pixel grid helpers =====
    void drawGridOverlay(QPainter& p);
    inline int gridW() const { return width()  / PIXEL_SIZE; }
    inline int gridH() const { return height() / PIXEL_SIZE; }
    void plotGridPixel(QPainter& p, int gx, int gy, const QColor& c);
    void drawCircleFilledMidpointGrid(QPainter& p, int gcx, int gcy, int gr, const QColor& c);

    // ===== Filled terrain helpers =====
    void drawFilledTerrain(QPainter& p);
    QColor grassShadeForBlock(int worldGX, int worldGY) const;
    static inline quint32 hash2D(int x, int y);

    // ===== Height-map (static terrain in world grid) =====
    void rasterizeSegmentToHeightMapWorld(int x1, int y1, int x2, int y2);
    void pruneHeightMap();

private:
    QTimer *m_timer = nullptr;
    QList<Line> m_lines;
    QList<Wheel*> m_wheels;
    Car* m_car = nullptr; // ADDED Car object pointer

    // Game state
    bool m_gameOver = false; // ADDED Game state flag

    // Terrain Generation
    int   m_lastX = 0;
    int   m_lastY = 0;
    float m_slope = 0.0f;
    const int   m_step = 20;
    float       m_difficulty = 0.01f;
    const float m_difficultyIncrement = 0.001f;
    const float m_irregularity = 0.5f;

    // Camera
    int m_cameraX = 0;
    int m_cameraY = 200;
    int m_cameraXFarthest = 0;

    // Input & State
    bool m_accelerating = false;
    bool m_braking = false;

    std::mt19937 m_rng;
    std::uniform_real_distribution<float> m_dist;

    // Pixelation controls
    static constexpr int PIXEL_SIZE = 8;
    bool m_showGrid = true;

    // World-grid ground height
    QHash<int,int> m_heightAtGX;

    // Grass shading
    static constexpr int SHADING_BLOCK = 6;
    QVector<QColor> m_grassPalette;

    // Camera smoothing
    QElapsedTimer m_clock;
    double m_camX = 0.0, m_camY = 0.0;
    double m_camVX = 0.0, m_camVY = 0.0;
    double m_camWN  = 8.0;
    double m_camZeta = 0.9;
};

inline quint32 MainWindow::hash2D(int x, int y) {
    quint32 h = 2166136261u;
    h ^= quint32(x); h *= 16777619u;
    h ^= quint32(y); h *= 16777619u;
    return h;
}

#endif // MAINWINDOW_H

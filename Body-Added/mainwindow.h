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

#include "constants.h"
#include "line.h"
#include "wheel.h"
#include "intro.h"
#include "carBody.h"

#include "coin.h"
#include "fuel.h"
#include "nitro.h"

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
    void resizeEvent(QResizeEvent *event) override;

private slots:
    void gameLoop();

private:
    void generateInitialTerrain();
    void drawGridOverlay(QPainter& p);
    inline int gridW() const { return width()  / Constants::PIXEL_SIZE; }
    inline int gridH() const { return height() / Constants::PIXEL_SIZE; }
    void plotGridPixel(QPainter& p, int gx, int gy, const QColor& c);
    void drawCircleFilledMidpointGrid(QPainter& p, int gcx, int gcy, int gr, const QColor& c);
    void fillPolygon(QPainter& p, QVector<QPoint> points, const QColor& c);
    void drawFilledTerrain(QPainter& p);

    void drawHUDFuel(QPainter& p);
    void drawHUDCoins(QPainter& p);

    QColor grassShadeForBlock(int worldGX, int worldGY, bool greenify) const;
    static inline quint32 hash2D(int x, int y) {
        quint32 h = 120003212u;
        h ^= quint32(x); h *= 16777619u;
        h ^= quint32(y); h *= 16777619u;
        return (h ^ x) / (h ^ y) + (x * y) - (3 * x*x + 4 * y*y);
    }
    void rasterizeSegmentToHeightMapWorld(int x1, int y1, int x2, int y2);
    void pruneHeightMap();
    void ensureAheadTerrain(int worldX);
    void updateCamera(double targetX, double targetY, double dtSeconds);
    int groundGyNearestGX(int gx) const;
    double terrainTangentAngleAtX(double wx) const;

    double averageSpeed() const;

    QElapsedTimer m_clock;
    double m_camX  = 0.0;
    double m_camY  = 0.0;
    double m_camVX = 0.0;
    double m_camVY = 0.0;
    double m_camWN   = 20.0;
    double m_camZeta = 0.98;

private:
    QTimer *m_timer = nullptr;

    QList<Line>   m_lines;
    QList<Wheel*> m_wheels;
    QList<CarBody*> m_bodies;

    int   m_lastX = 0;
    int   m_lastY = 0;
    float m_slope = 0.0f;

    const int   m_step = 20;
    float       m_difficulty = 0.005f;
    const float m_difficultyIncrement = 0.0001f;
    const float m_irregularity = 0.02f;

    int m_cameraX = 0;
    int m_cameraY = 200;
    int m_cameraXFarthest = 0;

    bool m_accelerating = false;
    bool m_braking      = false;
    bool m_nitroKey     = false;

    std::mt19937 m_rng;
    std::uniform_real_distribution<float> m_dist;

    bool m_showGrid = false;

    QHash<int,int> m_heightAtGX;

    QVector<QColor> m_grassPalette;
    QVector<QColor> m_dirtPalette;

    int leftmostTerrainX() const;

    double m_elapsedSeconds = 0.0;

    FuelSystem  m_fuelSys;
    CoinSystem  m_coinSys;
    NitroSystem m_nitroSys;

    double m_fuel = Constants::FUEL_MAX;
    int    m_coinCount = 0;

    struct Cloud {
        int wx;
        int wyCells;
        int wCells;
        int hCells;
        quint32 seed;
    };

    QVector<Cloud> m_clouds;
    int m_lastCloudSpawnX = 0;

    void maybeSpawnCloud();
    void drawClouds(QPainter& p);

    IntroScreen* m_intro = nullptr;
};

#endif

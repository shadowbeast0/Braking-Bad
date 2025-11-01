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

    
    void drawGridOverlay(QPainter& p);
    inline int gridW() const { return width()  / PIXEL_SIZE; }
    inline int gridH() const { return height() / PIXEL_SIZE; }
    void plotGridPixel(QPainter& p, int gx, int gy, const QColor& c);
    void drawCircleFilledMidpointGrid(QPainter& p, int gcx, int gcy, int gr, const QColor& c);

    
    void drawFilledTerrain(QPainter& p);
    void drawWorldFuel(QPainter& p);
    void drawWorldCoins(QPainter& p);
    void drawNitroFlame(QPainter& p);

    
    void drawHUDFuel(QPainter& p);
    void drawHUDCoins(QPainter& p);
    void drawHUDNitro(QPainter& p);

    
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
    bool m_nitro        = false;   

    std::mt19937 m_rng;
    std::uniform_real_distribution<float> m_dist;

    static constexpr int PIXEL_SIZE = 6;
    bool m_showGrid = true;

    QHash<int,int> m_heightAtGX;

    static constexpr int SHADING_BLOCK = 3;
    QVector<QColor> m_grassPalette;
    QVector<QColor> m_dirtPalette;

    int leftmostTerrainX() const;

    
    double m_elapsedSeconds = 0.0;

    
    struct FuelCan { int wx; int wy; bool taken = false; };
    QVector<FuelCan> m_worldFuel;
    int m_lastPlacedFuelX = 0;

    double m_fuel = 1.0;
    static constexpr double FUEL_MAX = 1.0;
    static constexpr double FUEL_BASE_BURN_PER_SEC = 0.015;
    static constexpr double FUEL_EXTRA_PER_SPEED   = 0.002;
    static constexpr double FUEL_PICKUP_AMOUNT     = 0.4;

    static constexpr int FUEL_PICKUP_RADIUS = 18;
    static constexpr int FUEL_FLOOR_OFFSET_CELLS = 6;

    static constexpr int HUD_TOP_MARGIN  = 6;
    static constexpr int HUD_LEFT_MARGIN = 4;

    static constexpr double FUEL_SPAWN_EASE = 0.4;

    int currentFuelSpacing();
    void maybePlaceFuelAtEdge();

    double averageSpeed() const;

    
    struct Coin { int cx; int cy; bool taken = false; };
    QVector<Coin> m_worldCoins;
    int m_lastPlacedCoinX = 0;
    int m_coinCount = 0;
    double m_lastCoinSpawnTimeSeconds = 0.0;


    static constexpr int COIN_RADIUS_CELLS        = 3;
    static constexpr int COIN_PICKUP_RADIUS       = 28;
    static constexpr int COIN_FLOOR_OFFSET_CELLS  = 6;
    static constexpr int COIN_GROUP_MIN           = 5;
    static constexpr int COIN_GROUP_MAX           = 16;
    static constexpr int COIN_GROUP_STEP_MIN      = 6;
    static constexpr int COIN_GROUP_STEP_MAX      = 9;
    static constexpr int COIN_SPAWN_MARGIN_CELLS  = 24;
    static constexpr int COIN_STREAM_LEN          = 10;
    static constexpr int COIN_STREAM_STEP_CELLS   = 7;
    static constexpr int COIN_STREAM_SPACING_PX   = 800;
    static constexpr int COIN_STREAM_AMP_CELLS    = 2;


    int  currentCoinSpacing();
    void maybePlaceCoinStreamAtEdge();

    
    static constexpr int    NITRO_MAX_ALT_CELLS      = 128;
    static constexpr double NITRO_THRUST             = 0.125;
    static constexpr double NITRO_DURATION_SECOND    = 3.0;  

    double m_nitroEndTime        = 0.0;
    double m_nitroCooldownUntil  = 0.0;
    double m_nitroDirX           = 0.0;
    double m_nitroDirY           = 1.0;
    int    m_nitroCeilY          = -1000000000;

    struct Cloud {
        int wx;        // world x (px) of left edge
        int wyCells;   // world y (in grid cells, NOT px) of top row of the cloud
        int wCells;    // width in grid cells
        int hCells;    // height in grid cells
        quint32 seed;  // for per-cloud pixel noise
    };

    QVector<Cloud> m_clouds;
    int m_lastCloudSpawnX = 0;


    static constexpr int CLOUD_SPACING_PX        = 1200;
    static constexpr int CLOUD_SKY_OFFSET_CELLS  = 40;
    static constexpr int CLOUD_MIN_W_CELLS       = 10;
    static constexpr int CLOUD_MAX_W_CELLS       = 18;
    static constexpr int CLOUD_MIN_H_CELLS       = 4;
    static constexpr int CLOUD_MAX_H_CELLS       = 7;

    void maybeSpawnCloud();
    void drawClouds(QPainter& p);


};

#endif

#ifndef CONSTANTS_H
#define CONSTANTS_H

#include <QColor>
#include <QString>
#include <QPoint>
#include <QHash>
#include <array>

struct Constants {

    static constexpr int PIXEL_SIZE = 6;
    static constexpr int SHADING_BLOCK = 3;

    // LEVEL MECHANICS
    inline static QVector<double> GRAVITY           = {0.08, 0.08, 0.08, 0.04, 0.06, 0.08};
    inline static QVector<double> AIR_RESISTANCE    = {0.0005, 0.0003, 0.0007, 0.00001, 0.00005, 0.0007};
    inline static QVector<double> RESTITUTION       = {0.8, 0.5, 0.8, 0.7, 0.07, 0.5};
    inline static QVector<double> FRICTION          = {0.003, 0.03, 0.0001, 0.001, 0.03, 0.03};
    inline static QVector<double> TRACTION          = {1, 1.25, 0.5, 0.5, 0.75, 1.5};
    inline static QVector<double> CLOUD_PROBABILITY = {0.5, 0.1, 0.7, 0, 0.05, 0};
    inline static QVector<double> STAR_PROBABILITY = {0, 0, 0, 0.7, 0, 0.7};
    inline static QVector<QColor> SKY_COLOR = {QColor(150,210,255), QColor(255,220,200), QColor(210,210,255), QColor(0,0,0), QColor(200, 150, 150), QColor(30, 30, 40)};
    inline static QVector<QColor> CLOUD_COLOR = {QColor(255,255,255), QColor(255,255,200), QColor(255,255,255), QColor(0,0,0), QColor(255, 220, 200), QColor(50, 50, 25)};
    inline static QVector<double> MAX_SLOPE = {1.0, 1.5, 0.8, 2, 1.5, 0.5};
    inline static QVector<double> INITIAL_DIFFICULTY = {0.005, 0.005, 0.008, 0.01, 0.008, 0.001};
    inline static QVector<double> DIFFICULTY_INCREMENT = {0.0001, 0.0002, 0.0001, 0.0003, 0.0002, 0.0001};
    inline static QVector<double> INITIAL_IRREGULARITY = {0.01, 0.001, 0.01, 0.05, 0.02, 0.05};
    inline static QVector<double> IRREGULARITY_INCREMENT = {0.00001, 0.000001, 0.00003, 0.0001, 0.00005, 0.0001};
    inline static QVector<double> INITIAL_TERRAIN_HEIGHT = {10, 10, 10, 20, 10, 5};
    inline static QVector<double> TERRAIN_HEIGHT_INCREMENT = {0.001, 0.002, 0.001, 0.002, 0.002, 0.0005};
    static constexpr int STEP = 20;

    //CAR MECHANICS
    static constexpr double MAX_VELOCITY    = 30.0;
    static constexpr double ACCELERATION    = 0.8;
    static constexpr double DECELERATION    = 0.8;
    static constexpr double SPRING_CONSTANT = 0.6;
    static constexpr double DAMPING         = 0.06;
    static constexpr double ANGULAR_ACCELERATION = 0.0010;
    static constexpr double ANGULAR_DECELERATION = 0.0010;
    static constexpr double ANGULAR_DAMPING      = 0.05;
    static constexpr double MAX_ANGULAR_VELOCITY = 0.04;

    // CAR
    static constexpr QColor CAR_COLOR = QColor(200, 50, 50);
    static constexpr QColor WHEEL_COLOR_OUTER = QColor(40, 50, 60);
    static constexpr QColor WHEEL_COLOR_INNER = QColor(160, 165, 170);
    static constexpr double WHEEL_REAR_X  = 100;
    static constexpr double WHEEL_REAR_Y  = 300;
    static constexpr double WHEEL_REAR_R  = 20;
    static constexpr double WHEEL_FRONT_X = 220;
    static constexpr double WHEEL_FRONT_Y = 300;
    static constexpr double WHEEL_FRONT_R = 20;
    static constexpr double WHEEL_MID_X   = 160;
    static constexpr double WHEEL_MID_Y   = 300;
    static constexpr double WHEEL_MID_R   = 0;
    static constexpr int TYRE_THICKNESS = 10;

    static constexpr QColor CAR_GLASS_COLOR  = QColor(20,20,20);
    static constexpr QColor CAR_HANDLE_COLOR = QColor(40,40,40);

    inline static const QVector<QPoint> CAR_BODY_POINTS = {
        QPoint(0,0), QPoint(0,31), QPoint(9,37), QPoint(15,19), QPoint(44,19),
        QPoint(50,37), QPoint(138,37), QPoint(144,19), QPoint(172,19), QPoint(179,37),
        QPoint(188,31), QPoint(181,6), QPoint(137,0), QPoint(112,-25), QPoint(62,-25),
        QPoint(37,-6), QPoint(25,-7), QPoint(19,-19), QPoint(4,-21), QPoint(1,-15), QPoint(12, -10)
    };

    inline static const QVector<QPoint> CAR_HITBOX_POINTS = {
        QPoint(15,6), QPoint(173, 1),  QPoint(137,0), QPoint(1,-15), QPoint(12, -10)
    };

    inline static const QVector<QPoint> CAR_KILL_POINTS = {QPoint(112, -25), QPoint(62, -25)};
    inline static const QVector<QPoint> CAR_GLASS_POINTS = {QPoint(60, 6), QPoint(70, -18), QPoint(116, -18), QPoint(140, 6)};
    inline static const QVector<QPoint> CAR_HANDLE_POINTS = {QPoint(96, 10), QPoint(102, 10), QPoint(102, 13), QPoint(96, 13)};

    // HUD
    static constexpr int HUD_TOP_MARGIN  = 6;
    static constexpr int HUD_LEFT_MARGIN = 4;

    // FUEL
    static constexpr double FUEL_MAX               = 1.0;
    static constexpr double FUEL_BASE_BURN_PER_SEC = 0.015;
    static constexpr double FUEL_EXTRA_PER_SPEED   = 0.002;
    static constexpr double FUEL_PICKUP_AMOUNT     = 0.4;
    static constexpr int    FUEL_PICKUP_RADIUS       = 18;
    static constexpr int    FUEL_FLOOR_OFFSET_CELLS  = 6;
    static constexpr double FUEL_SPAWN_EASE          = 0.4;


    // COIN
    static constexpr int COIN_RADIUS_CELLS       = 3;
    static constexpr int COIN_PICKUP_RADIUS      = 28;
    static constexpr int COIN_FLOOR_OFFSET_CELLS = 6;
    static constexpr int COIN_GROUP_MIN         = 5;
    static constexpr int COIN_GROUP_MAX         = 10;
    static constexpr int COIN_GROUP_STEP_MIN    = 9;
    static constexpr int COIN_GROUP_STEP_MAX    = 13;
    static constexpr int COIN_SPAWN_MARGIN_CELLS= 24;
    static constexpr int COIN_STREAM_LEN        = 10;
    static constexpr int COIN_STREAM_STEP_CELLS = 7;
    static constexpr int COIN_STREAM_SPACING_PX = 800;
    static constexpr int COIN_STREAM_AMP_CELLS  = 2;

    // NITRO
    static constexpr int    NITRO_MAX_ALT_CELLS     = 2048;
    static constexpr double NITRO_THRUST            = 0.25;
    static constexpr double NITRO_DURATION_SECOND   = 1.5;

    // CLOUD
    static constexpr int CLOUD_SPACING_PX       = 200;
    static constexpr int CLOUD_SKY_OFFSET_CELLS = 40;
    static constexpr int CLOUD_MIN_W_CELLS      = 10;
    static constexpr int CLOUD_MAX_W_CELLS      = 18;
    static constexpr int CLOUD_MIN_H_CELLS      = 4;
    static constexpr int CLOUD_MAX_H_CELLS      = 7;

    // TOPPLING
    static constexpr double FLIPPED_COS_MIN = -0.90;
    static constexpr double FLIPPED_SIN_MAX =  0.35;

    // GAME OVER
    static constexpr int GAME_OVER_DELAY_MS = 2000;

    // SCORE CALCULATION
    static constexpr double SCORE_DIST_PER_CELL = 1.0;
    static constexpr int SCORE_PER_COIN = 50;
    static constexpr int SCORE_PER_NITRO = 25;


    // === TEXT COLORS
    static constexpr QColor INTRO_COIN_COLOR = QColor(254, 194, 12);
    inline static QVector<QColor> TEXT_COLOR = {QColor(20,20,20), QColor(20,20,20), QColor(20,20,20), QColor(200,200,200), QColor(20,20,20), QColor(200, 200, 200)};
    inline static QVector<QColor> FLIP_COLOR = {QColor(0,0,0), QColor(0,0,0), QColor(0,0,0), QColor(237,181,37), QColor(0,0,0), QColor(237,181,37)};
};


//Level generation
static QVector<QString> m_levelNames = {
    "MEADOW",
    "DESERT",
    "TUNDRA",
    "LUNAR",
    "MARTIAN",
    "NIGHTLIFE"
};

static QVector<int>  m_levelCosts = {0, 300, 600, 1000, 1500, 2500};

// palettes
inline const QVector<QVector<QColor>> m_grassPalette = {
    {QColor(100,140,50), QColor(115,155,60), QColor(130,170,80),
    QColor(85,120,40),  QColor(100,140,50), QColor(115,155,60),
    QColor(130,170,80), QColor(115,155,60), QColor(130,170,80),
     QColor(85,120,40)},

    {QColor(225,190,100), QColor(240,210,120), QColor(250,225,140),
      QColor(205,170, 90), QColor(225,190,100), QColor(240,210,120),
      QColor(250,225,140), QColor(240,210,120), QColor(250,225,140),
      QColor(205,170, 90)},

    {QColor(190,200,215), QColor(220,225,235), QColor(245,245,255),
     QColor(160,170,190), QColor(190,200,215), QColor(220,225,235),
     QColor(245,245,255), QColor(220,225,235), QColor(245,245,255),
     QColor(160,170,190)},

    {QColor(120,120,120), QColor(150,150,150), QColor(180,180,180),
     QColor(90, 90, 90),  QColor(120,120,120), QColor(150,150,150),
     QColor(180,180,180), QColor(150,150,150), QColor(180,180,180),
     QColor(90, 90, 90)},

    {QColor(190,100, 50), QColor(220,120, 70), QColor(240,150, 90),
     QColor(150, 80, 40), QColor(190,100, 50), QColor(220,120, 70),
     QColor(240,150, 90), QColor(220,120, 70), QColor(240,150, 90),
     QColor(150, 80, 40)},

    {QColor(65, 65, 70), QColor(85, 85, 90), QColor(105,105,110),
      QColor(45, 45, 50), QColor(65, 65, 70), QColor(85, 85, 90),
      QColor(105,105,110), QColor(85, 85, 90), QColor(105,105,110),
      QColor(45, 45, 50)}
};

inline const QVector<QVector<QColor>> m_dirtPalette = {
    {QColor(125,105,40), QColor(135,117,50), QColor(145,125,60),
    QColor(150,120,50), QColor(120,100,30), QColor(145,125,60),
    QColor(150,120,50), QColor(125,105,40), QColor(135,117,50),
     QColor(120,100,30)},

    {QColor(213,157, 95), QColor(221,165,103), QColor(243,190,125),
     QColor(198,140, 80), QColor(213,157, 95), QColor(221,165,103),
     QColor(243,190,125), QColor(221,165,103), QColor(243,190,125),
     QColor(198,140, 80)},

    {QColor(100,100,110), QColor(120,120,125), QColor(135,135,140),
     QColor(80, 80, 90),  QColor(100,100,110), QColor(120,120,125),
     QColor(135,135,140), QColor(120,120,125), QColor(135,135,140),
     QColor(80, 80, 90)},

    {QColor(65, 65, 70), QColor(80, 80, 85), QColor(95, 95,100),
     QColor(50, 50, 55), QColor(65, 65, 70), QColor(80, 80, 85),
     QColor(95, 95,100), QColor(80, 80, 85), QColor(95, 95,100),
     QColor(50, 50, 55)},

    {QColor(100, 70, 65), QColor(120, 90, 85), QColor(135,105,100),
     QColor(80, 50, 45),  QColor(100, 70, 65), QColor(120, 90, 85),
     QColor(135,105,100), QColor(120, 90, 85), QColor(135,105,100),
     QColor(80, 50, 45)},

    {QColor(40, 40, 45), QColor(35, 35, 40), QColor(45, 45, 50),
     QColor(30, 30, 35), QColor(40, 40, 45), QColor(35, 35, 40),
     QColor(45, 45, 50), QColor(40, 40, 45), QColor(35, 35, 40),
     QColor(30, 30, 35)}
};

// font mapping
inline const QHash<QChar, std::array<uint8_t,7>> font_map = {
    {'A',{0x0E,0x11,0x11,0x1F,0x11,0x11,0x11}},
    {'B',{0x1E,0x11,0x11,0x1E,0x11,0x11,0x1E}},
    {'C',{0x0E,0x11,0x10,0x10,0x10,0x11,0x0E}},
    {'D',{0x1E,0x11,0x11,0x11,0x11,0x11,0x1E}},
    {'E',{0x1F,0x10,0x10,0x1E,0x10,0x10,0x1F}},
    {'F',{0x1F,0x10,0x10,0x1E,0x10,0x10,0x10}},
    {'G',{0x0E,0x11,0x10,0x10,0x13,0x11,0x0E}},
    {'H',{0x11,0x11,0x11,0x1F,0x11,0x11,0x11}},
    {'I',{0x1F,0x04,0x04,0x04,0x04,0x04,0x1F}},
    {'J',{0x07,0x02,0x02,0x02,0x12,0x12,0x0C}},
    {'K',{0x11,0x12,0x14,0x18,0x14,0x12,0x11}},
    {'L',{0x10,0x10,0x10,0x10,0x10,0x10,0x1F}},
    {'M',{0x11,0x1B,0x15,0x15,0x11,0x11,0x11}},
    {'m',{0x00,0x00,0x1A,0x15,0x15,0x15,0x15}},
    {'N',{0x11,0x19,0x15,0x13,0x11,0x11,0x11}},
    {'O',{0x0E,0x11,0x11,0x11,0x11,0x11,0x0E}},
    {'P',{0x1E,0x11,0x11,0x1E,0x10,0x10,0x10}},
    {'Q',{0x0E,0x11,0x11,0x11,0x15,0x12,0x0D}},
    {'R',{0x1E,0x11,0x11,0x1E,0x14,0x12,0x11}},
    {'S',{0x0F,0x10,0x10,0x0E,0x01,0x01,0x1E}},
    {'T',{0x1F,0x04,0x04,0x04,0x04,0x04,0x04}},
    {'U',{0x11,0x11,0x11,0x11,0x11,0x11,0x0E}},
    {'V',{0x11,0x11,0x11,0x11,0x11,0x0A,0x04}},
    {'W',{0x11,0x11,0x11,0x15,0x15,0x1B,0x11}},
    {'X',{0x11,0x0A,0x04,0x04,0x0A,0x11,0x11}},
    {'Y',{0x11,0x11,0x0A,0x04,0x04,0x04,0x04}},
    {'Z',{0x1F,0x01,0x02,0x04,0x08,0x10,0x1F}},
    {'0',{0x0E,0x11,0x13,0x15,0x19,0x11,0x0E}},
    {'1',{0x04,0x0C,0x04,0x04,0x04,0x04,0x0E}},
    {'2',{0x0E,0x11,0x01,0x02,0x04,0x08,0x1F}},
    {'3',{0x0E,0x11,0x01,0x06,0x01,0x11,0x0E}},
    {'4',{0x02,0x06,0x0A,0x12,0x1F,0x02,0x02}},
    {'5',{0x1F,0x10,0x1E,0x01,0x01,0x11,0x0E}},
    {'6',{0x06,0x08,0x10,0x1E,0x11,0x11,0x0E}},
    {'7',{0x1F,0x01,0x02,0x04,0x08,0x08,0x08}},
    {'8',{0x0E,0x11,0x11,0x0E,0x11,0x11,0x0E}},
    {'9',{0x0E,0x11,0x11,0x0F,0x01,0x02,0x0C}},
    {':',{0x04,0x04,0x00,0x00,0x04,0x04,0x00}},
    {' ',{0x00,0x00,0x00,0x00,0x00,0x00,0x00}},
    {'.',{0x00,0x00,0x00,0x00,0x00,0x04,0x04}},
    {'<',{0x01,0x03,0x07,0x0F,0x07,0x03,0x01}},
    {'>',{0x10,0x18,0x1C,0x1E,0x1C,0x18,0x10}}
};

#endif

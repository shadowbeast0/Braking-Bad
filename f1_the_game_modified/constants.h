#ifndef CONSTANTS_H
#define CONSTANTS_H

#include <QColor>
#include <QHash>
#include <array>

struct Constants {
    // GAME MECHANICS
    static constexpr double GRAVITY         = 0.08;
    static constexpr double AIR_RESISTANCE  = 0.9992;
    static constexpr double MAX_VELOCITY    = 30.0;
    static constexpr double RESTITUTION     = -0.2;
    static constexpr double FRICTION        = 0.003;
    static constexpr double ACCELERATION    = 0.9;
    static constexpr double DECELERATION    = 0.3;
    static constexpr double SPRING_CONSTANT = 0.6;
    static constexpr double DAMPING         = 0.06;
    static constexpr double ROTATION        = 0.3;

    // CAR
    static constexpr QColor CAR_COLOR = QColor(200, 50, 50);
    static constexpr QColor WHEEL_COLOR_OUTER = QColor(40, 50, 60);
    static constexpr QColor WHEEL_COLOR_INNER = QColor(160, 165, 170);
    static constexpr int TYRE_THICKNESS = 10;

    static constexpr int PIXEL_SIZE = 6;

    static constexpr int SHADING_BLOCK = 3;

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
    static constexpr int COIN_GROUP_MAX         = 16;
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
    static constexpr int GAME_OVER_DELAY_MS = 5000;

    // SCORE CALCULATION
    static constexpr double SCORE_DIST_PER_CELL = 1.0;
    static constexpr int SCORE_PER_COIN = 50;
    static constexpr int SCORE_PER_NITRO = 25;
};


// palettes
inline const QVector<QColor> m_grassPalette = {
    QColor(100,140,50), QColor(115,155,60), QColor(130,170,80),
    QColor(85,120,40),  QColor(100,140,50), QColor(115,155,60),
    QColor(130,170,80), QColor(115,155,60), QColor(130,170,80),
    QColor(85,120,40)
};

inline const QVector<QColor> m_dirtPalette = {
    QColor(125,105,40), QColor(135,117,50), QColor(145,125,60),
    QColor(150,120,50), QColor(120,100,30), QColor(145,125,60),
    QColor(150,120,50), QColor(125,105,40), QColor(135,117,50),
    QColor(120,100,30)
};

/*
inline const QVector<QColor> mainWin_m_grassPalette = {
    QColor(100,140,50), QColor(115,155,60), QColor(130,170,80),
    QColor(85,120,40),  QColor(100,140,50), QColor(115,155,60),
    QColor(130,170,80), QColor(115,155,60), QColor(130,170,80),
    QColor(85,120,40),  QColor(85,120,40),  QColor(100,140,50),
    QColor(100,140,50), QColor(115,155,60), QColor(130,170,80),
    QColor(85,120,40),  QColor(100,140,50), QColor(115,155,60),
    QColor(85,120,40),  QColor(100,140,50), QColor(115,155,60),
    QColor(100,140,50), QColor(115,155,60), QColor(130,170,80),
    QColor(85,120,40),  QColor(130,170,80), QColor(130,170,80),
    QColor(85,120,40)
};

inline const QVector<QColor> mainWin_m_dirtPalette = {
    QColor(125,105,40), QColor(135,117,50), QColor(145,125,60),
    QColor(145,125,60), QColor(150,120,50), QColor(120,100,30),
    QColor(125,105,40), QColor(150,120,50), QColor(120,100,30),
    QColor(150,120,50), QColor(125,105,40), QColor(135,117,50),
    QColor(135,117,50), QColor(145,125,60), QColor(150,120,50),
    QColor(120,100,30), QColor(125,105,40), QColor(135,117,50),
    QColor(125,105,40), QColor(135,117,50), QColor(150,120,50),
    QColor(145,125,60), QColor(145,125,60), QColor(145,125,60),
    QColor(120,100,30), QColor(125,105,40), QColor(120,100,30),
    QColor(120,100,30), QColor(150,120,50), QColor(125,105,40),
    QColor(135,117,50), QColor(135,117,50), QColor(150,120,50),
    QColor(145,125,60), QColor(120,100,30), QColor(150,120,50)
};
*/


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
    {'.',{0x00,0x00,0x00,0x00,0x00,0x04,0x04}}
};

#endif

#ifndef CONSTANTS_H
#define CONSTANTS_H

#include <QColor>

struct Constants {
    // ===== Physics (existing) =====
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

    // ===== Car visuals (declared here; defined in constants.cpp, unchanged) =====
    static const QColor CAR_COLOR;
    static const QColor TYRE_COLOR;
    static const QColor WHEEL_COLOR;
    static constexpr int  TYRE_THICKNESS = 10;

    // ===== Game-wide grid/scaling =====
    static constexpr int PIXEL_SIZE = 6;

    // ===== Terrain shading =====
    static constexpr int SHADING_BLOCK = 3;

    // ===== HUD =====
    static constexpr int HUD_TOP_MARGIN  = 6;
    static constexpr int HUD_LEFT_MARGIN = 4;

    // ===== Fuel system =====
    static constexpr double FUEL_MAX               = 1.0;
    static constexpr double FUEL_BASE_BURN_PER_SEC = 0.015;
    static constexpr double FUEL_EXTRA_PER_SPEED   = 0.002;
    static constexpr double FUEL_PICKUP_AMOUNT     = 0.4;

    static constexpr int    FUEL_PICKUP_RADIUS       = 18;
    static constexpr int    FUEL_FLOOR_OFFSET_CELLS  = 6;
    static constexpr double FUEL_SPAWN_EASE          = 0.4;

    // ===== Coin system =====
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

    // ===== Nitro system =====
    static constexpr int    NITRO_MAX_ALT_CELLS     = 2048;
    static constexpr double NITRO_THRUST            = 0.25;
    static constexpr double NITRO_DURATION_SECOND   = 5.0;

    // ===== Clouds =====
    static constexpr int CLOUD_SPACING_PX       = 200;
    static constexpr int CLOUD_SKY_OFFSET_CELLS = 40;
    static constexpr int CLOUD_MIN_W_CELLS      = 10;
    static constexpr int CLOUD_MAX_W_CELLS      = 18;
    static constexpr int CLOUD_MIN_H_CELLS      = 4;
    static constexpr int CLOUD_MAX_H_CELLS      = 7;
};

#endif // CONSTANTS_H

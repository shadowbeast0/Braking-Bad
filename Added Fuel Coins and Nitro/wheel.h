#ifndef WHEEL_H
#define WHEEL_H

#include "line.h"
#include <QList>
#include <optional>
#include <array>

class Wheel {
public:
    double x = 0.0, y = 0.0;
    double m_vx = 0.0, m_vy = 0.0;
    double m_angle = 0.0;
    double m_omega = 0.0;
    bool   m_isRoot = false;

    Wheel(int x, int y, int radius);
    int radius() const;

    void attach(Wheel* other);

    // signature with nitro stays
    void simulate(const QList<Line>& lines,
                  bool accelerating,
                  bool braking,
                  bool nitro);

    // (centerX, centerY, radius) for rendering after camera offset
    std::optional<std::array<int, 3>> get(
        int x1, int y1, int x2, int y2, int cx, int cy
        ) const;

private:
    int m_radius;
    QList<Wheel*> m_others;
    QList<double> m_distances;

    // constants from your tuned "code 1" physics (:contentReference[oaicite:8]{index=8})
    static constexpr double GRAVITY         = 0.1;
    static constexpr double AIR_RESISTANCE  = 0.9992;
    static constexpr double MAX_VELOCITY    = 30.0;
    static constexpr double RESTITUTION     = -0.2;
    static constexpr double FRICTION        = 0.9972;
    static constexpr double ACCELERATION    = 0.3;
    static constexpr double DECELERATION    = 0.3;
    static constexpr double SPRING_CONSTANT = 0.12;
    static constexpr double DAMPING_FACTOR  = 0.03;

    static constexpr double ANGULAR_ACCELERATION = 0.0015;
    static constexpr double ANGULAR_DECELERATION = 0.0015;
    static constexpr double ANGULAR_DAMPING      = 0.98;
    static constexpr double MAX_ANGULAR_VELOCITY = 0.04;
};

#endif

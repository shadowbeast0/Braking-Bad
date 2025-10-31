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
    // System rotation (shared between attached wheels)
    double m_angle = 0.0;      // radians, rotation around center of mass
    double m_omega = 0.0;      // angular velocity
    bool m_isRoot = false;     // true if this wheel controls rotation

    Wheel(int x, int y, int radius);

    void attach(Wheel* other);
    void simulate(const QList<Line>& lines, bool accelerating, bool braking);
    // Returns (centerX, centerY, radius) in screen space after camera offset
    std::optional<std::array<int, 3>> get(int x1, int y1, int x2, int y2, int cx, int cy) const;

private:
    int m_radius;
    QList<Wheel*> m_others;
    QList<double> m_distances;

    // Tunable physics constants (kept the same)
    static constexpr double GRAVITY         = 0.1;     // 0.05;
    static constexpr double AIR_RESISTANCE  = 0.9992;   // 0.9995
    static constexpr double MAX_VELOCITY    = 30.0;    // 25.00
    static constexpr double RESTITUTION     = -0.2;    // -0.1
    static constexpr double FRICTION        = 0.9972;  // 0.9985
    static constexpr double ACCELERATION    = 0.3;    // 0.1
    static constexpr double DECELERATION    = 0.3;    // 0.1;
    static constexpr double SPRING_CONSTANT = 0.12;     // 0.05
    static constexpr double DAMPING_FACTOR  = 0.03;    // 0.02

    // Tunable rotational parameters
    static constexpr double ANGULAR_ACCELERATION = 0.0015;  // 0.0005
    static constexpr double ANGULAR_DECELERATION = 0.0015;  // 0.0005
    static constexpr double ANGULAR_DAMPING      = 0.98;    // 0.98
    static constexpr double MAX_ANGULAR_VELOCITY = 0.04;   // 0.018

};

#endif // WHEEL_H

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
    static constexpr double GRAVITY         = 0.02;
    static constexpr double AIR_RESISTANCE  = 0.9999;
    static constexpr double MAX_VELOCITY    = 10.0;
    static constexpr double RESTITUTION     = -0.2;
    static constexpr double FRICTION        = 0.99;
    static constexpr double ACCELERATION    = 0.2;
    static constexpr double DECELERATION    = 0.1;
    static constexpr double SPRING_CONSTANT = 0.01;
    static constexpr double DAMPING_FACTOR  = 0.05;
};

#endif // WHEEL_H

#ifndef WHEEL_H
#define WHEEL_H

#include "line.h"
#include <QList>
#include <optional>
#include <array>

class Wheel {
public:
    double x = 0.0, y = 0.0;
    // Made public to allow Car class to apply torque
    double m_vx = 0.0, m_vy = 0.0;

    Wheel(int x, int y, int radius);

    void attach(Wheel* other);
    void simulate(const QList<Line>& lines, bool accelerating, bool braking);
    std::optional<std::array<int, 3>> get(int x1, int y1, int x2, int y2, int cx, int cy) const;
    int getRadius() const;

private:
    int m_radius;
    QList<Wheel*> m_others;
    QList<double> m_distances;
};

#endif // WHEEL_H

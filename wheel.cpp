#include "wheel.h"
#include <QtMath>
#include <algorithm>

Wheel::Wheel(int x_, int y_, int radius)
    : x(x_), y(y_), m_radius(radius) {}

int Wheel::getRadius() const {
    return m_radius;
}

void Wheel::attach(Wheel* other) {
    m_others.append(other);
    double dx = other->x - x;
    double dy = other->y - y;
    m_distances.append(qSqrt(dx * dx + dy * dy));
}

void Wheel::simulate(const QList<Line>& lines, bool accelerating, bool braking) {
    // Integrate position
    x += m_vx;
    y -= m_vy;

    // Apply forces
    m_vy -= 0.02; // Gravity
    m_vx *= 0.9999; // Air resistance
    m_vy *= 0.9999; // Air resistance

    const double friction = 0.999;
    const double maxVelocity = 10.0;

    // Ground collision and response
    for (const Line& line : lines) {
        double m = line.getSlope();
        double b = line.getIntercept();
        double dist = qAbs(m * x - y + b) / qSqrt(m * m + 1);
        double intersection_x = (m * (y - b) + x) / (m * m + 1);

        if (dist < m_radius && intersection_x >= line.getX1() && intersection_x <= line.getX2()) {
            // Correct position to prevent sinking
            while (dist < m_radius) {
                y -= 1.0;
                dist = qAbs(m * x - y + b) / qSqrt(m * m + 1);
            }

            // Decompose velocity
            double theta = -qAtan(m);
            double vAlongLine0 = m_vx * qCos(theta) + m_vy * qSin(theta);
            double vNormalToLine0 = m_vy * qCos(theta) - m_vx * qSin(theta);

            double vAlongLine1 = vAlongLine0 * friction;
            // Sigmoid function for bounce
            double vNormalToLine1 = vNormalToLine0 * (1.0 / (1.0 + qPow(3, -vNormalToLine0)));

            // Apply player input
            if (accelerating && vAlongLine1 < maxVelocity) vAlongLine1 += 0.2;
            if (braking && vAlongLine1 > -maxVelocity) vAlongLine1 -= 0.1;

            // Recompose velocity
            m_vx = vAlongLine1 * qCos(theta) - vNormalToLine1 * qSin(theta);
            m_vy = vAlongLine1 * qSin(theta) + vNormalToLine1 * qCos(theta);
        }
    }

    // Spring-damper constraints
    for (int i = 0; i < m_others.size(); ++i) {
        Wheel* other = m_others.at(i);
        const double springConstant = 0.01;
        const double dampingFactor = 0.05;
        double desiredDistance = m_distances.at(i);

        double deltaX = other->x - x;
        double deltaY = other->y - y;
        double actualDistance = qSqrt(deltaX * deltaX + deltaY * deltaY);
        if (actualDistance == 0) continue;

        double displacement = actualDistance - desiredDistance;
        double springForceMagnitude = displacement * springConstant;

        double unitX = -deltaX / actualDistance;
        double unitY = deltaY / actualDistance;

        double forceX = unitX * springForceMagnitude;
        double forceY = unitY * springForceMagnitude;

        double relativeVx = other->m_vx - m_vx;
        double relativeVy = other->m_vy - m_vy;
        double dampingForceX = relativeVx * dampingFactor;
        double dampingForceY = relativeVy * dampingFactor;

        this->m_vx -= (forceX - dampingForceX);
        this->m_vy -= (forceY - dampingForceY);
        other->m_vx += (forceX - dampingForceX);
        other->m_vy += (forceY - dampingForceY);
    }
}

std::optional<std::array<int, 3>> Wheel::get(int x1, int y1, int x2, int y2, int cx, int cy) const {
    if (x + m_radius + cx < x1 || x - m_radius + cx > x2 || y + m_radius + cy < y1 || y - m_radius + cy > y2)
        return std::nullopt;
    return std::array<int, 3>{
        static_cast<int>(std::lround(x + cx)),
        static_cast<int>(std::lround(y + cy)),
        m_radius
    };
}

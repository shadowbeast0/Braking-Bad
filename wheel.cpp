#include "wheel.h"
#include <cmath>
#include <algorithm>

Wheel::Wheel(int x_, int y_, int radius)
    : x(x_), y(y_), m_radius(radius) {}

void Wheel::attach(Wheel* other) {
    m_others.append(other);
    double dx = other->x - x;
    double dy = other->y - y;
    m_distances.append(std::sqrt(dx * dx + dy * dy));
}

void Wheel::simulate(const QList<Line>& lines, bool accelerating, bool braking) {
    // Integrate position
    x += m_vx;
    y -= m_vy;

    // Gravity
    m_vy -= GRAVITY;

    // Air resistance / drag
    m_vx *= AIR_RESISTANCE;
    m_vy *= AIR_RESISTANCE;

    // Ground collision / response (same as before)
    for (const Line& line : lines) {
        double m = line.getSlope();
        double b = line.getIntercept();

        double dist = std::abs(m * x - y + b) / std::sqrt(m * m + 1);
        double intersection_x = (m * (y - b) + x) / (m * m + 1);

        int minX = std::min(line.getX1(), line.getX2());
        int maxX = std::max(line.getX1(), line.getX2());

        if (dist < m_radius && intersection_x >= minX && intersection_x <= maxX) {
            double overlap = m_radius - dist;
            double normal_angle = std::atan2(-m, 1.0);
            y -= overlap * std::cos(normal_angle);
            x -= overlap * std::sin(normal_angle);

            // Rotate velocity into line coordinates
            double theta = -std::atan(m);
            double vAlongLine     = m_vx * std::cos(theta) + m_vy * std::sin(theta);
            double vNormalToLine  = m_vy * std::cos(theta) - m_vx * std::sin(theta);

            // Response
            vNormalToLine *= RESTITUTION;
            vAlongLine    *= FRICTION;

            // Player input along the tangent
            if (accelerating && vAlongLine <  MAX_VELOCITY) vAlongLine += ACCELERATION;
            if (braking     && vAlongLine > -MAX_VELOCITY) vAlongLine -= DECELERATION;

            // Rotate back to world coordinates
            m_vx = vAlongLine * std::cos(theta) - vNormalToLine * std::sin(theta);
            m_vy = vAlongLine * std::sin(theta) + vNormalToLine * std::cos(theta);
        }
    }

    // Spring-damper constraints between attached wheels (same)
    for (int i = 0; i < m_others.size(); ++i) {
        Wheel* other = m_others.at(i);
        double desiredDistance = m_distances.at(i);

        double deltaX = other->x - x;
        double deltaY = other->y - y;
        double actualDistance = std::sqrt(deltaX * deltaX + deltaY * deltaY);
        if (actualDistance == 0) continue;

        double displacement = actualDistance - desiredDistance;
        double springForceMagnitude = displacement * SPRING_CONSTANT;

        double unitX = -deltaX / actualDistance;
        double unitY =  deltaY / actualDistance;

        double forceX = unitX * springForceMagnitude;
        double forceY = unitY * springForceMagnitude;

        double relativeVx = other->m_vx - m_vx;
        double relativeVy = other->m_vy - m_vy;
        double dampingForceX = relativeVx * DAMPING_FACTOR;
        double dampingForceY = relativeVy * DAMPING_FACTOR;

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

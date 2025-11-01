#include "wheel.h"
#include <cmath>
#include <algorithm>

Wheel::Wheel(int x_, int y_, int radius)
    : x(x_), y(y_), m_radius(radius)
{}

int Wheel::radius() const { return m_radius; }

void Wheel::attach(Wheel* other) {
    m_others.append(other);

    double dx = other->x - x;
    double dy = other->y - y;
    m_distances.append(std::sqrt(dx * dx + dy * dy));

    m_isRoot = true;
    other->m_isRoot = false;
}

void Wheel::simulate(const QList<Line>& lines, bool accelerating, bool braking, bool nitro)
{
    // integrate position
    x += m_vx;
    y -= m_vy;

    // gravity
    m_vy -= GRAVITY;

    // air drag
    m_vx *= AIR_RESISTANCE;
    m_vy *= AIR_RESISTANCE;

    // collision with terrain lines
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

            // push wheel out of ground
            y -= overlap * std::cos(normal_angle);
            x -= overlap * std::sin(normal_angle);

            // rotate velocity into slope frame
            double theta = -std::atan(m);
            double vAlongLine    = m_vx * std::cos(theta) + m_vy * std::sin(theta);
            double vNormalToLine = m_vy * std::cos(theta) - m_vx * std::sin(theta);

            // bounce normal
            vNormalToLine *= (vNormalToLine < 0.2) ? RESTITUTION : 1;

            // tangential friction / clamp
            if ((vAlongLine >  MAX_VELOCITY / 1000) ||
                (vAlongLine < -MAX_VELOCITY / 1000))
            {
                vAlongLine += ((vAlongLine > 0) ? -1 : 1) * MAX_VELOCITY / 1500;
            } else {
                vAlongLine = 0;
            }

            // driving force along tangent
            if (accelerating && vAlongLine <  MAX_VELOCITY) {
                vAlongLine += (ACCELERATION *
                               (1 - (vAlongLine / MAX_VELOCITY)));
            }
            if (braking      && vAlongLine > -MAX_VELOCITY) {
                vAlongLine -= (DECELERATION *
                               (1 + (vAlongLine / MAX_VELOCITY)));
            }

            // rotate back to world frame
            m_vx = vAlongLine    * std::cos(theta)
                   - vNormalToLine * std::sin(theta);
            m_vy = vAlongLine    * std::sin(theta)
                   + vNormalToLine * std::cos(theta);

            // collision bleeds some spin energy
            if (m_isRoot && m_others.size() > 0 && dist < m_radius) {
                m_omega *= 0.9;
            }
        }
    }

    // shared body tilt / rotation between two wheels
    if (m_isRoot && m_others.size() > 0) {
        Wheel* other = m_others.first();

        // angular control:
        // 1. nitro: mild damping of omega (straight flight)
        // 2. both accel+brake: stabilizing tilt + gravity relief
        // 3. accel only: tilt backward (wheelie)
        // 4. brake only: tilt forward (nose down)
        // 5. none: passive damping

        if (nitro) {
            m_omega *= 0.96;
        } else if (accelerating && braking) {
            // stabilization branch (what you had in code 1 for both keys)
            m_omega *= 0.5;
            const double RESTORING_FACTOR = 0.002;
            m_omega -= RESTORING_FACTOR * m_angle;

            // small upward assist (same trick you had)
            m_vy -= GRAVITY / 1.5;
        } else if (accelerating) {
            m_omega -= ANGULAR_ACCELERATION;
            if (m_omega < -MAX_ANGULAR_VELOCITY) {
                m_omega = -MAX_ANGULAR_VELOCITY;
            }
        } else if (braking) {
            m_omega += ANGULAR_DECELERATION;
            if (m_omega >  MAX_ANGULAR_VELOCITY) {
                m_omega =  MAX_ANGULAR_VELOCITY;
            }
        } else {
            // passive damp
            m_omega *= ANGULAR_DAMPING;
            if (std::abs(m_omega) < 1e-4) {
                m_omega = 0.0;
            }
        }

        // integrate angle
        m_angle += m_omega;
        if (m_angle >  2 * M_PI) m_angle -= 2 * M_PI;
        else if (m_angle < -2 * M_PI) m_angle += 2 * M_PI;

        // apply incremental rotation (about COM), not teleport
        if (std::abs(m_omega) > 1e-6) {
            double cx = (x + other->x) / 2.0;
            double cy = (y + other->y) / 2.0;

            double rx1 = x        - cx;
            double ry1 = y        - cy;
            double rx2 = other->x - cx;
            double ry2 = other->y - cy;

            double sinA = std::sin(m_omega);
            double cosA = std::cos(m_omega);

            double nx1 = rx1 * cosA - ry1 * sinA;
            double ny1 = rx1 * sinA + ry1 * cosA;

            double nx2 = rx2 * cosA - ry2 * sinA;
            double ny2 = rx2 * sinA + ry2 * cosA;

            x        = cx + nx1;
            y        = cy + ny1;
            other->x = cx + nx2;
            other->y = cy + ny2;
        }
    }

    // spring-damper keeping wheel distances
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

        m_vx -= (forceX - dampingForceX);
        m_vy -= (forceY - dampingForceY);
        other->m_vx += (forceX - dampingForceX);
        other->m_vy += (forceY - dampingForceY);
    }
}

std::optional<std::array<int, 3>> Wheel::get(int x1, int y1, int x2, int y2, int cx, int cy) const
{
    if (x + m_radius + cx < x1 ||
        x - m_radius + cx > x2 ||
        y + m_radius + cy < y1 ||
        y - m_radius + cy > y2)
    {
        return std::nullopt;
    }

    return std::array<int,3>{
        static_cast<int>(std::lround(x + cx)),
        static_cast<int>(std::lround(y + cy)),
        m_radius
    };
}

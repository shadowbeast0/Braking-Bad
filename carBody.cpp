#include "carBody.h"

#include <cmath>
#include <climits>
#include <algorithm>
#include <cstdlib>
#include <QPoint>
#include <constants.h>

CarBody::CarBody() : m_cx(0.0), m_cy(0.0), m_angle(0.0), m_vx(0.0), m_vy(0.0), m_isAlive(true) {}

CarBody::~CarBody() {
    m_wheels.clear();
}

int CarBody::getX() const {
    return static_cast<int>(std::round(m_cx));
}

int CarBody::getY() const {
    return static_cast<int>(std::round(m_cy));
}

void CarBody::addPoints(const QVector<QPoint>& points) {
    for(QPoint p : points){
        m_points.append(Point(p.x(), p.y()));
    }
}

void CarBody::addHitbox(const QVector<QPoint>& points){
    for(QPoint p : points){
        hitbox.append(Point(p.x(), p.y()));
    }
}

void CarBody::addKillSwitches(const QVector<QPoint>& points) {
    m_killSwitches.reserve(points.size());
    for (const QPoint& p : points) {
        m_killSwitches.append(Point(p.x(), p.y()));
    }
}

void CarBody::finish() {
    int cxmin = INT_MAX, cymin = INT_MAX;
    int cxmax = INT_MIN, cymax = INT_MIN;
    for (const Point& point : m_points) {
        cxmin = std::min(cxmin, static_cast<int>(std::round(point.coords[0])));
        cymin = std::min(cymin, static_cast<int>(std::round(point.coords[1])));
        cxmax = std::max(cxmax, static_cast<int>(std::round(point.coords[0])));
        cymax = std::max(cymax, static_cast<int>(std::round(point.coords[1])));
    }

    m_cx = (cxmin + cxmax) / 2.0;
    m_cy = (cymin + cymax) / 2.0;

    for (Point& point : m_points) {
        point.coords[0] -= m_cx;
        point.coords[1] -= m_cy;
    }

    for (auto& entry : m_attachments) {
        for (Point& point : entry.first) {
            point.coords[0] -= m_cx;
            point.coords[1] -= m_cy;
        }
    }

    for (auto& point : hitbox) {
        point.coords[0] -= m_cx;
        point.coords[1] -= m_cy;
    }

    for (Point& point : m_killSwitches) {
        point.coords[0] -= m_cx;
        point.coords[1] -= m_cy;
    }

    if (m_wheels.isEmpty()) return;

    double averagex = 0;
    double averagey = 0;
    double averager = 0;

    for (Wheel* wheel : m_wheels) {
        averagex += wheel->getX();
        averagey += wheel->getY();
        averager += wheel->radius();
    }
    averagex /= m_wheels.size();
    averagey /= m_wheels.size();
    averager /= m_wheels.size();

    wheel_average_desired_distance = averager * 3;

    double theta = std::atan2(m_wheels.last()->getY() - m_wheels.first()->getY(), m_wheels.last()->getX() - m_wheels.first()->getX());

    move(averagex + static_cast<int>(std::round(wheel_average_desired_distance * std::sin(theta))), averagey - static_cast<int>(std::round(wheel_average_desired_distance * std::cos(theta))), theta);

    for (Wheel* wheel : m_wheels) {
        attach(wheel);
    }
}

QVector<QPoint> CarBody::get(int dx, int dy) {
    QVector<QPoint> newPoints;
    auto newCenter = Point(m_cx, m_cy).get(dx, dy, 0);
    for(const Point& point : m_points) {
        newPoints.append(QPoint(point.coords[0] + newCenter[0], point.coords[1] + newCenter[1]));
    }
    return newPoints;
}

void CarBody::move(int dx, int dy, double angle) {
    m_cx = dx;
    m_cy = dy;
    double angleDelta = angle - m_angle;

    for (Point& point : m_points) {
        point.coords = Point::rotate(point.coords, angleDelta);
    }
    for (Point& point : hitbox) {
        point.coords = Point::rotate(point.coords, angleDelta);
    }
    for (auto& entry : m_attachments) {
        for (Point& point : entry.first) {
            point.coords = Point::rotate(point.coords, angleDelta);
        }
    }
    for (Point& killSwitch : m_killSwitches) {
        killSwitch.coords = Point::rotate(killSwitch.coords, angleDelta);
    }
    m_angle = angle;
}

void CarBody::rotate(double angle) {
    double angleDelta = angle - m_angle;
    for (Point& point : m_points) {
        point.coords = Point::rotate(point.coords, angleDelta);
    }
    for (Point& point : hitbox) {
        point.coords = Point::rotate(point.coords, angleDelta);
    }
    for (auto& entry : m_attachments) {
        for (Point& point : entry.first) {
            point.coords = Point::rotate(point.coords, angleDelta);
        }
    }
    for (Point& killSwitch : m_killSwitches) {
        killSwitch.coords = Point::rotate(killSwitch.coords, angleDelta);
    }
    m_angle = angle;
}

void CarBody::addWheel(Wheel* wheel) {
    m_wheels.append(wheel);
}

void CarBody::kill() {
    m_isAlive = false;
    for (Wheel* wheel : m_wheels) {
        wheel->kill();
        double rand_vx = ((double)std::rand() / RAND_MAX) * 10.0 - 5.0;
        double rand_vy = ((double)std::rand() / RAND_MAX) * 10.0 - 5.0;
        wheel->updateV(rand_vx, rand_vy);
    }
    m_wheels.clear();
    m_attachDistances.clear();
}

bool CarBody::isAlive() const {
    return m_isAlive;
}

void CarBody::attach(Wheel* wheel) {
    double dist = std::sqrt(std::pow(wheel->getX() - m_cx, 2) + std::pow(wheel->getY() - m_cy, 2));
    m_attachDistances.append(dist);
}

QVector<Line> CarBody::getLines() {
    QVector<Line> lines;
    if (m_points.isEmpty()) return lines;

    lines.reserve(m_points.size());
    for (int i = 0; i < m_points.size(); i++) {
        std::array<int, 2> p1 = m_points.at((i + 1) % m_points.size()).get(static_cast<int>(std::round(m_cx)), static_cast<int>(std::round(m_cy)), m_angle);
        std::array<int, 2> p2 = m_points.at(i).get(static_cast<int>(std::round(m_cx)), static_cast<int>(std::round(m_cy)), m_angle);
        lines.append(Line(p1[0], p1[1], p2[0], p2[1]));
    }
    return lines;
}

void CarBody::simulate(int level_index, const QVector<Line>& terrain, bool accelerating, bool braking) {
    if (m_isAlive && m_wheels.size() >= 2) {
        double theta = std::atan2(m_wheels[1]->getY() - m_wheels[0]->getY(), m_wheels[1]->getX() - m_wheels[0]->getX());
        rotate(theta);
    }

    m_cx += m_vx;
    m_cy -= m_vy;

    m_vy -= Constants::GRAVITY[level_index];

    m_vx *= 1 - Constants::AIR_RESISTANCE[level_index];
    m_vy *= 1 - Constants::AIR_RESISTANCE[level_index];

    if(accelerating && braking){
        m_vy -= Constants::GRAVITY[level_index] * 0.5;
    }

    for (const Line& line : terrain) {
        double m = line.getSlope();
        double b = line.getIntercept();

        for (const Point& point : hitbox) {
            int x = static_cast<int>(std::round(m_cx + point.coords[0]));
            int y = static_cast<int>(std::round(m_cy + point.coords[1]));

            double dist = std::abs(m * x - y + b) / std::sqrt(m * m + 1);
            double intersectionx = (m * (y - b) + x) / (m * m + 1);

            if (dist <= 4 && intersectionx >= line.getX1() - 1 && intersectionx <= line.getX2() + 1) {
                double intersectiony = m * intersectionx + b;
                double theta = -std::atan(m);

                while (dist < 4) {
                    m_cy -= std::cos(theta);
                    m_cx -= std::sin(theta);
                    int cur_x = static_cast<int>(std::round(m_cx + point.coords[0]));
                    int cur_y = static_cast<int>(std::round(m_cy + point.coords[1]));
                    dist = std::abs(m * cur_x - cur_y + b) / std::sqrt(m * m + 1);
                }

                double vAlongLine = m_vx * std::cos(theta) + m_vy * std::sin(theta);
                double vNormalToLine = m_vy * std::cos(theta) - m_vx * std::sin(theta);

                vNormalToLine = vNormalToLine * Constants::RESTITUTION[level_index] / (1 + std::exp(-vNormalToLine));
                vAlongLine *= 1 - Constants::FRICTION[level_index];

                m_vx = vAlongLine * std::cos(theta) - vNormalToLine * std::sin(theta);
                m_vy = vAlongLine * std::sin(theta) + vNormalToLine * std::cos(theta);
            }
        }

        for (const Point& point : m_killSwitches) {
            int x = static_cast<int>(std::round(m_cx + point.coords[0]));
            int y = static_cast<int>(std::round(m_cy + point.coords[1]));

            double dist = std::abs(m * x - y + b) / std::sqrt(m * m + 1);
            double intersectionx = (m * (y - b) + x) / (m * m + 1);

            if (dist <= 4 && intersectionx >= line.getX1() - 1 && intersectionx <= line.getX2() + 1) {

                if (m_isAlive) kill();

                double intersectiony = m * intersectionx + b;
                double theta = -std::atan(m);

                while (dist < 4) {
                    m_cy -= std::cos(theta);
                    m_cx -= std::sin(theta);
                    int cur_x = static_cast<int>(std::round(m_cx + point.coords[0]));
                    int cur_y = static_cast<int>(std::round(m_cy + point.coords[1]));
                    dist = std::abs(m * cur_x - cur_y + b) / std::sqrt(m * m + 1);
                }

                double vAlongLine = m_vx * std::cos(theta) + m_vy * std::sin(theta);
                double vNormalToLine = m_vy * std::cos(theta) - m_vx * std::sin(theta);

                vNormalToLine = vNormalToLine * Constants::RESTITUTION[level_index] / (1 + std::exp(-vNormalToLine));
                vAlongLine *= 1 - Constants::FRICTION[level_index];

                m_vx = vAlongLine * std::cos(theta) - vNormalToLine * std::sin(theta);
                m_vy = vAlongLine * std::sin(theta) + vNormalToLine * std::cos(theta);

                double torque = 0.01 * ((intersectionx - m_cx) * std::sin(theta) - (intersectiony - m_cy) * std::cos(theta));
                m_angle += 100 * torque;
            }
        }
    }

    for (int i = 0; m_isAlive && i < m_wheels.size(); i++) {
        Wheel* wheel = m_wheels.at(i);
        double desiredDistance = m_attachDistances.at(i);

        double deltaX = wheel->getX() - m_cx;
        double deltaY = wheel->getY() - m_cy;
        double actualDistance = std::sqrt(deltaX * deltaX + deltaY * deltaY);

        if (actualDistance == 0) continue;

        double displacement = actualDistance - desiredDistance;
        double springForceMagnitude = displacement * Constants::SPRING_CONSTANT;

        double unitX = -deltaX / actualDistance;
        double unitY =  deltaY / actualDistance;

        double forceX = unitX * springForceMagnitude;
        double forceY = unitY * springForceMagnitude;

        // 2. Calculate relative velocity
        double relativeVx = wheel->getVx() - m_vx;
        double relativeVy = wheel->getVy() - m_vy;

        // 3. Project relative velocity onto the spring axis (Dot Product)
        // This isolates the component of velocity that is stretching/compressing the spring
        double velocityAlongSpring = relativeVx * unitX + relativeVy * unitY;

        // 4. Calculate damping force components based ONLY on that projection
        double dampingForceX = velocityAlongSpring * unitX * Constants::DAMPING;
        double dampingForceY = velocityAlongSpring * unitY * Constants::DAMPING;

        m_vx -= (forceX - dampingForceX);
        m_vy -= (forceY - dampingForceY);
        wheel->updateV(forceX - dampingForceX, forceY - dampingForceY);
    }

    if (!m_isAlive) {
        double pushUp = 0.0;
        for (const Point& p : hitbox) {
            int px = int(std::lround(m_cx + p.coords[0]));
            int py = int(std::lround(m_cy + p.coords[1]));
            for (const Line& line : terrain) {
                int x1 = line.getX1(), x2 = line.getX2();
                if (!((x1 <= px && px <= x2) || (x2 <= px && px <= x1))) continue;
                double m = line.getSlope();
                double b = line.getIntercept();
                double gy = m * px + b;
                double clear = 4.0;
                double need = py - (gy - clear);
                if (need > pushUp) pushUp = need;
                break;
            }
        }
        if (pushUp > 0.0)
            m_cy -= pushUp;
    }
}

void CarBody::addAttachment(const QVector<QPoint>& points, const QColor& color) {
    QVector<Point> pts;
    pts.reserve(points.size());
    for (QPoint p : points) pts.append(Point(p.x(), p.y()));
    m_attachments.append(qMakePair(pts, color));
}

QVector<QPair<QVector<QPoint>, QColor>> CarBody::getAttachments(int dx, int dy) {
    QVector<QPair<QVector<QPoint>, QColor>> out;
    out.reserve(m_attachments.size());
    auto newCenter = Point(m_cx, m_cy).get(dx, dy, 0);
    for (const auto& entry : m_attachments) {
        QVector<QPoint> poly;
        poly.reserve(entry.first.size());
        for (const Point& pt : entry.first) {
            poly.append(QPoint(pt.coords[0] + newCenter[0], pt.coords[1] + newCenter[1]));
        }
        out.append(qMakePair(poly, entry.second));
    }
    return out;
}

QVector<QPoint> CarBody::getKillSwitches(int dx, int dy) const {
    QVector<QPoint> out;
    out.reserve(m_killSwitches.size());
    auto c = Point(m_cx, m_cy).get(dx, dy, 0);
    for (const Point& p : m_killSwitches) {
        out.append(QPoint(p.coords[0] + c[0], p.coords[1] + c[1]));
    }
    return out;
}

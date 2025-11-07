#ifndef CARBODY_H
#define CARBODY_H

#include <QVector>
#include <QMap>
#include <QPair>
#include <QColor>
#include <QPoint>

#include "point.h"
#include "wheel.h"
#include "line.h"

class CarBody {
public:
    CarBody();
    virtual ~CarBody();

    int getX() const;
    int getY() const;

    void addPoints(const QVector<QPoint>& points);

    void addHitbox(const QVector<QPoint>& points);

    void addKillSwitches(const QVector<QPoint>& points);

    void finish();

    void addAttachment(const QVector<QPoint>& points, const QColor& color);
    QVector<QPair<QVector<QPoint>, QColor>> getAttachments(int dx, int dy);

    QVector<QPoint> get(int dx, int dy);

    void move(int dx, int dy, double angle);
    void rotate(double angle);

    void addWheel(Wheel* wheel);

    void kill();
    bool isAlive() const;

    QVector<Line> getLines();

    void simulate(int, const QVector<Line>& terrain, bool accelerating, bool braking);

    QVector<QPoint> getKillSwitches(int dx, int dy) const;

private:
    void attach(Wheel* wheel);

    QVector<Point> m_points;

    QVector<Point> hitbox;

    double m_cx = 0.0;
    double m_cy = 0.0;
    double m_angle = 0.0;
    double m_vx = 0.0;
    double m_vy = 0.0;

    QVector<Wheel*> m_wheels;
    QVector<double> m_attachDistances;

    double wheel_average_desired_distance;

    QVector<QPair<QVector<Point>, QColor>> m_attachments;

    QVector<Point> m_killSwitches;
    bool m_isAlive = true;
};

#endif // CARBODY_H

#ifndef POINT_H
#define POINT_H

#include <array>

class Point {
public:
    std::array<double, 2> coords;

    Point(double x = 0.0, double y = 0.0);

    static std::array<double, 2> translate(const std::array<double, 2>& coords, double dx, double dy);

    static std::array<double, 2> rotate(const std::array<double, 2>& coords, double angle);

    static std::array<int, 2> round(const std::array<double, 2>& coords);

    std::array<int, 2> get(double dx, double dy, double angle) const;
};

#endif // POINT_H

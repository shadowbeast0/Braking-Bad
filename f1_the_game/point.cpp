#include "point.h"
#include <cmath>

Point::Point(double x, double y) : coords{x, y} {
}

std::array<double, 2> Point::translate(const std::array<double, 2>& coords, double dx, double dy) {
    return {coords[0] + dx, coords[1] + dy};
}

std::array<double, 2> Point::rotate(const std::array<double, 2>& coords, double angle) {
    double newX = coords[0] * std::cos(angle) - coords[1] * std::sin(angle);
    double newY = coords[0] * std::sin(angle) + coords[1] * std::cos(angle);
    return {newX, newY};
}

std::array<int, 2> Point::round(const std::array<double, 2>& coords) {
    return {static_cast<int>(std::round(coords[0])), static_cast<int>(std::round(coords[1]))};
}

std::array<int, 2> Point::get(double dx, double dy, double angle) const {
    return round(rotate(translate(coords, dx, dy), angle));
}

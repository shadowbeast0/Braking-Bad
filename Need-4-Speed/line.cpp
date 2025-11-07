#include "line.h"
#include <cmath>

Line::Line(int x1, int y1, int x2, int y2)
    : m_x1(x1), m_y1(y1), m_x2(x2), m_y2(y2) {
    if (m_x2 - m_x1 == 0) {
        m_slope = 1e9;
    } else {
        m_slope = static_cast<double>(m_y2 - m_y1) / static_cast<double>(m_x2 - m_x1);
    }
    m_intercept = m_y1 - m_slope * m_x1;
}

int Line::getX1() const { return m_x1; }
int Line::getX2() const { return m_x2; }
int Line::getY1() const {return m_y1; }
int Line::getY2() const {return m_y2; }
double Line::getSlope() const { return m_slope; }
double Line::getIntercept() const { return m_intercept; }

std::optional<std::array<int, 4>> Line::get(int x1bound, int y1bound, int x2bound, int y2bound, int dx, int dy) const {
    int screen_x1 = m_x1 + dx;
    int screen_y1 = m_y1 + dy;
    int screen_x2 = m_x2 + dx;
    int screen_y2 = m_y2 + dy;

    if ((screen_x1 < x1bound && screen_x2 < x1bound) ||
        (screen_x1 > x2bound && screen_x2 > x2bound) ||
        (screen_y1 < y1bound && screen_y2 < y1bound) ||
        (screen_y1 > y2bound && screen_y2 > y2bound)) {
        return std::nullopt;
    }

    return std::array<int, 4>{screen_x1, screen_y1, screen_x2, screen_y2};
}

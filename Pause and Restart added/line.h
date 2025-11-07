#ifndef LINE_H
#define LINE_H

#include <optional>
#include <array>

class Line {
public:
    Line(int x1, int y1, int x2, int y2);
    int getX1() const;
    int getX2() const;
    int getY1() const;
    int getY2() const;
    double getSlope() const;
    double getIntercept() const;
    std::optional<std::array<int, 4>> get(int x1bound, int y1bound, int x2bound, int y2bound, int dx, int dy) const;
private:
    int m_x1, m_y1, m_x2, m_y2;
    double m_slope, m_intercept;
};

#endif // LINE_H

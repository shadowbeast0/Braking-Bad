#ifndef CAR_H
#define CAR_H

#include <QImage>

// Forward declarations to avoid circular dependencies
class Wheel;
class QPainter;

class Car {
public: // <-- MODIFIED: All members are now public for easy access
    Car(Wheel* backWheel, Wheel* frontWheel);

    void update(bool accelerating, bool braking);
    void draw(QPainter& p, int camX, int camY);
    bool isFlipped() const;

    // MODIFIED: Moved constants from private to public
    static constexpr double IMG_BACK_CX      = 36.0;
    static constexpr double IMG_FRONT_CX     = 114.0;
    static constexpr double IMG_WHEEL_BASE   = IMG_FRONT_CX - IMG_BACK_CX;
    static constexpr double IMG_WHEEL_RADIUS = 12.0;

private:
    void loadImage();

    Wheel* m_backWheel = nullptr;
    Wheel* m_frontWheel = nullptr;
    QImage m_image;

    // Constants ported from the Java version
    static constexpr double IMG_BACK_CY      = 112.0;
    static constexpr double TORQUE_FORCE     = 0.05;

    // State variables
    double m_angle = 0.0;
    double m_drawScale = 1.0;
    double m_fineDY = 18.0; // Vertical offset for sprite
    bool m_isFlipped = false;
};

#endif // CAR_H

#include "car.h"
#include "wheel.h"
#include <QPainter>
#include <QtMath> // For M_PI and atan2

Car::Car(Wheel* backWheel, Wheel* frontWheel)
    : m_backWheel(backWheel), m_frontWheel(frontWheel) {
    loadImage();
}

void Car::loadImage() {
    // Load the image from Qt's resource system.
    // Make sure you have a .qrc file in your project!
    m_image.load(":/shino/car.png");
    if (m_image.isNull()) {
        qWarning("Failed to load car.png from resources!");
    }
}

void Car::update(bool accelerating, bool braking) {
    if (!m_backWheel || !m_frontWheel) return;

    // Apply torque by directly changing wheel vertical velocities
    if (accelerating) {
        m_backWheel->m_vy -= TORQUE_FORCE;
        m_frontWheel->m_vy += TORQUE_FORCE;
    }
    if (braking) {
        m_backWheel->m_vy += TORQUE_FORCE;
        m_frontWheel->m_vy -= TORQUE_FORCE;
    }

    // Calculate angle based on the physical positions of the wheels
    m_angle = qAtan2(m_frontWheel->y - m_backWheel->y, m_frontWheel->x - m_backWheel->x);

    // Check if the car is flipped (angle > 90 degrees)
    if (qAbs(m_angle) > M_PI / 2.0) {
        m_isFlipped = true;
    }

    // Calculate draw scale based on wheel size
    double rAvg = (m_backWheel->getRadius() + m_frontWheel->getRadius()) * 0.5;
    m_drawScale = rAvg / IMG_WHEEL_RADIUS;
}

bool Car::isFlipped() const {
    return m_isFlipped;
}

void Car::draw(QPainter& p, int camX, int camY) {
    if (m_image.isNull() || !m_backWheel) return;

    p.save(); // Save painter state

    // Get screen coordinates of the back wheel
    const double screenBackX = m_backWheel->x - camX;
    const double screenBackY = m_backWheel->y + camY;

    // Apply transformations (Translate, Rotate, Scale)
    p.translate(screenBackX, screenBackY);
    p.rotate(qRadiansToDegrees(m_angle)); // qAtan2 is in radians, rotate is in degrees
    p.scale(m_drawScale, m_drawScale);
    p.translate(-IMG_BACK_CX, -IMG_BACK_CY + m_fineDY);

    // Draw the car image
    p.drawImage(0, 0, m_image);

    p.restore(); // Restore painter state
}

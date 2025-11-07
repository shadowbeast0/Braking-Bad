#pragma once
#include <QPainter>
#include <Qt>

class KeyLog {
public:
    void setPressed(int key, bool down) {
        if (key == Qt::Key_W) m_w = down;
        else if (key == Qt::Key_A) m_a = down;
        else if (key == Qt::Key_D) m_d = down;
    }
    void draw(QPainter& p, int screenW, int screenH, int pixelSize);

private:
    bool m_w = false, m_a = false, m_d = false;

    static inline void plot(QPainter& p, int gx, int gy, int ps, const QColor& c) {
        p.fillRect(gx*ps, gy*ps, ps, ps, c);
    }
    void drawKey(QPainter& p, int gx, int gy, int w, int h, int ps, QChar label, bool pressed);
    void drawGlyph(QPainter& p, int gx, int gy, int w, int h, int ps, QChar ch, const QColor& color);
};

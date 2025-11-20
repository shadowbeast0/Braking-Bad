#include "keylog.h"
#include "constants.h"
#include <QRect>
#include <algorithm>

void KeyLog::draw(QPainter& p, int screenW, int screenH, int ps) {
    const int gridW = screenW / ps;
    const int gridH = screenH / ps;

    const int margin = 2;
    const int gap    = 1;
    const int keyW   = 11;
    const int keyH   = 9;

    const int packW  = 2*keyW + gap;
    const int packH  = 2*keyH + gap;

    const int gx0 = gridW - margin - packW;
    const int gy0 = gridH - margin - packH;

    const int gxW = gx0 + (packW - keyW)/2;
    const int gyW = gy0;

    const int gxA = gx0;
    const int gyA = gy0 + keyH + gap;

    const int gxD = gx0 + keyW + gap;
    const int gyD = gyA;

    p.save();
    drawKey(p, gxW, gyW, keyW, keyH, ps, QChar('W'), m_w);
    drawKey(p, gxA, gyA, keyW, keyH, ps, QChar('A'), m_a);
    drawKey(p, gxD, gyD, keyW, keyH, ps, QChar('D'), m_d);
    p.restore();
}

void KeyLog::drawKey(QPainter& p, int gx, int gy, int w, int h, int ps, QChar label, bool pressed) {
    const QColor fill(120,120,130,40);
    const QColor borderDim(70,70,80);
    const QColor borderHot(255,255,255);

    for (int y=0; y<h; ++y)
        for (int x=0; x<w; ++x)
            plot(p, gx+x, gy+y, ps, fill);

    const QColor bc = pressed ? borderHot : borderDim;
    for (int x=0; x<w; ++x) { plot(p, gx+x, gy+0, ps, bc); plot(p, gx+x, gy+h-1, ps, bc); }
    for (int y=0; y<h; ++y) { plot(p, gx+0, gy+y, ps, bc); plot(p, gx+w-1, gy+y, ps, bc); }

    drawGlyph(p, gx, gy, w, h, ps, label, QColor(255,255,255));
}

void KeyLog::drawGlyph(QPainter& p, int gx, int gy, int w, int h, int ps, QChar ch, const QColor& color) {
    auto it = font_map.find(ch.toUpper());
    if (it == font_map.end()) return;

    const int gw = 5, gh = 7;
    const int pad = 2;
    const int innerW = std::max(0, w - 2*pad);
    const int innerH = std::max(0, h - 2*pad);
    const int cell = std::max(1, std::min(innerW / gw, innerH / gh));
    const int ox = gx + pad + (innerW - gw*cell)/2;
    const int oy = gy + pad + (innerH - gh*cell)/2;

    const auto& rows = it.value();
    for (int r=0; r<gh; ++r) {
        uint8_t row = rows[r];
        for (int c=0; c<gw; ++c) {
            if (row & (1 << (gw-1-c))) {
                for (int yy=0; yy<cell; ++yy)
                    for (int xx=0; xx<cell; ++xx)
                        plot(p, ox + c*cell + xx, oy + r*cell + yy, ps, color);
            }
        }
    }
}

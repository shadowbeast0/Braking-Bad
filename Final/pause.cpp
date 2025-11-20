#include "pause.h"
#include <QPainter>
#include <QMouseEvent>
#include <algorithm>

PauseOverlay::PauseOverlay(QWidget* parent) : QWidget(parent) {
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_NoSystemBackground, false);
    setFocusPolicy(Qt::NoFocus);
    connect(&m_timer, &QTimer::timeout, this, [this]{
        if (--m_count <= 0) { m_timer.stop(); emit resumeRequested(); }
        update();
    });
}

void PauseOverlay::setLevelIndex(int idx) {
    m_levelIndex = idx;
}

void PauseOverlay::showPaused() {
    m_state = Paused;
    m_count = 3;
    m_timer.stop();
    show();
    raise();
    update();
}

void PauseOverlay::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.fillRect(rect(), QColor(0,0,0,150));

    const int gw = gridW();
    const int gh = gridH();
    const QString title = "GAME PAUSED";

    int ts = std::clamp(gh / (7 * 8), 2, 6);
    int tw = textWidthCells(title, ts);
    int tgx = (gw - tw) / 2;
    int tgy = gh / 3;
    drawPixelText(p, title, tgx, tgy, ts, Constants::TEXT_COLOR[m_levelIndex], true);

    if (m_state == Paused) {
        QRect r = resumeRectPx();
        p.setPen(Qt::NoPen);
        p.setBrush(QColor(0,0,0,160));
        p.drawRect(r.translated(3*Constants::PIXEL_SIZE, 3*Constants::PIXEL_SIZE));
        p.setBrush(QColor(180,180,190));
        p.drawRect(r);
        const QString lab = "RESUME";
        int rc = r.width()  / Constants::PIXEL_SIZE;
        int rr = r.height() / Constants::PIXEL_SIZE;
        int s  = std::clamp(std::min(rc / (int(lab.size())*CHAR_ADV - (CHAR_ADV-5)), rr / 2), 1, 6);
        int gx = r.left()/Constants::PIXEL_SIZE + (rc - textWidthCells(lab, s))/2;
        int gy = r.top() /Constants::PIXEL_SIZE + (rr - 7*s)/2;
        drawPixelText(p, lab, gx, gy, s, QColor(25,25,28), false);
        m_resumeRectPx = r;
    } else {
        const QString num = QString::number(m_count);
        int ns = std::clamp(gh / (7 * 4), 3, 10);
        int nw = textWidthCells(num, ns);
        int ngx = (gw - nw) / 2;
        int ngy = tgy + 7*ts + 8;
        drawPixelText(p, num, ngx, ngy, ns, Constants::TEXT_COLOR[m_levelIndex], true);
    }
}

void PauseOverlay::mousePressEvent(QMouseEvent* e) {
    if (m_state != Paused) return;
    if (resumeRectPx().contains(e->pos())) {
        m_state = CountingDown;
        m_count = 3;
        m_timer.start(1000);
        update();
    }
}

void PauseOverlay::resizeEvent(QResizeEvent*) {
    update();
}

int PauseOverlay::textWidthCells(const QString& s, int scale) const {
    if (s.isEmpty()) return 0;
    return (int(s.size()) - 1) * CHAR_ADV * scale + 5 * scale;
}

void PauseOverlay::drawPixelText(QPainter& p, const QString& s, int gx, int gy, int scale, const QColor& c, bool bold) {
    auto plot = [&](int x,int y,const QColor& col){ plotGridPixel(p, gx + x, gy + y, col); };
    for (int i = 0; i < s.size(); ++i) {
        const QChar ch = s.at(i).toUpper();
        const auto rows = font_map.value(font_map.contains(ch) ? ch : QChar(' '));
        int baseOff = i * CHAR_ADV * scale;
        for (int ry=0; ry<7; ++ry) {
            uint8_t row = rows[ry];
            for (int rx=0; rx<5; ++rx) {
                if (row & (1<<(4-rx))) {
                    for (int sy=0; sy<scale; ++sy) {
                        for (int sx=0; sx<scale; ++sx) {
                            if (bold) {
                                plot(baseOff + rx*scale+sx-1, ry*scale+sy, c);
                                plot(baseOff + rx*scale+sx+1, ry*scale+sy, c);
                                plot(baseOff + rx*scale+sx, ry*scale+sy-1, c);
                                plot(baseOff + rx*scale+sx, ry*scale+sy+1, c);
                            }
                            plot(baseOff + rx*scale+sx, ry*scale+sy, c);
                        }
                    }
                }
            }
        }
    }
}

void PauseOverlay::plotGridPixel(QPainter& p, int gx, int gy, const QColor& c) {
    if (gx < 0 || gy < 0 || gx >= gridW()+1 || gy >= gridH()+1) return;
    p.fillRect(gx * Constants::PIXEL_SIZE, gy * Constants::PIXEL_SIZE, Constants::PIXEL_SIZE, Constants::PIXEL_SIZE, c);
}

QRect PauseOverlay::resumeRectPx() const {
    int rc = std::min(std::max(gridW()/6, 30), 60);
    int rr = std::max(10, gridH()/18);
    int gx = (gridW() - rc) / 2;
    int gy = gridH()/2;
    return QRect(gx*Constants::PIXEL_SIZE, gy*Constants::PIXEL_SIZE, rc*Constants::PIXEL_SIZE, rr*Constants::PIXEL_SIZE);
}

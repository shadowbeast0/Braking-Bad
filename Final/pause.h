#pragma once
#include <QWidget>
#include <QTimer>
#include <QRect>
#include <QString>
#include "constants.h"

class PauseOverlay : public QWidget {
    Q_OBJECT
public:
    explicit PauseOverlay(QWidget* parent=nullptr);
    void setLevelIndex(int idx);
    void showPaused();
signals:
    void resumeRequested();
protected:
    void paintEvent(QPaintEvent*) override;
    void mousePressEvent(QMouseEvent*) override;
    void resizeEvent(QResizeEvent*) override;
private:
    int m_levelIndex = 0;
    enum State { Paused, CountingDown } m_state = Paused;
    int m_count = 3;
    QTimer m_timer;
    QRect m_resumeRectPx;
    static constexpr int CHAR_ADV = 7;
    inline int gridW() const { return width()  / Constants::PIXEL_SIZE; }
    inline int gridH() const { return height() / Constants::PIXEL_SIZE; }
    int textWidthCells(const QString& s, int scale) const;
    void drawPixelText(QPainter& p, const QString& s, int gx, int gy, int scale, const QColor& c, bool bold);
    void plotGridPixel(QPainter& p, int gx, int gy, const QColor& c);
    QRect resumeRectPx() const;
};

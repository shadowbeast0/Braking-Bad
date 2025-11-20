// outro.h
#pragma once
#include <QWidget>
#include <QRect>

class QPushButton;
class QPaintEvent;
class QResizeEvent;
class QMouseEvent;

class OutroScreen : public QWidget {
    Q_OBJECT
public:
    explicit OutroScreen(QWidget* parent = nullptr);
    void setStats(int coinCount, int nitroCount, int score, double distanceMeters);
    void setFlips(int flips);

signals:
    void exitRequested();
    void restartRequested();

protected:
    void paintEvent(QPaintEvent* e) override;
    void resizeEvent(QResizeEvent* e) override;
    void mousePressEvent(QMouseEvent* e) override;

private:
    void centerInParent();
    void drawPixelCoin(QPainter& p, int gx, int gy, int rCells);
    void drawPixelFlame(QPainter& p, int gx, int gy, int lenCells);

    int m_flips = 0;

private:
    QPushButton* m_exitBtn = nullptr;
    int  m_cell   = 6;

    int m_coins = 0;
    int m_nitros = 0;
    int m_score = 0;
    double m_distanceM = 0.0;

    QRect m_btnExitRect;
    QRect m_btnRestartRect;
};

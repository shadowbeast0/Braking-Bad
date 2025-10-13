#ifndef MY_LABEL_H
#define MY_LABEL_H

#include <QLabel>
#include <QPoint>

class my_label : public QLabel
{
    Q_OBJECT
public:
    explicit my_label(QWidget *parent = nullptr);
signals:
    void mousePressedAt(QPoint pos, Qt::MouseButton button);
    void mouseMovedAt(QPoint pos, Qt::MouseButtons buttons);
    void mouseReleasedAt(QPoint pos, Qt::MouseButton button);
protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
};

#endif

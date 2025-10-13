#include "my_label.h"
#include <QMouseEvent>

my_label::my_label(QWidget *parent) : QLabel(parent)
{
    setMouseTracking(true);
}

void my_label::mousePressEvent(QMouseEvent *event)
{
    emit mousePressedAt(event->pos(), event->button());
    QLabel::mousePressEvent(event);
}

void my_label::mouseMoveEvent(QMouseEvent *event)
{
    emit mouseMovedAt(event->pos(), event->buttons());
    QLabel::mouseMoveEvent(event);
}

void my_label::mouseReleaseEvent(QMouseEvent *event)
{
    emit mouseReleasedAt(event->pos(), event->button());
    QLabel::mouseReleaseEvent(event);
}

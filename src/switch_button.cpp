#include "switch_button.h"
#include <QEasingCurve>

SwitchButton::SwitchButton(QWidget *parent)
    : QCheckBox(parent)
    , m_width(44)
    , m_height(22)
    , m_activeColor("#00C3FF")
    , m_inactiveColor("#999999")
    , m_circleColor("#FFFFFF")
{
    setText("");
    setFixedSize(m_width, m_height);
    setCursor(Qt::PointingHandCursor);

    m_circlePosition = isChecked() ? (m_width - m_height + 2) : 2;

    m_animation = new QPropertyAnimation(this, "circlePosition", this);
    m_animation->setDuration(150);
    m_animation->setEasingCurve(QEasingCurve::InOutQuad);
}

qreal SwitchButton::circlePosition() const {
    return m_circlePosition;
}

void SwitchButton::setCirclePosition(qreal pos) {
    m_circlePosition = pos;
    update();
}

void SwitchButton::setChecked(bool checked) {
    if (isChecked() == checked) return;
    QCheckBox::setChecked(checked);
    animate(checked);
}

void SwitchButton::nextCheckState() {
    bool nextState = !isChecked();
    QCheckBox::setChecked(nextState);
    animate(nextState);
}

void SwitchButton::animate(bool checked) {
    m_animation->stop();
    qreal endVal = checked ? (m_width - m_height + 2) : 2;
    m_animation->setEndValue(endVal);
    m_animation->start();
}

bool SwitchButton::hitButton(const QPoint &pos) const {
    return rect().contains(pos);
}

QSize SwitchButton::sizeHint() const {
    return QSize(m_width, m_height);
}

void SwitchButton::paintEvent(QPaintEvent *event) {
    Q_UNUSED(event);
    
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(Qt::NoPen);

    QColor currentBg = isChecked() ? m_activeColor : m_inactiveColor;

    painter.setBrush(currentBg);
    painter.drawRoundedRect(0, 0, width(), height(), height() / 2.0, height() / 2.0);

    painter.setBrush(m_circleColor);
    qreal circleDiameter = height() - 4;
    painter.drawEllipse(QRectF(m_circlePosition, 2, circleDiameter, circleDiameter));
    painter.end();
}

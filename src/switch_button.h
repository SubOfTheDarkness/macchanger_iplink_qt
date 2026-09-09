#pragma once

#include <QCheckBox>
#include <QPropertyAnimation>
#include <QPainter>
#include <QColor>

class SwitchButton : public QCheckBox {
    Q_OBJECT
    Q_PROPERTY(qreal circlePosition READ circlePosition WRITE setCirclePosition)

public:
    explicit SwitchButton(QWidget *parent = nullptr);
    
    void setChecked(bool checked);

    qreal circlePosition() const;
    void setCirclePosition(qreal pos);

    QSize sizeHint() const override;

protected:
    void nextCheckState() override;
    bool hitButton(const QPoint &pos) const override;
    void paintEvent(QPaintEvent *event) override;

private:
    void animate(bool checked);

    qreal m_circlePosition;
    QPropertyAnimation *m_animation;

    QColor m_activeColor;
    QColor m_inactiveColor;
    QColor m_circleColor;
    
    int m_width;
    int m_height;
};

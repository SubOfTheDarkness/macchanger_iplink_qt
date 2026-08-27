#pragma once

#include <QWidget>
#include <QVector>
#include <QPainter>
#include <QPainterPath>
#include <QColor>
#include <algorithm> 

class PingGraph : public QWidget {
    Q_OBJECT

public:
    explicit PingGraph(QWidget *parent = nullptr) 
        : QWidget(parent), m_enabled(true) {}

    void addRttPoint(double rtt) {
        m_points.append(rtt);
        if (m_points.size() > 60) {
            m_points.removeFirst();
        }
        update(); 
    }

    void clearGraph() {
        m_points.clear();
        update();
    }

    void setGraphEnabled(bool enabled) {
        m_enabled = enabled;
        update();
    }

protected:
    void paintEvent(QPaintEvent *event) override {
        Q_UNUSED(event);
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);

        painter.fillRect(rect(), QColor("#0c0c0c"));

        if (!m_enabled) return;

        double maxRtt = 10.0;
        
        if (!m_points.isEmpty()) {
            auto it = std::max_element(m_points.begin(), m_points.end());
            if (*it > maxRtt) {
                maxRtt = *it;
            }
        }
        
        maxRtt *= 1.2; 

        painter.setPen(QPen(QColor("#222222"), 1, Qt::DashLine));
        
        int gridLinesCount = 4;
        double rttStep = maxRtt / gridLinesCount;

        for (int i = 1; i < gridLinesCount; ++i) {
            double currentGridMs = i * rttStep;
            int y = height() - (currentGridMs * height() / maxRtt);
            
            if (y > 0 && y < height()) {
                painter.drawLine(0, y, width(), y);
                
                painter.setPen(QColor("#555555"));
                painter.drawText(5, y - 2, QString::number(currentGridMs, 'f', 1) + " ms");
                painter.setPen(QPen(QColor("#222222"), 1, Qt::DashLine));
            }
        }

        if (m_points.isEmpty()) return;

        painter.setPen(QPen(QColor("#00C3FF"), 2, Qt::SolidLine));
        
        double stepX = (m_points.size() > 1) ? (double)width() / (m_points.size() - 1) : (double)width();
        QPainterPath path;

        for (int i = 0; i < m_points.size(); ++i) {
            double rtt = m_points[i];
            
            double x = i * stepX;
            double y = height() - (rtt * height() / maxRtt);

            if (i == 0) {
                path.moveTo(x, y);
            } else {
                path.lineTo(x, y);
            }
        }
        painter.drawPath(path);
    }

private:
    QVector<double> m_points;
    bool m_enabled;
};

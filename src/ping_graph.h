#pragma once

#include <QWidget>
#include <QVector>
#include <QPainter>
#include <QPainterPath>
#include <QColor>
#include <algorithm>
#include <cmath>

class PingGraph : public QWidget {
    Q_OBJECT

public:
    struct PingPoint {
        double rtt;
        bool isLoss;
    };

    explicit PingGraph(QWidget *parent = nullptr) 
        : QWidget(parent), m_enabled(true) {}

    void addRttPoint(double rtt, bool isLoss = false) {
        m_points.append({rtt, isLoss});
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
            double currentMax = 0.0;
            for (const auto& pt : m_points) {
                if (!pt.isLoss && pt.rtt > currentMax) {
                    currentMax = pt.rtt;
                }
            }
            if (currentMax > 0) {
                maxRtt = currentMax;
            }
        }
        
        int gridLinesCount = 6;
        double rawStep = maxRtt / gridLinesCount;
        double magnitude = std::pow(10, std::floor(std::log10(rawStep)));
        double residual = rawStep / magnitude;

        double cleanStep;
        if (residual < 1.5) cleanStep = 1.0 * magnitude;
        else if (residual < 3.0) cleanStep = 2.0 * magnitude;
        else if (residual < 7.0) cleanStep = 5.0 * magnitude;
        else cleanStep = 10.0 * magnitude;

        maxRtt = cleanStep * gridLinesCount;

        painter.setPen(QPen(QColor("#222222"), 1, Qt::DashLine));
        
        for (int i = 1; i <= gridLinesCount; ++i) {
            double currentGridMs = i * cleanStep;
            int y = height() - (currentGridMs * height() / maxRtt);
            
            if (y >= 0 && y < height()) {
                painter.drawLine(0, y, width(), y);
                
                painter.setPen(QColor("#555555"));
                painter.drawText(5, y - 2, QString::number(currentGridMs, 'g', 4) + " ms");
                painter.setPen(QPen(QColor("#222222"), 1, Qt::DashLine));
            }
        }

        if (m_points.isEmpty()) return;

        double stepX = (m_points.size() > 1) ? (double)width() / (m_points.size() - 1) : (double)width();

        for (int i = 0; i < m_points.size() - 1; ++i) {
            double rtt1 = m_points[i].rtt;
            double rtt2 = m_points[i+1].rtt;
            
            double x1 = i * stepX;
            double y1 = height() - (std::min(rtt1, maxRtt) * height() / maxRtt);
            
            double x2 = (i + 1) * stepX;
            double y2 = height() - (std::min(rtt2, maxRtt) * height() / maxRtt);

            if (m_points[i+1].isLoss || m_points[i].isLoss) {
                painter.setPen(QPen(QColor("#ff4f4f"), 2, Qt::SolidLine));
            } else {
                painter.setPen(QPen(QColor("#00C3FF"), 2, Qt::SolidLine));
            }

            painter.drawLine(QPointF(x1, y1), QPointF(x2, y2));
        }

        for (int i = 0; i < m_points.size(); ++i) {
            if (m_points[i].isLoss) {
                double x = i * stepX;
                double y = height() - 1;
                painter.setBrush(QColor("#ff4f4f"));
                painter.setPen(Qt::NoPen);
                painter.drawEllipse(QPointF(x, y), 3, 3);
            }
        }
    }

private:
    QVector<PingPoint> m_points;
    bool m_enabled;
};

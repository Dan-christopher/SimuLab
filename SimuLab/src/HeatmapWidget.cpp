#include "HeatmapWidget.h"

#include "ColorMap.h"

#include <QLinearGradient>
#include <QMouseEvent>
#include <QPainter>
#include <QToolTip>

#include <algorithm>

// ============================== HeatmapWidget ==============================

HeatmapWidget::HeatmapWidget(QWidget *parent)
    : QWidget(parent)
{
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    setMouseTracking(true);          // hover tooltips
    setCursor(Qt::CrossCursor);
}

void HeatmapWidget::setTemperatureRange(double minTemp, double maxTemp)
{
    m_minTemp = minTemp;
    m_maxTemp = (maxTemp > minTemp) ? maxTemp : minTemp + 1.0;
    rebuildImage();
}

void HeatmapWidget::setData(const std::vector<double> &temperatures, int width, int height)
{
    if (width <= 0 || height <= 0 ||
        temperatures.size() != static_cast<std::size_t>(width) * static_cast<std::size_t>(height))
        return;

    m_width  = width;
    m_height = height;
    m_temps  = temperatures;     // reuses capacity when the size is unchanged
    rebuildImage();
}

void HeatmapWidget::setSelectedCell(int x, int y)
{
    m_selX = x;
    m_selY = y;
    update();
}

void HeatmapWidget::setGridLinesEnabled(bool enabled)
{
    m_gridLines = enabled;
    update();
}

void HeatmapWidget::rebuildImage()
{
    if (m_width <= 0 || m_height <= 0)
        return;

    if (m_image.isNull() || m_image.width() != m_width || m_image.height() != m_height)
        m_image = QImage(m_width, m_height, QImage::Format_RGB32);

    const double inv = 1.0 / (m_maxTemp - m_minTemp);
    for (int y = 0; y < m_height; ++y) {
        QRgb *line = reinterpret_cast<QRgb *>(m_image.scanLine(y));
        const double *src = &m_temps[static_cast<std::size_t>(y) * static_cast<std::size_t>(m_width)];
        for (int x = 0; x < m_width; ++x)
            line[x] = ColorMap::rgb((src[x] - m_minTemp) * inv);
    }
    update();
}

QRectF HeatmapWidget::gridRect() const
{
    if (m_width <= 0 || m_height <= 0)
        return {};

    const double margin = 6.0;
    const double availW = std::max(1.0, width()  - 2.0 * margin);
    const double availH = std::max(1.0, height() - 2.0 * margin);
    const double cell   = std::min(availW / m_width, availH / m_height);   // square cells
    const double gw = cell * m_width;
    const double gh = cell * m_height;
    return QRectF((width() - gw) / 2.0, (height() - gh) / 2.0, gw, gh);
}

bool HeatmapWidget::cellAt(const QPointF &pos, int &x, int &y) const
{
    const QRectF r = gridRect();
    if (r.isEmpty() || !r.contains(pos))
        return false;
    const double cell = r.width() / m_width;
    x = std::min(m_width  - 1, static_cast<int>((pos.x() - r.left()) / cell));
    y = std::min(m_height - 1, static_cast<int>((pos.y() - r.top())  / cell));
    return true;
}

void HeatmapWidget::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.fillRect(rect(), QColor(16, 18, 24));

    if (m_image.isNull())
        return;

    const QRectF r = gridRect();
    const double cell = r.width() / m_width;

    p.setRenderHint(QPainter::SmoothPixmapTransform, false);   // crisp cells
    p.drawImage(r, m_image);

    // Cell boundaries - only when cells are big enough to be useful.
    if (m_gridLines && cell >= 7.0) {
        p.setPen(QPen(QColor(0, 0, 0, 70), 1));
        for (int x = 0; x <= m_width; ++x) {
            const double px = r.left() + x * cell;
            p.drawLine(QPointF(px, r.top()), QPointF(px, r.bottom()));
        }
        for (int y = 0; y <= m_height; ++y) {
            const double py = r.top() + y * cell;
            p.drawLine(QPointF(r.left(), py), QPointF(r.right(), py));
        }
    }

    // Selected-cell marker (black outline + white outline stays visible on every colour).
    if (m_selX >= 0 && m_selY >= 0 && m_selX < m_width && m_selY < m_height) {
        const QRectF c(r.left() + m_selX * cell, r.top() + m_selY * cell, cell, cell);
        const QRectF marker = c.adjusted(-1, -1, 1, 1);
        p.setBrush(Qt::NoBrush);
        p.setPen(QPen(Qt::black, 3));
        p.drawRect(marker);
        p.setPen(QPen(Qt::white, 1.5));
        p.drawRect(marker);
    }

    p.setBrush(Qt::NoBrush);
    p.setPen(QPen(QColor(80, 86, 104), 1));
    p.drawRect(r);
}

void HeatmapWidget::mousePressEvent(QMouseEvent *event)
{
    int x = 0, y = 0;
    if (event->button() == Qt::LeftButton && cellAt(event->position(), x, y))
        emit cellSelected(x, y);
}

void HeatmapWidget::mouseMoveEvent(QMouseEvent *event)
{
    int x = 0, y = 0;
    if (!cellAt(event->position(), x, y)) {
        QToolTip::hideText();
        return;
    }

    if (event->buttons() & Qt::LeftButton)
        emit cellSelected(x, y);

    const double t = m_temps[static_cast<std::size_t>(y) * static_cast<std::size_t>(m_width) +
                             static_cast<std::size_t>(x)];
    QToolTip::showText(event->globalPosition().toPoint(),
                       QStringLiteral("Cell (%1, %2): %3 \u00B0C").arg(x).arg(y).arg(t, 0, 'f', 2),
                       this);
}

// ============================ ColorLegendWidget ============================

ColorLegendWidget::ColorLegendWidget(QWidget *parent)
    : QWidget(parent)
{
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
}

void ColorLegendWidget::setTemperatureRange(double minTemp, double maxTemp)
{
    m_minTemp = minTemp;
    m_maxTemp = (maxTemp > minTemp) ? maxTemp : minTemp + 1.0;
    update();
}

void ColorLegendWidget::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::TextAntialiasing);

    const int barLeft = 10, barWidth = 22;
    const int top = 38, bottom = std::max(top + 20, height() - 16);
    const QRect bar(barLeft, top, barWidth, bottom - top);

    p.setPen(QColor(200, 204, 214));
    p.drawText(QRect(0, 4, width(), 22), Qt::AlignHCenter | Qt::AlignVCenter,
               QStringLiteral("Temp (\u00B0C)"));

    // Hot (red) at the top, cold (blue) at the bottom.
    QLinearGradient g(bar.topLeft(), bar.bottomLeft());
    for (int i = 0; i <= 4; ++i)
        g.setColorAt(i / 4.0, QColor(ColorMap::rgb(1.0 - i / 4.0)));
    p.fillRect(bar, g);
    p.setPen(QColor(90, 96, 112));
    p.drawRect(bar);

    p.setPen(QColor(200, 204, 214));
    for (int i = 0; i <= 4; ++i) {
        const double y = top + (bottom - top) * i / 4.0;
        const double value = m_maxTemp - (m_maxTemp - m_minTemp) * i / 4.0;
        p.drawLine(QPointF(barLeft + barWidth, y), QPointF(barLeft + barWidth + 4, y));
        p.drawText(QRectF(barLeft + barWidth + 7, y - 9, width() - barLeft - barWidth - 8, 18),
                   Qt::AlignVCenter | Qt::AlignLeft, QString::number(value, 'f', 1));
    }
}

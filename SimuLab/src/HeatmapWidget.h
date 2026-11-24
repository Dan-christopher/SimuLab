#pragma once

#include <QImage>
#include <QRectF>
#include <QWidget>

#include <vector>

/**
 * HeatmapWidget
 *
 * Renders a 2D temperature field as a colour-coded grid with QPainter.
 * It knows nothing about SimulationEngine: the owner pushes a snapshot of the
 * data with setData() and a colour range with setTemperatureRange().
 *
 * Rendering strategy: each update converts the temperatures into a small
 * QImage (one pixel per grid cell) which is then drawn scaled - without
 * smoothing - into the largest square-cell rectangle that fits the widget.
 * This is visually identical to filling one rectangle per cell, but costs a
 * single drawImage() call, so it stays smooth for large grids and resizes
 * instantly (no recolouring is needed on resize).
 */
class HeatmapWidget : public QWidget
{
    Q_OBJECT

public:
    explicit HeatmapWidget(QWidget *parent = nullptr);

    /// Temperature range mapped onto the colour gradient (deg C).
    void setTemperatureRange(double minTemp, double maxTemp);

    /// Copies a row-major snapshot (size must be width*height) and repaints.
    void setData(const std::vector<double> &temperatures, int width, int height);

    void setSelectedCell(int x, int y);
    void setGridLinesEnabled(bool enabled);

    QSize sizeHint() const override { return {640, 640}; }
    QSize minimumSizeHint() const override { return {240, 240}; }

signals:
    /// Emitted when the user clicks (or drags over) a cell.
    void cellSelected(int x, int y);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;

private:
    QRectF gridRect() const;                       // where the grid is drawn
    bool   cellAt(const QPointF &pos, int &x, int &y) const;
    void   rebuildImage();

    std::vector<double> m_temps;
    int    m_width  = 0;
    int    m_height = 0;
    double m_minTemp = 0.0;
    double m_maxTemp = 1.0;
    int    m_selX = -1;
    int    m_selY = -1;
    bool   m_gridLines = true;
    QImage m_image;
};

/**
 * ColorLegendWidget - vertical gradient bar with temperature labels, drawn with
 * the same ColorMap as the heatmap so the two always agree.
 */
class ColorLegendWidget : public QWidget
{
    Q_OBJECT

public:
    explicit ColorLegendWidget(QWidget *parent = nullptr);

    void setTemperatureRange(double minTemp, double maxTemp);

    QSize sizeHint() const override { return {96, 320}; }
    QSize minimumSizeHint() const override { return {96, 160}; }

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    double m_minTemp = 0.0;
    double m_maxTemp = 1.0;
};

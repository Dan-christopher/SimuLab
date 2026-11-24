#pragma once

#include <QRgb>

#include <array>
#include <cstddef>

/**
 * ColorMap - temperature colour scale shared by the heatmap and the legend.
 *
 * Maps a *normalised* value v in [0,1] (0 = coldest, 1 = hottest of the
 * configured temperature range) onto the gradient
 *
 *      blue (0.00) -> cyan (0.25) -> green (0.50) -> yellow (0.75) -> red (1.00)
 *
 * A 256-entry lookup table is built once, so colouring a cell is a single
 * array access (important when repainting 1000 x 1000 grids).
 */
namespace ColorMap {
namespace detail {

struct Stop { double pos; int r, g, b; };

inline constexpr Stop kStops[5] = {
    {0.00,   0,   0, 255},   // blue   - cold
    {0.25,   0, 255, 255},   // cyan   - moderately cool
    {0.50,   0, 255,   0},   // green  - medium
    {0.75, 255, 255,   0},   // yellow - warm
    {1.00, 255,   0,   0},   // red    - hot
};

inline QRgb interpolate(double v)
{
    for (int i = 0; i < 4; ++i) {
        const Stop &a = kStops[i];
        const Stop &b = kStops[i + 1];
        if (v <= b.pos) {
            const double t = (v - a.pos) / (b.pos - a.pos);
            return qRgb(static_cast<int>(a.r + (b.r - a.r) * t + 0.5),
                        static_cast<int>(a.g + (b.g - a.g) * t + 0.5),
                        static_cast<int>(a.b + (b.b - a.b) * t + 0.5));
        }
    }
    return qRgb(255, 0, 0);
}

inline const std::array<QRgb, 256> &table()
{
    static const std::array<QRgb, 256> t = [] {
        std::array<QRgb, 256> a{};
        for (std::size_t i = 0; i < a.size(); ++i)
            a[i] = interpolate(static_cast<double>(i) / 255.0);
        return a;
    }();
    return t;
}

} // namespace detail

/// Colour for a normalised value; values outside [0,1] (and NaN) are clamped.
inline QRgb rgb(double normalized)
{
    if (!(normalized > 0.0)) normalized = 0.0;   // also catches NaN
    if (normalized > 1.0)    normalized = 1.0;
    return detail::table()[static_cast<std::size_t>(normalized * 255.0 + 0.5)];
}

} // namespace ColorMap

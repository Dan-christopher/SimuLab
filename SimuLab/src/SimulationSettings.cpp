#include "SimulationSettings.h"

#include <algorithm>
#include <cmath>

QString SimulationSettings::validate() const
{
    if (gridWidth < kMinGrid || gridWidth > kMaxGrid ||
        gridHeight < kMinGrid || gridHeight > kMaxGrid) {
        return QStringLiteral("Grid width and height must each be between %1 and %2 cells.")
            .arg(kMinGrid).arg(kMaxGrid);
    }

    const auto tempOk = [](double t) {
        return std::isfinite(t) && t >= kMinTemp && t <= kMaxTemp;
    };
    if (!tempOk(backgroundTemp) || !tempOk(boundaryTemp) || !tempOk(hotTemp)) {
        return QStringLiteral("Temperatures must be between %1 and %2 \u00B0C.")
            .arg(kMinTemp).arg(kMaxTemp);
    }

    if (hotWidth < 1 || hotHeight < 1) {
        return QStringLiteral("The hot region must be at least 1 x 1 cells.");
    }
    if (hotX < 0 || hotY < 0 ||
        hotX + hotWidth > gridWidth || hotY + hotHeight > gridHeight) {
        return QStringLiteral(
                   "The hot region (x=%1, y=%2, %3 x %4 cells) does not fit inside the "
                   "%5 x %6 grid. Use \"Center hot region\" or adjust its position/size.")
            .arg(hotX).arg(hotY).arg(hotWidth).arg(hotHeight)
            .arg(gridWidth).arg(gridHeight);
    }

    if (updateIntervalMs < 1 || updateIntervalMs > kMaxIntervalMs) {
        return QStringLiteral("The update interval must be between 1 and %1 ms.")
            .arg(kMaxIntervalMs);
    }
    if (stepsPerUpdate < 1 || stepsPerUpdate > kMaxSteps) {
        return QStringLiteral("Calculation steps per update must be between 1 and %1.")
            .arg(kMaxSteps);
    }

    // Stability of the explicit 2D scheme (von Neumann analysis): r <= 1/4.
    if (!std::isfinite(diffusionCoefficient) || diffusionCoefficient <= 0.0 ||
        diffusionCoefficient > kMaxStableR) {
        return QStringLiteral(
                   "The diffusion coefficient r must be greater than 0 and at most %1.\n\n"
                   "Larger values make the explicit finite-difference scheme numerically "
                   "unstable: temperatures would oscillate and blow up instead of diffusing.")
            .arg(kMaxStableR);
    }

    return QString();
}

double SimulationSettings::colorRangeMin() const
{
    return std::min({backgroundTemp, boundaryTemp, hotTemp});
}

double SimulationSettings::colorRangeMax() const
{
    const double lo = colorRangeMin();
    const double hi = std::max({backgroundTemp, boundaryTemp, hotTemp});
    // Heat diffusion obeys a maximum principle, so temperatures never leave
    // [lo, hi]. Guard against a zero-width range (all temperatures equal).
    return (hi - lo < 1e-9) ? lo + 1.0 : hi;
}

void SimulationSettings::centerHotRegion()
{
    hotWidth  = std::max(1, std::min(hotWidth,  gridWidth));
    hotHeight = std::max(1, std::min(hotHeight, gridHeight));
    hotX = (gridWidth  - hotWidth)  / 2;
    hotY = (gridHeight - hotHeight) / 2;
}

bool SimulationSettings::sameInitialConditions(const SimulationSettings &o) const
{
    return gridWidth == o.gridWidth && gridHeight == o.gridHeight &&
           backgroundTemp == o.backgroundTemp && boundaryTemp == o.boundaryTemp &&
           hotTemp == o.hotTemp && hotX == o.hotX && hotY == o.hotY &&
           hotWidth == o.hotWidth && hotHeight == o.hotHeight;
}

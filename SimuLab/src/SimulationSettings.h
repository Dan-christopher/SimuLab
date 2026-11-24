#pragma once

#include <QString>

/**
 * SimulationSettings
 *
 * Plain value type that holds every user-configurable parameter of a
 * simulation run. It contains no GUI code and no numerical code - it is only
 * data plus validation, so it can be unit-tested on its own and later be
 * serialised (import/export of configurations).
 *
 * Grid cells are addressed as (x, y) with x = column (0 .. gridWidth-1) and
 * y = row (0 .. gridHeight-1); (0,0) is the top-left cell.
 */
struct SimulationSettings
{
    // ---- limits used for validation (also used by the GUI spin boxes) ----
    static constexpr int    kMinGrid       = 5;
    static constexpr int    kMaxGrid       = 1000;
    static constexpr double kMinTemp       = -273.15;   // absolute zero (deg C)
    static constexpr double kMaxTemp       = 10000.0;
    static constexpr double kMaxStableR    = 0.25;      // 2D explicit stability limit
    static constexpr int    kMaxIntervalMs = 5000;
    static constexpr int    kMaxSteps      = 10000;

    // ---- grid ----
    int gridWidth  = 100;
    int gridHeight = 100;

    // ---- initial / boundary temperatures (deg C) ----
    double backgroundTemp = 20.0;   // initial temperature of every interior cell
    double boundaryTemp   = 20.0;   // fixed (Dirichlet) temperature of the outer ring
    double hotTemp        = 100.0;  // initial temperature inside the hot region

    // ---- hot region: top-left cell + size, all in cells ----
    int hotX      = 40;
    int hotY      = 40;
    int hotWidth  = 20;
    int hotHeight = 20;

    // ---- run-time behaviour ----
    int    updateIntervalMs     = 30;    // QTimer interval  ("simulation speed")
    int    stepsPerUpdate       = 5;     // numerical steps computed per timer tick
    double diffusionCoefficient = 0.20;  // r = alpha * dt / dx^2

    /// Returns an empty string if the settings are valid, otherwise a
    /// human-readable description of the first problem found.
    QString validate() const;

    /// Lowest temperature that can occur (used as the bottom of the colour scale).
    double colorRangeMin() const;
    /// Highest temperature that can occur (top of the colour scale).
    double colorRangeMax() const;

    /// Moves the hot region to the centre of the grid (shrinks it if it does not fit).
    void centerHotRegion();

    /// True if both objects describe the same *initial state* (grid, temperatures,
    /// hot region). Run-time parameters (speed, steps, r) are ignored.
    bool sameInitialConditions(const SimulationSettings &other) const;
};

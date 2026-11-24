#pragma once

#include "SimulationSettings.h"

#include <cstddef>
#include <vector>

/// Summary statistics of the current temperature field (deg C).
struct TemperatureStats
{
    double min     = 0.0;
    double max     = 0.0;
    double average = 0.0;
};

/**
 * SimulationEngine
 *
 * Pure numerical core: solves the 2D heat equation
 *
 *      dT/dt = alpha * (d2T/dx2 + d2T/dy2)
 *
 * with the explicit Finite Difference Method (FTCS scheme) on a uniform grid:
 *
 *      Tnew(i,j) = T(i,j) + r * [T(i+1,j) + T(i-1,j) + T(i,j+1) + T(i,j-1) - 4 T(i,j)]
 *
 * where r = alpha * dt / dx^2 must satisfy r <= 0.25 for stability.
 *
 * Design notes
 *  - No GUI code and no dependency on QWidget: it can be unit-tested headless
 *    and moved into a worker thread (QThread) unchanged.
 *  - Two buffers (current / next) are allocated once in initialize(); a step
 *    only reads from `current`, writes into `next`, then swaps them in O(1).
 *    Updated cells therefore never influence their neighbours inside the same
 *    iteration, and no memory is allocated while the simulation runs.
 *  - The grid is stored row-major in one contiguous vector: index = y*width + x.
 *  - The outer ring of cells is a fixed-temperature (Dirichlet) boundary. It is
 *    never written by step(), so it keeps its initial value in both buffers.
 */
class SimulationEngine
{
public:
    SimulationEngine() = default;

    /// Builds the initial temperature field. Returns false (and changes nothing)
    /// if `settings.validate()` reports a problem.
    bool initialize(const SimulationSettings &settings);

    /// Restores the initial temperature distribution of the last initialize().
    void reset();

    /// One explicit time step.
    void step();

    /// `steps` explicit time steps (values <= 0 are ignored).
    void advance(int steps);

    /// Changes r while running. Returns false if r is outside (0, 0.25].
    bool setDiffusionCoefficient(double r);
    double diffusionCoefficient() const { return m_r; }

    int       width() const     { return m_width; }
    int       height() const    { return m_height; }
    long long stepCount() const { return m_stepCount; }
    bool      isInitialized() const { return !m_current.empty(); }

    bool   contains(int x, int y) const;
    /// Temperature of cell (x, y); quiet NaN if the cell is outside the grid.
    double temperatureAt(int x, int y) const;

    /// Row-major temperature field (size width*height).
    const std::vector<double> &temperatures() const { return m_current; }

    /// Minimum / maximum / average over the whole grid (single pass, O(N)).
    TemperatureStats statistics() const;

private:
    SimulationSettings  m_settings;      // initial configuration (used by reset())
    int                 m_width  = 0;
    int                 m_height = 0;
    double              m_r      = 0.0;
    long long           m_stepCount = 0;
    std::vector<double> m_current;       // T at time level n
    std::vector<double> m_next;          // T at time level n+1 (scratch)
};

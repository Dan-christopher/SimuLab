#include "SimulationEngine.h"

#include <algorithm>
#include <limits>
#include <utility>

bool SimulationEngine::initialize(const SimulationSettings &s)
{
    if (!s.validate().isEmpty())
        return false;

    m_settings  = s;
    m_width     = s.gridWidth;
    m_height    = s.gridHeight;
    m_r         = s.diffusionCoefficient;
    m_stepCount = 0;

    const std::size_t w = static_cast<std::size_t>(m_width);
    const std::size_t h = static_cast<std::size_t>(m_height);

    // 1) background temperature everywhere
    m_current.assign(w * h, s.backgroundTemp);

    // 2) rectangular hot region
    for (int y = s.hotY; y < s.hotY + s.hotHeight; ++y) {
        double *row = &m_current[static_cast<std::size_t>(y) * w];
        for (int x = s.hotX; x < s.hotX + s.hotWidth; ++x)
            row[x] = s.hotTemp;
    }

    // 3) fixed-temperature boundary ring (overrides the hot region if it touches the edge)
    for (std::size_t x = 0; x < w; ++x) {
        m_current[x]                = s.boundaryTemp;   // top row
        m_current[(h - 1) * w + x]  = s.boundaryTemp;   // bottom row
    }
    for (std::size_t y = 0; y < h; ++y) {
        m_current[y * w]            = s.boundaryTemp;   // left column
        m_current[y * w + (w - 1)]  = s.boundaryTemp;   // right column
    }

    // The scratch buffer starts as a copy so its boundary cells are already correct.
    m_next = m_current;
    return true;
}

void SimulationEngine::reset()
{
    SimulationSettings s = m_settings;
    s.diffusionCoefficient = m_r;          // keep a live-adjusted r
    initialize(s);
}

void SimulationEngine::step()
{
    if (m_current.empty())
        return;

    const int    w   = m_width;
    const int    h   = m_height;
    const double r   = m_r;
    const double *c  = m_current.data();
    double       *n  = m_next.data();

    // Interior cells only; boundary cells keep their fixed value in both buffers.
    for (int y = 1; y < h - 1; ++y) {
        const std::size_t row = static_cast<std::size_t>(y) * static_cast<std::size_t>(w);
        for (int x = 1; x < w - 1; ++x) {
            const std::size_t i = row + static_cast<std::size_t>(x);
            n[i] = c[i] + r * (c[i + 1] + c[i - 1] + c[i + w] + c[i - w] - 4.0 * c[i]);
        }
    }

    m_current.swap(m_next);   // O(1): "next" becomes "current"
    ++m_stepCount;
}

void SimulationEngine::advance(int steps)
{
    for (int i = 0; i < steps; ++i)
        step();
}

bool SimulationEngine::setDiffusionCoefficient(double r)
{
    if (!(r > 0.0) || r > SimulationSettings::kMaxStableR)
        return false;
    m_r = r;
    return true;
}

bool SimulationEngine::contains(int x, int y) const
{
    return x >= 0 && y >= 0 && x < m_width && y < m_height;
}

double SimulationEngine::temperatureAt(int x, int y) const
{
    if (!contains(x, y))
        return std::numeric_limits<double>::quiet_NaN();
    return m_current[static_cast<std::size_t>(y) * static_cast<std::size_t>(m_width) +
                     static_cast<std::size_t>(x)];
}

TemperatureStats SimulationEngine::statistics() const
{
    TemperatureStats st;
    if (m_current.empty())
        return st;

    double lo  = m_current[0];
    double hi  = m_current[0];
    double sum = 0.0;
    for (double t : m_current) {
        lo   = std::min(lo, t);
        hi   = std::max(hi, t);
        sum += t;
    }
    st.min     = lo;
    st.max     = hi;
    st.average = sum / static_cast<double>(m_current.size());
    return st;
}

# SimuLab - 2D Heat Diffusion Simulation (C++17 / Qt 6)

SimuLab is a **numerical** simulation: every temperature shown on screen is computed by
the heat-diffusion model below. Nothing is measured from sensors and nothing is random.

## 1. Project layout

```
SimuLab/
├── CMakeLists.txt
├── README.md
└── src/
    ├── main.cpp                 application start-up + dark theme
    ├── MainWindow.h/.cpp        GUI composition, signals/slots, QTimer loop
    ├── SimulationEngine.h/.cpp  finite-difference solver (no GUI code)
    ├── SimulationSettings.h/.cpp  parameters + validation (plain data)
    ├── HeatmapWidget.h/.cpp     QPainter heatmap + colour legend widget
    ├── ColorMap.h               shared blue→cyan→green→yellow→red gradient
    └── CsvExporter.h/.cpp       CSV writer (QFile + QTextStream)
```

## 2. Build & run in Qt Creator (Windows, MinGW 64-bit)

1. Install Qt 6 with the **MinGW 64-bit** component (Qt Online Installer) - the kit
   "Desktop Qt 6.x.x MinGW 64-bit" must exist in Qt Creator.
2. `File → Open File or Project…` → select `SimuLab/CMakeLists.txt`.
3. In *Configure Project* tick the **MinGW 64-bit** kit → *Configure Project*.
4. Press **Ctrl+R** (Run). The first build takes ~1 minute.

Command line alternative:

```
cmake -S . -B build -G "MinGW Makefiles" -DCMAKE_PREFIX_PATH=C:/Qt/6.x.x/mingw_64
cmake --build build
build\SimuLab.exe        (add C:\Qt\6.x.x\mingw_64\bin to PATH so the Qt DLLs are found)
```

Requirements: CMake ≥ 3.16, Qt 6 (Widgets), a C++17 compiler. (Verified to build and run on Qt 6.4 / GCC.)

## 3. Using the application

| Control | Action |
|---|---|
| **Start Simulation** | validates all parameters, starts the QTimer; each tick computes *Steps per update* numerical steps and refreshes the heatmap + statistics |
| **Pause Simulation** | stops the timer, keeps the grid |
| **Step Forward** | exactly one numerical step |
| **Reset Simulation** | validates and re-applies the configuration, restoring the initial distribution |
| **Export CSV** | choose a file with a save dialog; writes the current grid |
| Click / drag on heatmap | selects a cell (shown in *Selected cell*); hover shows a tooltip |

Configuration notes:
* Grid size, temperatures and hot-region geometry define the *initial state* → applied by **Reset**
  (or automatically by Start/Step while the step counter is still 0). A yellow hint appears while changes are pending.
* *Update interval*, *Steps per update* and *r* apply live.
* Hot region = top-left cell (X, Y) + width/height in cells. "Center hot region" re-centres it.
* The colour scale is fixed to [lowest, highest] of background / boundary / hot temperature, so colours stay comparable over time.

## 4. Mathematical model

Heat equation in 2D:

    ∂T/∂t = α (∂²T/∂x² + ∂²T/∂y²)

Discretise on a uniform grid (spacing Δx = Δy, time step Δt) with the explicit
forward-time / centred-space (FTCS) finite-difference scheme:

* ∂T/∂t   ≈ (Tⁿ⁺¹(i,j) − Tⁿ(i,j)) / Δt
* ∂²T/∂x² ≈ (T(i+1,j) − 2T(i,j) + T(i−1,j)) / Δx²   (same for y)

Solving for Tⁿ⁺¹ gives the update used in `SimulationEngine::step()`:

    Tnew(i,j) = T(i,j) + r · [T(i+1,j) + T(i−1,j) + T(i,j+1) + T(i,j−1) − 4·T(i,j)]
    r = α · Δt / Δx²

**Stability.** A von Neumann analysis of FTCS in 2D requires r ≤ ¼ (Δx = Δy). For larger r the
high-frequency error modes are amplified every step and the solution explodes with oscillations.
`SimulationSettings::validate()` therefore rejects r ≤ 0 or r > 0.25, and the engine refuses to
accept such a value at run-time.

**Meaning of r.** The GUI works in dimensionless grid units: choosing r fixes Δt = r·Δx²/α, i.e.
simulated time after n steps is t = n·Δt. Larger r (≤ 0.25) = faster diffusion per step.

**Boundary conditions.** The outer ring of cells is a fixed-temperature (Dirichlet) boundary at the
configured boundary temperature (default 20 °C). Only interior cells are updated.

**Two buffers.** The update reads only from `current` and writes only into `next`; the buffers are
then swapped (O(1)). Without this, cells already updated in the same sweep would feed their new
values to neighbours and the scheme would no longer be the one derived above.

**Sanity properties** (checked while developing): temperatures never leave [min(initial), max(initial)]
(maximum principle); a symmetric start stays symmetric; the average falls slowly because heat leaks
through the cold boundary.

## 5. Class responsibilities

| Class | Responsibility | Depends on |
|---|---|---|
| `SimulationSettings` | Holds all parameters; validates them (grid limits, temperature limits, hot region fits, r ≤ 0.25); colour range; "same initial conditions" check | QString only |
| `SimulationEngine` | Owns the two temperature buffers; initialises grid/hot region/boundary; `step()`, `advance(n)`, `reset()`; min/max/average; cell lookup. **No GUI code** | SimulationSettings |
| `HeatmapWidget` | Converts temperatures to colours, draws the scaled grid with optional cell boundaries and a selection marker, emits `cellSelected(x,y)`, tooltips. Knows nothing about the engine | ColorMap |
| `ColorLegendWidget` | Vertical gradient bar with temperature ticks (same ColorMap as the heatmap) | ColorMap |
| `ColorMap` | blue→cyan→green→yellow→red lookup table | - |
| `CsvExporter` | Writes the grid as CSV with column headers (x) and row identifiers (y); reports file errors | QFile, QTextStream |
| `MainWindow` | Builds the dashboard; connects signals/slots; owns the QTimer loop; validates before running; export dialog | all of the above |

CSV format:

```
y\x,0,1,2,...
0,20.0000,20.0000,...
1,20.0000,...
```

## 6. Performance & reliability decisions

* Buffers allocated once in `initialize()`; `step()` allocates nothing and swaps buffers.
* Row-major contiguous storage + raw pointers in the inner loop → cache-friendly traversal.
* Heatmap = one `QImage` (1 pixel per cell) drawn scaled with nearest-neighbour: one draw call, no
  per-cell `drawRect`, instant resizing. Cells stay square and centred.
* Statistics are computed once per GUI update (one pass), not per numerical step.
* Every parameter is validated before running; file errors are reported in a message box.

## 7. Moving the solver into a `QThread` (design hook)

`SimulationEngine` is a plain C++ class with no widgets, so threading needs no engine changes:

1. Create `SimulationWorker : QObject` that owns a `SimulationEngine` and has slots
   `advance(int)` / `reset()`, and a signal `frameReady(std::vector<double>, TemperatureStats, long long)`.
2. `worker->moveToThread(&thread)`; `MainWindow` connects the QTimer to `advance` (queued connection).
3. `frameReady` delivers a *copy* of the grid to the GUI thread → `HeatmapWidget::setData()`.
   `HeatmapWidget` already works on a copied snapshot, so it is thread-safe by construction.

## 8. Extension points

* Live graphs / history: append `engine.statistics()` per tick (e.g. `std::deque<TemperatureStats>`).
* Multiple heat sources: replace the single rectangle in `SimulationEngine::initialize()` by a list of regions.
* Other boundary conditions: change how boundary cells are handled in `step()` (Neumann = copy neighbour).
* Material properties: make `r` a per-cell array (`r(i,j) = α(i,j)·Δt/Δx²`); keep r ≤ 0.25 everywhere.
* Config import/export and snapshots: `SimulationSettings` is plain data - serialise it (JSON/INI); CSV export already exists.

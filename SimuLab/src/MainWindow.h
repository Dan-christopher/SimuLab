#pragma once

#include "SimulationEngine.h"
#include "SimulationSettings.h"

#include <QMainWindow>
#include <QTimer>

class HeatmapWidget;
class ColorLegendWidget;
class QCheckBox;
class QDoubleSpinBox;
class QLabel;
class QPushButton;
class QSpinBox;

/**
 * MainWindow - the controller / view-composition layer.
 *
 * Owns the SimulationEngine, the HeatmapWidget and all control widgets, and
 * wires them together with signals & slots. It is the only class that knows
 * about both the numerical engine and the GUI.
 *
 * Configuration semantics
 *  - Structural parameters (grid size, temperatures, hot region) define the
 *    initial state and are applied by "Reset" (or automatically by "Start" /
 *    "Step" while the simulation is still at step 0).
 *  - Run-time parameters (update interval, steps per update, r) take effect
 *    immediately, even while the simulation is running.
 */
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);

private slots:
    void onStart();
    void onPause();
    void onReset();
    void onStep();
    void onExport();
    void onTick();                       // QTimer timeout
    void onCellSelected(int x, int y);
    void onConfigEdited();
    void onCenterHotRegion();

private:
    // ---- UI construction ----
    void     buildUi();
    QWidget *buildControlsGroup();
    QWidget *buildConfigGroup();
    QWidget *buildStatsGroup();
    QDoubleSpinBox *makeTempSpin(double value);
    QSpinBox       *makeIntSpin(int min, int max, int value, const QString &suffix = QString());

    // ---- helpers ----
    SimulationSettings collectSettings() const;
    void loadSettingsIntoUi(const SimulationSettings &s);
    bool validateOrWarn(const SimulationSettings &s);
    void applyStructure(const SimulationSettings &s);   // (re)initialise engine + views
    bool prepareRun();                                   // validate + apply before start/step
    void setRunning(bool running);
    void refreshView();
    void updateStatistics();

    // ---- model ----
    SimulationEngine   m_engine;
    SimulationSettings m_applied;        // settings the engine was initialised with
    QTimer             m_timer;
    bool               m_running = false;
    int                m_selX = -1;   // -1 => centred on first applyStructure()
    int                m_selY = -1;

    // ---- views / widgets ----
    HeatmapWidget     *m_heatmap = nullptr;
    ColorLegendWidget *m_legend  = nullptr;

    QPushButton *m_startBtn  = nullptr;
    QPushButton *m_pauseBtn  = nullptr;
    QPushButton *m_resetBtn  = nullptr;
    QPushButton *m_stepBtn   = nullptr;
    QPushButton *m_exportBtn = nullptr;

    QSpinBox       *m_gridW = nullptr;
    QSpinBox       *m_gridH = nullptr;
    QDoubleSpinBox *m_bgTemp = nullptr;
    QDoubleSpinBox *m_boundaryTemp = nullptr;
    QDoubleSpinBox *m_hotTemp = nullptr;
    QSpinBox       *m_hotX = nullptr;
    QSpinBox       *m_hotY = nullptr;
    QSpinBox       *m_hotW = nullptr;
    QSpinBox       *m_hotH = nullptr;
    QSpinBox       *m_interval = nullptr;
    QSpinBox       *m_steps = nullptr;
    QDoubleSpinBox *m_coeff = nullptr;
    QCheckBox      *m_gridLines = nullptr;
    QLabel         *m_stabilityLabel = nullptr;
    QLabel         *m_pendingLabel = nullptr;

    QLabel *m_stepLabel = nullptr;
    QLabel *m_maxLabel  = nullptr;
    QLabel *m_minLabel  = nullptr;
    QLabel *m_avgLabel  = nullptr;
    QLabel *m_cellLabel = nullptr;
    QLabel *m_stateLabel = nullptr;
};

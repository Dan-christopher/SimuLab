#include "MainWindow.h"

#include "CsvExporter.h"
#include "HeatmapWidget.h"

#include <QCheckBox>
#include <QDir>
#include <QDoubleSpinBox>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QScrollArea>
#include <QSpinBox>
#include <QStandardPaths>
#include <QStatusBar>
#include <QVBoxLayout>

#include <algorithm>

namespace {
const QString kDegC = QStringLiteral(" \u00B0C");

QString fmtTemp(double t)
{
    return QString::number(t, 'f', 3) + kDegC;
}
} // namespace

// ------------------------------------------------------------------ ctor

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setWindowTitle(tr("SimuLab - Heat Diffusion Simulation"));
    resize(1280, 800);

    buildUi();

    // Default configuration: 100x100 grid, 20x20 hot square in the middle.
    SimulationSettings defaults;
    defaults.centerHotRegion();
    loadSettingsIntoUi(defaults);
    applyStructure(defaults);

    // Signals are connected *after* the initial values are loaded.
    const auto edited = [this] { onConfigEdited(); };
    for (QSpinBox *sb : {m_gridW, m_gridH, m_hotX, m_hotY, m_hotW, m_hotH, m_interval, m_steps})
        connect(sb, &QSpinBox::valueChanged, this, edited);
    for (QDoubleSpinBox *sb : {m_bgTemp, m_boundaryTemp, m_hotTemp, m_coeff})
        connect(sb, &QDoubleSpinBox::valueChanged, this, edited);

    connect(&m_timer, &QTimer::timeout, this, &MainWindow::onTick);
    connect(m_heatmap, &HeatmapWidget::cellSelected, this, &MainWindow::onCellSelected);
    connect(m_gridLines, &QCheckBox::toggled, m_heatmap, &HeatmapWidget::setGridLinesEnabled);

    connect(m_startBtn,  &QPushButton::clicked, this, &MainWindow::onStart);
    connect(m_pauseBtn,  &QPushButton::clicked, this, &MainWindow::onPause);
    connect(m_resetBtn,  &QPushButton::clicked, this, &MainWindow::onReset);
    connect(m_stepBtn,   &QPushButton::clicked, this, &MainWindow::onStep);
    connect(m_exportBtn, &QPushButton::clicked, this, &MainWindow::onExport);

    m_timer.setInterval(defaults.updateIntervalMs);
    onConfigEdited();
    setRunning(false);
    statusBar()->showMessage(tr("Numerical simulation: every value is computed by the heat-diffusion model, not measured."));
}

// ------------------------------------------------------------ UI building

QDoubleSpinBox *MainWindow::makeTempSpin(double value)
{
    auto *sb = new QDoubleSpinBox;
    sb->setRange(SimulationSettings::kMinTemp, SimulationSettings::kMaxTemp);
    sb->setDecimals(1);
    sb->setSingleStep(5.0);
    sb->setSuffix(kDegC);
    sb->setValue(value);
    return sb;
}

QSpinBox *MainWindow::makeIntSpin(int min, int max, int value, const QString &suffix)
{
    auto *sb = new QSpinBox;
    sb->setRange(min, max);
    sb->setValue(value);
    if (!suffix.isEmpty())
        sb->setSuffix(suffix);
    return sb;
}

QWidget *MainWindow::buildControlsGroup()
{
    auto *box = new QGroupBox(tr("Simulation Controls"));
    auto *grid = new QGridLayout(box);

    m_startBtn  = new QPushButton(tr("Start Simulation"));
    m_pauseBtn  = new QPushButton(tr("Pause Simulation"));
    m_resetBtn  = new QPushButton(tr("Reset Simulation"));
    m_stepBtn   = new QPushButton(tr("Step Forward"));
    m_exportBtn = new QPushButton(tr("Export CSV"));
    m_startBtn->setObjectName(QStringLiteral("startButton"));

    grid->addWidget(m_startBtn, 0, 0);
    grid->addWidget(m_pauseBtn, 0, 1);
    grid->addWidget(m_stepBtn,  1, 0);
    grid->addWidget(m_resetBtn, 1, 1);
    grid->addWidget(m_exportBtn, 2, 0, 1, 2);

    m_stateLabel = new QLabel;
    m_stateLabel->setAlignment(Qt::AlignCenter);
    grid->addWidget(m_stateLabel, 3, 0, 1, 2);
    return box;
}

QWidget *MainWindow::buildConfigGroup()
{
    auto *box  = new QGroupBox(tr("Configuration"));
    auto *form = new QFormLayout(box);

    m_gridW = makeIntSpin(SimulationSettings::kMinGrid, SimulationSettings::kMaxGrid, 100, tr(" cells"));
    m_gridH = makeIntSpin(SimulationSettings::kMinGrid, SimulationSettings::kMaxGrid, 100, tr(" cells"));
    m_bgTemp       = makeTempSpin(20.0);
    m_boundaryTemp = makeTempSpin(20.0);
    m_hotTemp      = makeTempSpin(100.0);
    m_hotX = makeIntSpin(0, SimulationSettings::kMaxGrid, 40);
    m_hotY = makeIntSpin(0, SimulationSettings::kMaxGrid, 40);
    m_hotW = makeIntSpin(1, SimulationSettings::kMaxGrid, 20, tr(" cells"));
    m_hotH = makeIntSpin(1, SimulationSettings::kMaxGrid, 20, tr(" cells"));
    m_interval = makeIntSpin(1, SimulationSettings::kMaxIntervalMs, 30, tr(" ms"));
    m_steps    = makeIntSpin(1, SimulationSettings::kMaxSteps, 5);

    m_coeff = new QDoubleSpinBox;
    m_coeff->setRange(0.0001, 1.0);
    m_coeff->setDecimals(4);
    m_coeff->setSingleStep(0.01);
    m_coeff->setValue(0.20);

    m_interval->setToolTip(tr("Time between GUI updates. Lower = faster simulation."));
    m_steps->setToolTip(tr("Numerical time steps computed on every timer tick."));
    m_coeff->setToolTip(tr("r = alpha * dt / dx^2. The explicit scheme is stable only for r <= 0.25."));
    m_boundaryTemp->setToolTip(tr("Fixed temperature of the outer ring of cells (Dirichlet boundary)."));

    form->addRow(tr("Grid width"), m_gridW);
    form->addRow(tr("Grid height"), m_gridH);
    form->addRow(tr("Background temp"), m_bgTemp);
    form->addRow(tr("Boundary temp"), m_boundaryTemp);
    form->addRow(tr("Hot-region temp"), m_hotTemp);
    form->addRow(tr("Hot region X"), m_hotX);
    form->addRow(tr("Hot region Y"), m_hotY);
    form->addRow(tr("Hot region width"), m_hotW);
    form->addRow(tr("Hot region height"), m_hotH);

    auto *center = new QPushButton(tr("Center hot region"));
    connect(center, &QPushButton::clicked, this, &MainWindow::onCenterHotRegion);
    form->addRow(center);

    form->addRow(tr("Update interval"), m_interval);
    form->addRow(tr("Steps per update"), m_steps);
    form->addRow(tr("Diffusion coeff. r"), m_coeff);

    m_stabilityLabel = new QLabel;
    form->addRow(m_stabilityLabel);

    m_gridLines = new QCheckBox(tr("Show cell boundaries"));
    m_gridLines->setChecked(true);
    form->addRow(m_gridLines);

    m_pendingLabel = new QLabel(tr("Grid / temperature / region changes are applied when you press Reset."));
    m_pendingLabel->setWordWrap(true);
    m_pendingLabel->setStyleSheet(QStringLiteral("color:#ffcc66;"));
    m_pendingLabel->hide();
    form->addRow(m_pendingLabel);
    return box;
}

QWidget *MainWindow::buildStatsGroup()
{
    auto *box  = new QGroupBox(tr("Temperature Information"));
    auto *form = new QFormLayout(box);

    const auto makeValue = [] {
        auto *l = new QLabel(QStringLiteral("-"));
        l->setStyleSheet(QStringLiteral("font-weight:600; color:#e8ecf5;"));
        l->setTextInteractionFlags(Qt::TextSelectableByMouse);
        return l;
    };
    m_stepLabel = makeValue();
    m_maxLabel  = makeValue();
    m_minLabel  = makeValue();
    m_avgLabel  = makeValue();
    m_cellLabel = makeValue();
    m_cellLabel->setWordWrap(true);

    form->addRow(tr("Simulation step"), m_stepLabel);
    form->addRow(tr("Maximum"), m_maxLabel);
    form->addRow(tr("Minimum"), m_minLabel);
    form->addRow(tr("Average"), m_avgLabel);
    form->addRow(tr("Selected cell"), m_cellLabel);

    auto *hint = new QLabel(tr("Click the heatmap to select a cell; hover to see its temperature."));
    hint->setWordWrap(true);
    hint->setStyleSheet(QStringLiteral("color:#8a91a5;"));
    form->addRow(hint);
    return box;
}

void MainWindow::buildUi()
{
    auto *central = new QWidget;
    auto *root    = new QHBoxLayout(central);
    root->setContentsMargins(10, 10, 10, 10);
    root->setSpacing(10);

    m_heatmap = new HeatmapWidget;
    m_legend  = new ColorLegendWidget;

    auto *panel = new QWidget;
    auto *pv    = new QVBoxLayout(panel);
    pv->addWidget(buildControlsGroup());
    pv->addWidget(buildStatsGroup());
    pv->addWidget(buildConfigGroup());
    pv->addStretch(1);

    auto *scroll = new QScrollArea;
    scroll->setWidget(panel);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scroll->setFixedWidth(350);

    root->addWidget(m_heatmap, 1);
    root->addWidget(m_legend);
    root->addWidget(scroll);
    setCentralWidget(central);
}

// ------------------------------------------------------- settings helpers

SimulationSettings MainWindow::collectSettings() const
{
    SimulationSettings s;
    s.gridWidth      = m_gridW->value();
    s.gridHeight     = m_gridH->value();
    s.backgroundTemp = m_bgTemp->value();
    s.boundaryTemp   = m_boundaryTemp->value();
    s.hotTemp        = m_hotTemp->value();
    s.hotX           = m_hotX->value();
    s.hotY           = m_hotY->value();
    s.hotWidth       = m_hotW->value();
    s.hotHeight      = m_hotH->value();
    s.updateIntervalMs     = m_interval->value();
    s.stepsPerUpdate       = m_steps->value();
    s.diffusionCoefficient = m_coeff->value();
    return s;
}

void MainWindow::loadSettingsIntoUi(const SimulationSettings &s)
{
    m_gridW->setValue(s.gridWidth);
    m_gridH->setValue(s.gridHeight);
    m_bgTemp->setValue(s.backgroundTemp);
    m_boundaryTemp->setValue(s.boundaryTemp);
    m_hotTemp->setValue(s.hotTemp);
    m_hotX->setValue(s.hotX);
    m_hotY->setValue(s.hotY);
    m_hotW->setValue(s.hotWidth);
    m_hotH->setValue(s.hotHeight);
    m_interval->setValue(s.updateIntervalMs);
    m_steps->setValue(s.stepsPerUpdate);
    m_coeff->setValue(s.diffusionCoefficient);
}

bool MainWindow::validateOrWarn(const SimulationSettings &s)
{
    const QString error = s.validate();
    if (error.isEmpty())
        return true;
    QMessageBox::warning(this, tr("Invalid parameters"), error);
    return false;
}

void MainWindow::applyStructure(const SimulationSettings &s)
{
    if (!m_engine.initialize(s))
        return;
    m_applied = s;

    m_heatmap->setTemperatureRange(s.colorRangeMin(), s.colorRangeMax());
    m_legend->setTemperatureRange(s.colorRangeMin(), s.colorRangeMax());

    // Keep the selected cell valid (default: grid centre) after a grid-size change.
    if (!m_engine.contains(m_selX, m_selY)) {
        m_selX = m_engine.width() / 2;
        m_selY = m_engine.height() / 2;
    }
    m_heatmap->setSelectedCell(m_selX, m_selY);
    refreshView();
    m_pendingLabel->hide();
}

bool MainWindow::prepareRun()
{
    const SimulationSettings s = collectSettings();
    if (!validateOrWarn(s))
        return false;

    // Untouched simulation: it is safe (and expected) to pick up edited initial conditions.
    if (m_engine.stepCount() == 0 && !s.sameInitialConditions(m_applied))
        applyStructure(s);

    m_engine.setDiffusionCoefficient(s.diffusionCoefficient);
    m_timer.setInterval(s.updateIntervalMs);
    return true;
}

void MainWindow::setRunning(bool running)
{
    m_running = running;
    if (running)
        m_timer.start();
    else
        m_timer.stop();

    m_startBtn->setEnabled(!running);
    m_stepBtn->setEnabled(!running);
    m_pauseBtn->setEnabled(running);

    const QString state = running ? tr("Running")
                          : (m_engine.stepCount() == 0 ? tr("Ready") : tr("Paused"));
    const QString color = running ? QStringLiteral("#6fdc8c") : QStringLiteral("#ffcc66");
    m_stateLabel->setText(state);
    m_stateLabel->setStyleSheet(QStringLiteral("font-weight:700; color:%1;").arg(color));
}

void MainWindow::refreshView()
{
    m_heatmap->setData(m_engine.temperatures(), m_engine.width(), m_engine.height());
    updateStatistics();
}

void MainWindow::updateStatistics()
{
    const TemperatureStats st = m_engine.statistics();
    m_stepLabel->setText(QString::number(m_engine.stepCount()));
    m_maxLabel->setText(fmtTemp(st.max));
    m_minLabel->setText(fmtTemp(st.min));
    m_avgLabel->setText(fmtTemp(st.average));

    if (m_engine.contains(m_selX, m_selY))
        m_cellLabel->setText(QStringLiteral("(%1, %2): %3")
                                 .arg(m_selX).arg(m_selY).arg(fmtTemp(m_engine.temperatureAt(m_selX, m_selY))));
    else
        m_cellLabel->setText(QStringLiteral("-"));
}

// ------------------------------------------------------------------ slots

void MainWindow::onStart()
{
    if (m_running || !prepareRun())
        return;
    setRunning(true);
}

void MainWindow::onPause()
{
    // Stops the timer only; the temperature grid is kept.
    setRunning(false);
}

void MainWindow::onReset()
{
    setRunning(false);
    const SimulationSettings s = collectSettings();
    if (!validateOrWarn(s))
        return;
    applyStructure(s);
    m_timer.setInterval(s.updateIntervalMs);
    setRunning(false);   // refresh "Ready" label
}

void MainWindow::onStep()
{
    if (m_running || !prepareRun())
        return;
    m_engine.step();
    refreshView();
    setRunning(false);   // refresh state label ("Paused")
}

void MainWindow::onTick()
{
    // 1. calculate  2. update grid (swap)  3. refresh heatmap  4. update statistics
    m_engine.advance(m_steps->value());
    refreshView();
}

void MainWindow::onCellSelected(int x, int y)
{
    m_selX = x;
    m_selY = y;
    m_heatmap->setSelectedCell(x, y);
    updateStatistics();
}

void MainWindow::onConfigEdited()
{
    const SimulationSettings s = collectSettings();

    const bool stable = s.diffusionCoefficient > 0.0 &&
                        s.diffusionCoefficient <= SimulationSettings::kMaxStableR;
    m_stabilityLabel->setText(stable ? tr("r is stable (r <= 0.25)")
                                     : tr("UNSTABLE: r must be <= 0.25"));
    m_stabilityLabel->setStyleSheet(stable ? QStringLiteral("color:#6fdc8c;")
                                           : QStringLiteral("color:#ff6b6b; font-weight:600;"));

    m_pendingLabel->setVisible(!s.sameInitialConditions(m_applied));

    // Run-time parameters apply immediately.
    m_timer.setInterval(s.updateIntervalMs);
    if (stable)
        m_engine.setDiffusionCoefficient(s.diffusionCoefficient);
}

void MainWindow::onCenterHotRegion()
{
    SimulationSettings s = collectSettings();
    s.centerHotRegion();
    m_hotX->setValue(s.hotX);
    m_hotY->setValue(s.hotY);
    m_hotW->setValue(s.hotWidth);
    m_hotH->setValue(s.hotHeight);
}

void MainWindow::onExport()
{
    if (!m_engine.isInitialized())
        return;

    const QString defaultPath =
        QDir(QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation))
            .filePath(QStringLiteral("simulab_step_%1.csv").arg(m_engine.stepCount()));

    QString path = QFileDialog::getSaveFileName(this, tr("Export temperature grid"),
                                                defaultPath, tr("CSV files (*.csv);;All files (*)"));
    if (path.isEmpty())
        return;
    if (QFileInfo(path).suffix().isEmpty())
        path += QStringLiteral(".csv");

    QString error;
    if (CsvExporter::exportGrid(path, m_engine.temperatures(), m_engine.width(), m_engine.height(), &error))
        statusBar()->showMessage(tr("Exported step %1 to %2").arg(m_engine.stepCount()).arg(path), 8000);
    else
        QMessageBox::critical(this, tr("Export failed"), error);
}

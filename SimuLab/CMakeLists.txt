cmake_minimum_required(VERSION 3.16)

project(SimuLab VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Qt's meta-object compiler (needed for Q_OBJECT / signals / slots)
set(CMAKE_AUTOMOC ON)

if(MSVC)
    add_compile_options(/utf-8)
endif()

find_package(Qt6 REQUIRED COMPONENTS Widgets)

set(SIMULAB_SOURCES
    src/main.cpp
    src/MainWindow.h
    src/MainWindow.cpp
    src/SimulationEngine.h
    src/SimulationEngine.cpp
    src/SimulationSettings.h
    src/SimulationSettings.cpp
    src/HeatmapWidget.h
    src/HeatmapWidget.cpp
    src/ColorMap.h
    src/CsvExporter.h
    src/CsvExporter.cpp
)

qt_add_executable(SimuLab ${SIMULAB_SOURCES})

target_link_libraries(SimuLab PRIVATE Qt6::Widgets)

set_target_properties(SimuLab PROPERTIES
    WIN32_EXECUTABLE ON      # no console window on Windows
    MACOSX_BUNDLE ON
)

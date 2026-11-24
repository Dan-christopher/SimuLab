#pragma once

#include <QString>

#include <vector>

/**
 * CsvExporter - writes a temperature grid to a CSV file (QFile + QTextStream).
 *
 * Layout (matrix form, one line per grid row):
 *
 *      y\x,0,1,2,...,W-1          <- column headers (x coordinate)
 *      0,20.0000,20.0000,...      <- row identifier (y coordinate) + temperatures
 *      1,...
 *
 * Numbers are formatted with QString::number(), which is locale-independent
 * (always '.' as decimal separator), so the file opens identically everywhere.
 */
class CsvExporter
{
public:
    /// Returns true on success; otherwise false and, if given, a readable reason in *errorMessage.
    static bool exportGrid(const QString &filePath,
                           const std::vector<double> &temperatures,
                           int width, int height,
                           QString *errorMessage = nullptr);
};

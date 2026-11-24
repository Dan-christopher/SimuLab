#include "CsvExporter.h"

#include <QFile>
#include <QTextStream>

namespace {
bool fail(QString *out, const QString &message)
{
    if (out)
        *out = message;
    return false;
}
} // namespace

bool CsvExporter::exportGrid(const QString &filePath,
                             const std::vector<double> &temperatures,
                             int width, int height,
                             QString *errorMessage)
{
    if (width <= 0 || height <= 0 ||
        temperatures.size() != static_cast<std::size_t>(width) * static_cast<std::size_t>(height))
        return fail(errorMessage, QStringLiteral("There is no valid temperature grid to export."));

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate))
        return fail(errorMessage,
                    QStringLiteral("Cannot open \"%1\" for writing: %2")
                        .arg(filePath, file.errorString()));

    QTextStream out(&file);

    // Header row: corner label + x coordinates.
    QString line;
    line.reserve(width * 12);
    line = QStringLiteral("y\\x");
    for (int x = 0; x < width; ++x)
        line += QLatin1Char(',') + QString::number(x);
    out << line << '\n';

    // One row per grid line: y coordinate + temperatures.
    for (int y = 0; y < height; ++y) {
        line.clear();
        line += QString::number(y);
        const double *row = &temperatures[static_cast<std::size_t>(y) * static_cast<std::size_t>(width)];
        for (int x = 0; x < width; ++x)
            line += QLatin1Char(',') + QString::number(row[x], 'f', 4);
        out << line << '\n';
    }

    out.flush();
    if (out.status() != QTextStream::Ok || file.error() != QFileDevice::NoError)
        return fail(errorMessage,
                    QStringLiteral("Writing to \"%1\" failed: %2").arg(filePath, file.errorString()));

    file.close();
    return true;
}

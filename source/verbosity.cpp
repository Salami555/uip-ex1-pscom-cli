
#include "verbosity.h"

#include <QString>
#include <QDebug>

/**
 * qDebug for verbose and debug messages
 * qInfo for informational user outputs
 * qWarning for non-blocking warnings
 * qCritical for program interrupts (user confirmations)
 * qFatal for exceptions
 * 
 * verbosities:
 * quiet/silent - no output, fail silently on criticals using status codes
 * normal/info - output includes levels down to info
 * verbose/debug - output includes all levels
 */

LogLevel LOG_LEVEL = LogLevel::normal;

const QVector<QStringList> VERBOSITY = {
    {"quiet", "silent"},
    {"normal", "info"},
    {"verbose", "debug"}
};

void setVerbosity(const QString & verbosity) {
    QString verbosityStr = verbosity.toLower();
    qDebug() << verbosityStr;
    for(int i = 0; i < VERBOSITY.count(); ++i) {
        if(VERBOSITY[i].contains(verbosityStr)) {
            LOG_LEVEL = static_cast<LogLevel>(i);
            qDebug() << QString("Verbosity=%1").arg(verbosityStr);
            return;
        }
    }
}

void VerbosityHandler(QtMsgType type, const QMessageLogContext & /*context*/, const QString & message)
{
    // QString ansiEsc("\033[%1m");
    // if(logLevelOfMsgType(type) < MAX_LOG_LEVEL) {
    //     return;
    // }

    QString prefix("");
    auto stream = stdout;

    switch (type) {
    case QtDebugMsg:
        prefix = QString("[DEBUG] ");
        break;
    case QtInfoMsg:
        prefix = QString("");
        break;
    case QtWarningMsg:
        prefix = QString("Warning: ");
        break;
    case QtCriticalMsg:
        break;
    case QtFatalMsg:
        prefix = QString("Fatal Error: ");
        break;
    }

    const QString format("%1%2");
    const auto local = format
        .arg(prefix)
        // .arg(ansiEsc.arg(";1;31"))
        .arg(message)
        .toLocal8Bit();
    fprintf(stream, "%s\n", local.constData());
}

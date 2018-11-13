
#include "verbosity.h"

#include <QString>
#include <QDebug>

bool Logging::quiet = false;
bool Logging::verbose = false;
bool Logging::suppressWarnings = false;

void VerbosityHandler(QtMsgType type, const QMessageLogContext & /*context*/, const QString & message)
{
    if(Logging::quiet) {
        return;
    }

    QString prefix("");
    bool noNewline = false;
    auto stream = stdout;

    /**
     * qDebug for verbose and debug messages
     * qInfo for informational user outputs
     * qWarning for non-blocking warnings
     * qCritical for program interrupts (user confirmations) or invalid arguments
     * qFatal for exceptions
     */

    switch (type) {
    case QtDebugMsg:
        if(!Logging::verbose) {
            return;
        }
        prefix = QString("DEBUG");
        break;
    case QtInfoMsg:
        prefix = QString("INFO ");
        break;
    case QtWarningMsg:
        if(Logging::suppressWarnings) {
            return;
        }
        prefix = QString("WARN ");
        break;
    case QtCriticalMsg:
        noNewline = true;
        prefix = QString("CRIT ");
        stream = stderr;
        break;
    case QtFatalMsg:
        prefix = QString("FATAL ");
        stream = stderr;
        break;
    }

    auto msg = Logging::verbose
        ? QString("[%1] %2").arg(prefix).arg(message)
        : QString("%2").arg(message);
    if(!noNewline) msg.append("\n");
    const auto local = msg.toLocal8Bit();
    fprintf(stream, "%s", local.constData());
}

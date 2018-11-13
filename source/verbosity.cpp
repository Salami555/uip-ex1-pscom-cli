
#include "verbosity.h"

#include <QString>
#include <QDebug>

int logLevelOfMsgType(QtMsgType type) {
    switch(type) {
        case QtDebugMsg:
            return 0;
        case QtInfoMsg:
            return 2;
        case QtWarningMsg:
            return 4;
        case QtCriticalMsg:
            return 8;
        case QtFatalMsg:
            return 16;
        default:
            return 1;
    }
}

int MAX_LOG_LEVEL;

void setVerbosity(const QString & verbosity) {
    QtMsgType type;
    if(verbosity.compare(QString("debug"), Qt::CaseInsensitive) == 0) {
        type = QtDebugMsg;
    } else if(verbosity.compare(QString("info"), Qt::CaseInsensitive) == 0) {
        type = QtInfoMsg;
    } else if(verbosity.compare(QString("warning"), Qt::CaseInsensitive) == 0) {
        type = QtWarningMsg;
    } else if(verbosity.compare(QString("critical"), Qt::CaseInsensitive) == 0) {
        type = QtCriticalMsg;
    } else if(verbosity.compare(QString("fatal"), Qt::CaseInsensitive) == 0) {
        type = QtFatalMsg;
    } else {
        MAX_LOG_LEVEL = 1;
        return;
    }
    MAX_LOG_LEVEL = logLevelOfMsgType(type);
    qDebug() << QString("Verbosity=%1").arg(verbosity);
}

void VerbosityHandler(QtMsgType type, const QMessageLogContext & /*context*/, const QString & message)
{
    // QString ansiEsc("\033[%1m");
    if(logLevelOfMsgType(type) < MAX_LOG_LEVEL) {
        return;
    }

    QString prefix("ERROR");
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
        prefix = QString("Critical: ");
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

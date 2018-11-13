#pragma once

#include <qlogging.h>

enum class LogLevel {
    quiet = 0,
    normal = 1,
    verbose = 2
};

QString verbosityLevels();
void setVerbosity(const QString & verbosity);

/**
 * @brief VerbosityHandler - http://doc.qt.io/qt-5/qtglobal.html#qInstallMessageHandler
 */
void VerbosityHandler(QtMsgType type, const QMessageLogContext & context, const QString & message);

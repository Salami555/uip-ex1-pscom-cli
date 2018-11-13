#pragma once

#include <qlogging.h>

void setVerbosity(const QString & verbosity);

/**
 * @brief VerbosityHandler - http://doc.qt.io/qt-5/qtglobal.html#qInstallMessageHandler
 */
void VerbosityHandler(QtMsgType type, const QMessageLogContext & context, const QString & message);

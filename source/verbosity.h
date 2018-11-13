#pragma once

#include <qlogging.h>

class Logging {
    public:
        static bool quiet; // disables every output, fails silently
        static bool verbose; // enables qDebug output
        static bool suppressWarnings; // disables qWarning output
};

/**
 * @brief VerbosityHandler - http://doc.qt.io/qt-5/qtglobal.html#qInstallMessageHandler
 */
void VerbosityHandler(QtMsgType type, const QMessageLogContext & context, const QString & message);

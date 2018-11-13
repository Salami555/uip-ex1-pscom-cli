
#include <pscom.h>

#include <QCoreApplication>
#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QDebug>
#include <QRegExp>
#include <QVersionNumber>

#include "verbosity.h"


/* PLEASE NOTE
 * - be carful using the pscom library, it may irreversible delete actual data if used without care.
 * - try to rely on pscom library exclusivel in order to fulfill the user stories / tasks.
 * - use qt only to drive the library and control command language aspects and CLI related features, e.g.,
 * progress, logging, verbosity, undo, help, etc.
 * - prefer qDebug, qInfo, qWarning, qCritical, and qFatal for verbosity and console output.
 */

using namespace std;

namespace lib_utils {

    namespace io_ops {
        QStringList supportedFormats() {
            return pscom::sf();
        }
        bool isPathExistingDirectory(const QString & path) {
            return pscom::de(path);
        }
        bool isPathExistingFile(const QString & path) {
            return pscom::fe(path);
        }
        bool isPathExisting(const QString & path) {
            return !pscom::ne(path);
        }
        QString fileExtension(const QString & path) {
            if(!isPathExistingFile(path)) {
                throw "";
            }
            return pscom::fs(path);
        }
        QString replaceFileExtensionIfSupported(const QString & path, const QString & extension) {
            if(!supportedFormats().contains(extension)) {
                throw "";
            }
            return pscom::cs(path, extension);
        }

    }

}

static const auto VERSION = QVersionNumber(1, 0, 0);

void initApplication() {
    // Setting the application name is not required, since, if not set, it defaults to the executable name.
    QCoreApplication::setApplicationName("pscom-cli");
    QCoreApplication::setApplicationVersion(VERSION.toString());
    
    // Read pscom version with format "%appName% version %appVersion% | pscom-%pscomVersion% qt-%qtVersion%"
    auto libVersion = pscom::vi();
    auto libVersionMessage = libVersion.right(libVersion.length() - libVersion.indexOf("pscom-", 1));
    
    QString versionString("%1 v%2 | %3");
    QCoreApplication::setApplicationVersion(versionString
        .arg(QCoreApplication::applicationName())
        .arg(QCoreApplication::applicationVersion())
        .arg(libVersionMessage));
}

int main(int argc, char *argv[])
{
    qInstallMessageHandler(VerbosityHandler);
    QCoreApplication app(argc, argv);
    initApplication();

    /* ToDo - have fun! */
    
    // qDebug() << app.applicationDirPath();

    qDebug() << app.applicationName();
    qDebug() << app.applicationVersion();

    return app.exec();
}

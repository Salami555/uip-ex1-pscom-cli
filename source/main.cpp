
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

namespace lib_utils {
    QStringList supportedFormats() {
        return pscom::sf();
    }

    namespace io_ops {
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

static const QString APP_NAME("pscom-cli");
static const QVersionNumber APP_VERSION(1, 0, 0);

void initApplication() {
    // Setting the application name is not required, since, if not set, it defaults to the executable name.
    QCoreApplication::setApplicationName(APP_NAME);
    QCoreApplication::setApplicationVersion(APP_VERSION.toString());
    
    // Read pscom version with format "%appName% version %appVersion% | pscom-%pscomVersion% qt-%qtVersion%"
    auto libVersion = pscom::vi();
    auto libVersionMessage = libVersion.right(libVersion.length() - libVersion.indexOf("pscom-", 1));
    
    QString versionString("%1 v%2 | %3");
    QCoreApplication::setApplicationVersion(versionString
        .arg(QCoreApplication::applicationName())
        .arg(QCoreApplication::applicationVersion())
        .arg(libVersionMessage));
}

void showSupportedFormats() {
    QString seperator(", ");
    qInfo().noquote() << QString("Supported image formats are: %1")
        .arg(lib_utils::supportedFormats().join(seperator));
}

int main(int argc, char *argv[])
{
    qInstallMessageHandler(VerbosityHandler);
    QCoreApplication app(argc, argv);
    initApplication();

    // create parser including default help and version support
	QCommandLineParser parser;
	parser.addVersionOption();
	parser.addHelpOption();
    QCommandLineOption verbosity("verbosity", "verbosity level of the output", "debug|info|warning|critical|fatal", "info");
    QCommandLineOption supportedFormatsFlag("supported-formats", "lists the supported file formats");
    parser.addOption(verbosity);
    parser.addOption(supportedFormatsFlag);

    /*if(argc <= 1) {
        // exit with error code 1 because the user didn't supply any arguments
        parser.showHelp(1);
    }*/

    // parse currently supported arguments
	parser.parse(QCoreApplication::arguments());

    setVerbosity(parser.value(verbosity));
    qInfo().noquote() << QString("Welcome to %1. Use --help to view all arguments.").arg(APP_NAME);

    if(parser.isSet(supportedFormatsFlag)) {
        qDebug() << QString("SupportedFormatsFlag=1");
        showSupportedFormats();
        return 0;
    }

    // process supplied arguments
	parser.process(app);
    
    qDebug() << QString("ExecutionDirectory=%1").arg(app.applicationDirPath());

    return app.exec();
}

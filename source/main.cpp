
#include <pscom.h>

#include <QCoreApplication>
#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QDebug>
#include <QRegExp>
#include <QVersionNumber>
#include <iostream>

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

void initApplication(QCoreApplication & app) {
    // Setting the application name is not required, since, if not set, it defaults to the executable name.
    app.setApplicationName(APP_NAME);
    app.setApplicationVersion(APP_VERSION.toString());
    
    // Read pscom version with format "%appName% version %appVersion% | pscom-%pscomVersion% qt-%qtVersion%"
    auto libVersion = pscom::vi();
    auto libVersionMessage = libVersion.right(libVersion.length() - libVersion.indexOf("pscom-", 1));
    
    QString versionString("v%2 | %3");
    app.setApplicationVersion(versionString
        .arg(app.applicationVersion())
        .arg(libVersionMessage));
}

static const QCommandLineOption quietFlag("quiet", "Sets the output to silent log level (no output, fails silently).");
static const QCommandLineOption verboseFlag("verbose", "Sets the output to verbose log level.");
static const QCommandLineOption suppressWarningsFlag("suppress-warnings", "Sets the output to verbose log level.");
static const QCommandLineOption supportedFormatsFlag("supported-formats", "Lists the supported file formats.");

QDebug _debug() {
    return qDebug().noquote();
}
QDebug _info() {
    return qInfo().noquote();
}
QDebug _warn() {
    return qWarning().noquote();
}
QDebug _crit() {
    return qCritical().noquote();
}
void critExit(const QString & message) {
    Logging::quiet = false;
    _crit() << message << "\n";
    std::exit(1);
}
bool userConfirmation(const QString & message) {
    _crit() << message << "[y/n] ";
    std::string input;
    std::cin >> input;
    switch(input[0]) {
        case 'y':
            return true;
        case 'n':
            return false;
        default:
            _warn() << QString("Unknown input \"%1\" - aborting!").arg(input.data());
            return false;
    }
}

void initParserAndLogging(const QCoreApplication & app, QCommandLineParser & parser) {
	parser.addVersionOption();
	parser.addHelpOption();
    parser.addOptions({quietFlag, verboseFlag, suppressWarningsFlag});
    parser.addPositionalArgument("command", "The pscom command to execute.", "<command> [<args>]");
    if(app.arguments().count() <= 1) {
        _info() << QString("Welcome to %1.").arg(APP_NAME);
        // exit with error code 1 because the user didn't supply any arguments
        parser.showHelp(1);
    }

    parser.parse(app.arguments()); // ignore unknown options for now
    Logging::quiet = parser.isSet(quietFlag);
    Logging::verbose = parser.isSet(verboseFlag);
    if(Logging::quiet && Logging::verbose) {
        std::exit(-1);
        // critExit("invalid arguments: --verbose cannot be set at the same time with --quiet");
    }
    Logging::suppressWarnings = parser.isSet(suppressWarningsFlag);
    // _debug()
    //     << QString("Quiet=%1").arg(Logging::quiet)
    //     << QString("Verbose=%2").arg(Logging::verbose)
    //     << QString("Suppress-Warnings=%3").arg(Logging::suppressWarnings);
}

void showSupportedFormats() {
    if(Logging::verbose) {
        _info() << QString("Supported image formats: %1")
            .arg(lib_utils::supportedFormats().join(", "));
    } else {
        _info() << lib_utils::supportedFormats().join("|");
    }
}

int main(int argc, char *argv[])
{
    qInstallMessageHandler(VerbosityHandler);
    QCoreApplication app(argc, argv);
    initApplication(app);

    // create parser including default help and version support
    QCommandLineParser parser;
    parser.addOptions({supportedFormatsFlag});
    initParserAndLogging(app, parser);
    // _debug() << QString("ExecutionDirectory=%1").arg(app.applicationDirPath());
    
    if(parser.isSet(supportedFormatsFlag)) {
        if(Logging::quiet) {
            return -1;
            // critExit("invalid arguments: --quiet suppresses output of --supported-formats");
        }
        // _debug() << QString("SupportedFormatsFlag=1");
        showSupportedFormats();
        return 0;
    }

    QStringList args = parser.positionalArguments();
    if(args.empty()) {
        // exit with error code 1 because the user didn't supply any arguments
        parser.showHelp(1);
    }

    return app.exec();
}

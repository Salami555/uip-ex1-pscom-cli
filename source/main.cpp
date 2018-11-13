
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

QDebug _debug() {
    return qDebug().noquote();
}
QDebug _info() {
    return qInfo().noquote();
}
QDebug _warn() {
    return qWarning().noquote();
}
void abnormalExit(const QString & message, int exitCode = 1) {
    qCritical().noquote() << message << "\n";
    std::exit(exitCode);
}
void fatalExit(const QString & message, int terminationCode = -1) {
    qFatal(message.toLocal8Bit().data());
    std::exit(terminationCode);
}

bool userConfirmation(const QString & message) {
    qCritical().noquote() << message << "[y/n] ";
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

const int progressBarWidth = 42;
const char
    filledProgress = '#',
    emptyProgress = '.';
void drawProgressBar(double progress, bool newLine = false) {
    if(progress < 0) progress = 0;
    if(progress > 1) progress = 1;

    const int width = (int) (progressBarWidth * progress);
    QTextStream(stdout) << QString("[%1] %L2%")
        .arg(QString(filledProgress).repeated(width), -progressBarWidth, emptyProgress)
        .arg(progress * 100, 5, 'f', 1);
    if(newLine)
        QTextStream(stdout) << '\n';
}
void clearProgressBar() {
    QTextStream(stdout) << "\r"
        << QString(" ").repeated(progressBarWidth + 9) // "[" + bar{width} + "] " + number{5} + "%"
        << "\r";
}

namespace lib_utils {
    QStringList supportedFormats() {
        return pscom::sf();
    }

    namespace io_ops {
        bool recursive = false;
        bool dryRun = false;

        bool isPathExistingDirectory(const QString & path) {
            return pscom::de(path);
        }
        bool isPathExistingFile(const QString & path) {
            return pscom::fe(path);
        }
        bool isPathExisting(const QString & path) {
            return !pscom::ne(path);
        }

        namespace filepath_ops {
            QString fileExtension(const QString & filepath) {
                if(!isPathExistingFile(filepath)) {
                    throw "file not found";
                }
                return pscom::fs(filepath);
            }
            QString pathSetFileExtension(const QString & filepath, const QString & extension) {
                if(!supportedFormats().contains(extension)) {
                    throw "unsupported file format";
                }
                return pscom::cs(filepath, extension);
            }
            QString pathSetDatedFileBaseName(const QString & filepath, const QString & dateTimeFormat, const QDateTime & dateTime) {
                return pscom::fn(filepath, dateTime, dateTimeFormat);
            }
            QString pathInsertDatedDirectory(const QString & filepath, const QString & dateFormat, const QDate & date) {
                return pscom::fp(filepath, date, dateFormat);
            }
        }

        bool moveFiles(const QStringList & filepaths, std::function<void (const QString &, bool, int, int)> fileCompletionCallback) {
            if(filepaths.empty())
                return true;
            
            QStringList unsuccessful;
            const int total = filepaths.count();
            for(int i = 0; i < total; ++i) {
                const QString filepath = filepaths[i];
                const int pos = i + 1;
                bool success = false;
                clearProgressBar();
                _debug() << QString("%1) Moving %2").arg(pos).arg(filepath);
                drawProgressBar((pos-1)/total);
                // todo move
                clearProgressBar();
                fileCompletionCallback(filepath, success, pos, total);
                _info() << QString("%1) Moved %2").arg(pos).arg(filepath);
                drawProgressBar(pos/total);
                if(!success) {
                    unsuccessful << filepath;
                }
            }
            clearProgressBar();
            // drawProgressBar(1.0, true);
            if(!unsuccessful.empty()) {
                _warn() << QString("Unsuccessfully moved files: %1").arg(unsuccessful.count());
            }
            return unsuccessful.empty();
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

void initParserAndLogging(const QCoreApplication & app, QCommandLineParser & parser) {
	parser.addVersionOption();
	parser.addHelpOption();
    parser.addOptions({quietFlag, verboseFlag, suppressWarningsFlag});
    parser.addPositionalArgument("command", "pscom command to execute", "<command> [<args>]");
    parser.addPositionalArgument("path", "directory path", "<path>");
    if(app.arguments().count() <= 1) {
        _info() << QString("Welcome to %1.").arg(APP_NAME);
        // exit with error code 1 because the user didn't supply any arguments
        parser.showHelp(1);
    }

    parser.parse(app.arguments()); // ignore unknown options for now
    Logging::quiet = parser.isSet(quietFlag);
    Logging::verbose = parser.isSet(verboseFlag);
    if(Logging::quiet && Logging::verbose) {
        abnormalExit("invalid arguments: --verbose cannot be set at the same time with --quiet");
    }
    Logging::suppressWarnings = parser.isSet(suppressWarningsFlag);
}

void showSupportedFormats() {
    if(Logging::verbose) {
        _info() << QString("Supported image formats: %1")
            .arg(lib_utils::supportedFormats().join(", "));
    } else {
        _info() << lib_utils::supportedFormats().join("|");
    }
}

static const QCommandLineOption listRecursiveFlag("recursive");
static const QCommandLineOption listFilterDateAfter("after", "Filters the files beeing created after the given date.", "<DATETIME>");
static const QCommandLineOption listFilterDateBefore("before", "Filters the files beeing created before the given date.", "<DATETIME>");
static const QCommandLineOption listFilterRegex("match", "Matches the filenames against the given regex.", "<REGEX>");

int main(int argc, char *argv[])
{
    qInstallMessageHandler(VerbosityHandler);
    QCoreApplication app(argc, argv);
    initApplication(app);

    // create parser including default help and version support
    QCommandLineParser parser;
    parser.addOptions({supportedFormatsFlag});
    initParserAndLogging(app, parser);
    
    if(parser.isSet(supportedFormatsFlag)) {
        if(Logging::quiet) {
            abnormalExit("invalid arguments: --quiet suppresses output of --supported-formats");
        }
        showSupportedFormats();
        return 0;
    }

    QStringList args = parser.positionalArguments();
    if(args.empty()) {
        // exit with error code 1 because the user didn't supply any arguments
        parser.showHelp(1);
    }
    const auto command = args[0].toLower();
    
    lib_utils::io_ops::moveFiles({"./a.txt"}, [](const QString & filepath, bool success, int i, int total) {
        _debug() << filepath << success << i << total;
    });
    // if(command == "list") {
        
    // } else if(command == "copy") {

    // } else if(command == "move") {

    // } else if(command == "rename") {
        
    // } else if(command == "group") {
        
    // } else if(command == "shrink") {
        
    // } else if(command == "format") {
        
    // } else if(command == "quality") {
        
    // }
    // process supplied arguments
	// parser.process(app);

    return app.exec();
}

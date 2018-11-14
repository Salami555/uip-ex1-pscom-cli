
#include <pscom.h>

#include <QCoreApplication>
#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QDebug>
#include <QDir>
#include <QFileInfo>
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

QTextStream & _stdout() {
    QTextStream out(stdout, QIODevice::WriteOnly | QIODevice::Unbuffered);
    return out;
}

bool progressBarVisible = false;
const int progressBarWidth = 42;
const char
    filledProgress = '#',
    emptyProgress = '.';
void drawProgressBar(double progress, bool newLine = false) {
    if(Logging::quiet)
        return;
    if(progress < 0) progress = 0;
    if(progress > 1) progress = 1;

    const int width = (int) (progressBarWidth * progress);
    _stdout() << QString("[%1] %L2%")
        .arg(QString(filledProgress).repeated(width), -progressBarWidth, emptyProgress)
        .arg(progress * 100, 5, 'f', 1);
    progressBarVisible = true;
    if(newLine) {
        _stdout() << '\n';
        progressBarVisible = false;
    }
}
void clearProgressBar() {
    if(progressBarVisible) {
        _stdout() << "\r"
            << QString(" ").repeated(progressBarWidth + 9) // "[" + bar{width} + "] " + number{5} + "%"
            << "\r";
        progressBarVisible = false;
    }
}

QDebug _debug() {
    clearProgressBar();
    return qDebug().noquote();
}
QDebug _info() {
    clearProgressBar();
    return qInfo().noquote();
}
QDebug _warn() {
    clearProgressBar();
    return qWarning().noquote();
}
void abnormalExit(const QString & message, int exitCode = 1) {
    clearProgressBar();
    qCritical().noquote() << message << "\n";
    std::exit(exitCode);
}
void fatalExit(const QString & message, int terminationCode = -1) {
    clearProgressBar();
    qFatal(message.toLocal8Bit().data());
    std::exit(terminationCode);
}

bool userConfirmation(const QString & message) {
    if(Logging::quiet)
        fatalExit("required confirmation in quiet mode");
    clearProgressBar();
    qCritical().noquote() << message << "[y/n] ";
    std::string input;
    std::cin >> input;
    switch(input[0]) {
        case 'y':
            return true;
        case 'n':
            return false;
        default:
            _warn() << QString("Unknown input \"%1\" - assuming no").arg(input.data());
            return false;
    }
}

static int log10(int i) {
    return (i >= 1000000000)
        ? 9 : (i >= 100000000)
        ? 8 : (i >= 10000000)
        ? 7 : (i >= 1000000)
        ? 6 : (i >= 100000)
        ? 5 : (i >= 10000)
        ? 4 : (i >= 1000)
        ? 3 : (i >= 100)
        ? 2 : (i >= 10)
        ? 1 : 0; 
}
QString progressMessage(int pos, int total, const QString & operation) {
    int size = log10(total); 
    return QString("[%1/%2]: %3")
        .arg(pos, size)
        .arg(total, size)
        .arg(operation);
}

namespace IOSettings {
    QStringList sourceDirectories;
    QString targetDirectory = "./";
    QRegExp filterRegex(".*");
    QString filterDateFormat = "yyyy-MM-dd";
    QDateTime filterDateTimeAfter, filterDateTimeBefore;
    bool recursive = false;
    bool skipExisting = false;
    bool forceOverwrite = false;
    bool dryRun = false;
    bool progressBar = false;
}

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

        namespace filepath_ops {
            QString fileExtension(const QString & filepath) {
                if(!isPathExistingFile(filepath)) {
                    throw "file not found";
                }
                return pscom::fs(filepath);
            }
            QString fileName(const QString & filepath) {
                // missing this in the library
                const auto fi = QFileInfo(filepath);
                return fi.completeBaseName() + '.' + fi.suffix();
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

        QDateTime fileCreationDateTime(const QString & filepath) {
            if(!isPathExistingFile(filepath)) {
                throw QString("File not found \"%1\"").arg(filepath);
            }
            return pscom::et(filepath);
        }

        bool removeFile(const QString & filepath) {
            if(!isPathExisting(filepath)) {
                return true;
            }
            if(!isPathExistingFile(filepath)) {
                _warn() << QString("Not a file to remove \"%1\"").arg(filepath);
                return false;
            }
            return IOSettings::dryRun || pscom::rm(filepath);
        }
        bool isFileOverwritePermitted(
            const QString & targetFilepath, const QString & confirmationMessge,
            bool force = false, bool userConfirm = true
        ) {
            if(!force) {
                if(!userConfirm || !userConfirmation(confirmationMessge)) {
                    _debug() << QString("Skipped file \"%1\"").arg(targetFilepath);
                    return false;
                }
            }
            _debug() << QString("Removing file \"%1\"").arg(targetFilepath);
            if(!removeFile(targetFilepath)) {
                _warn() << QString("Removing file failed \"%1\"").arg(targetFilepath);
                return false;
            }
            return true;
        }
        bool copyFile(const QString & sourceFilepath, const QString & targetFilepath, bool force = false, bool userConfirm = true) {
            if(sourceFilepath == targetFilepath) {
                _debug() << QString("Equal source and target file \"%1\"").arg(sourceFilepath);
                return true;
            }
            if(!isPathExistingFile(sourceFilepath)) {
                _warn() << QString("File not found \"%1\"").arg(sourceFilepath);
                return false;
            }
            if(isPathExistingFile(targetFilepath)) {
                if(!isFileOverwritePermitted(targetFilepath, QString("Overwrite file \"%1\" with \"%2\"?")
                    .arg(targetFilepath).arg(sourceFilepath), force, userConfirm)) {
                    _warn() << QString("Copying file failed caused by existing target \"%1\"").arg(targetFilepath);
                    return false;
                }
            }
            _debug() << QString("Copying file \"%1\" to \"%2\"").arg(sourceFilepath).arg(targetFilepath);
            return IOSettings::dryRun || pscom::cp(sourceFilepath, targetFilepath);
        }
        bool moveFile(const QString & sourceFilepath, const QString & targetFilepath, bool force = false, bool userConfirm = true) {
            if(sourceFilepath == targetFilepath) {
                _debug() << QString("Equal source and target file \"%1\"").arg(sourceFilepath);
                return true;
            }
            if(!isPathExistingFile(sourceFilepath)) {
                _warn() << QString("File not found \"%1\"").arg(sourceFilepath);
                return false;
            }
            if(isPathExistingFile(targetFilepath)) {
                if(!isFileOverwritePermitted(targetFilepath, QString("Overwrite file \"%1\" with \"%2\"?")
                    .arg(targetFilepath).arg(sourceFilepath), force, userConfirm)) {
                    _warn() << QString("Moving file failed caused by existing target \"%1\"").arg(targetFilepath);
                    return false;
                }
            }
            _debug() << QString("Moving file \"%1\" to \"%2\"").arg(sourceFilepath).arg(targetFilepath);
            return IOSettings::dryRun || pscom::mv(sourceFilepath, targetFilepath);
        }
        bool renameFile(const QString & sourceFilepath, const QString & targetFilepath) {
            return moveFile(sourceFilepath, targetFilepath, true);
        }

        bool createDirectories(const QString & path) {
            if(isPathExisting(path)) {
                return true;
            }
            return IOSettings::dryRun || pscom::mk(path);
        }
        // bool moveDirectory(const QString & sourcePath, const QString & targetPath) {
        //     if(sourcePath == targetPath) {

        //     }
        //     if(!isPathExistingDirectory(sourcePath)) {
        //         throw "Directory not found";
        //     }
        //     if(isPathExistingDirectory(targetPath)) {
        //         throw "Target directory already exists";
        //     }
        //     return dryRun || pscom::mv(sourcePath, targetPath);
        // }
        // bool renameDirectory(const QString & sourcePath, const QString & targetPath) {
        //     return moveDirectory(sourcePath, targetPath);
        // }
        
        void filter(QStringList & fileList, std::function<bool (const QString &)> filter) {
            QMutableStringListIterator i(fileList);
            while(i.hasNext()) {
                if(!filter(i.next())) {
                    i.remove();
                }
            }
        }
        void filterFileListExtensions(QStringList & fileList, const QStringList & extensionList) {
            filter(fileList, [&](const QString & filename) {
                return isPathExistingFile(filename)
                    && extensionList.contains(filepath_ops::fileExtension(filename));
            });
        }
        void filterFileListDateAfter(QStringList & fileList, const QDateTime & minDateTime) {
            filter(fileList, [&](const QString & filename) {
                return minDateTime < fileCreationDateTime(filename);
            });
        }
        void filterFileListDateBefore(QStringList & fileList, const QDateTime & maxDateTime) {
            filter(fileList, [&](const QString & filename) {
                return fileCreationDateTime(filename) < maxDateTime;
            });
        }
        QStringList listFiles(const QString & path, bool recursive, const QRegExp & regex = QRegExp(".*")) {
            if(!isPathExistingDirectory(path)) {
                abnormalExit(QString("Source directory not found \"%1\"").arg(path), 6);
            }
            _debug() << QString("Listing directory \"%1\"").arg(path);
            auto fileList = pscom::re(path, regex, recursive);
            filterFileListExtensions(fileList, supportedFormats());
            _debug() << QString("%1 supported files found").arg(fileList.count());
            return fileList;
        }
        QStringList listFiles(const QStringList & paths, bool recursive, const QRegExp & regex = QRegExp(".*")) {
            QStringList files;
            for(const QString & path : paths) {
                files.append(listFiles(path, recursive, regex));
            }
            return files;
        }
        QStringList listFiles() {
            using namespace IOSettings;
            auto fileList = listFiles(sourceDirectories, recursive, filterRegex);
            if(filterDateTimeAfter.isValid()) {
                filterFileListDateAfter(fileList, filterDateTimeAfter);
            }
            if(filterDateTimeBefore.isValid()) {
                filterFileListDateBefore(fileList, filterDateTimeBefore);
            }
            _debug() << QString("%1 filtered files found").arg(fileList.count());
            return fileList;
        }

        QStringList multiFileOperation(
            const QStringList & fileList,
            std::function<QString (const QString &)> operationMessage,
            std::function<bool (const QString &)> operation,
            bool silent = false
        ) {
            QStringList unsuccessful;
            const int total = fileList.count();
            for(int i = 0; i < total; ++i) {
                const QString filepath = fileList[i];
                const int pos = i + 1;
                if(!silent)
                    _info() << progressMessage(pos, total, operationMessage(filepath));
                // if(IOSettings::progressBar) drawProgressBar((pos-1)/total);
                bool success = operation(filepath);
                _debug() << progressMessage(pos, total, QString("Finished %1").arg(operationMessage(filepath)));
                // if(IOSettings::progressBar) drawProgressBar(pos/total);
                if(!success) {
                    unsuccessful << filepath;
                }
            }
            // clearProgressBar();
            return unsuccessful;
        }
    }
}

struct Task {
    std::function<void (QCommandLineParser &)> parameterInitializer;
    std::function<int (QCommandLineParser &)> taskHandler;
    bool unknown;
};

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

// Verbosity flags instead of --verbosity levels
static const QCommandLineOption quietFlag({"q", "quiet", "silent"}, "Sets the output to silent log level (no output, fails silently).");
static const QCommandLineOption verboseFlag({"verbose", "debug"}, "Sets the output to debug log level.");
static const QCommandLineOption suppressWarningsFlag("suppress-warnings", "Suppresses warnings but keeps every other output.");

static const QCommandLineOption supportedFormatsFlag("supported-formats", "Lists the supported file formats.");

// general task flags
static const QCommandLineOption searchRecursivelyFlag({"r", "recursive"}, "Traverse the directory recursively.");
static const QCommandLineOption sourceDirectoryOption({"s", "source", "dir"}, "Source directory.", "source directory", "./");
static const QCommandLineOption filterRegexOption({"match", "regex"}, "Match the filenames against the given regex.", "regex");
static const QCommandLineOption filterDateFormatOption("datetime-format", "Used format for filtering with --after or --before. Default: yyyy-MM-dd (ISO 8601)", "datetime-format", "yyyy-MM-dd");
static const QCommandLineOption filterDateTimeAfterOption("after", "Filter the images to be created after the given date (exclusive).", "datetime");
static const QCommandLineOption filterDateTimeBeforeOption("before", "Filter the images to be created before the given date (exclusive).", "datetime");

// specific task flags
static const QCommandLineOption targetDirectoryOption({"t", "target", "dest"}, "Target directory.", "target directory", "./");
static const QCommandLineOption progressBarFlag({"p", "progress"}, "Show a progress bar (if the task supports it).");
static const QCommandLineOption fileOPsSkipExistingFlag({"skip", "skip-existing"}, "Skip existing images without asking.");
static const QCommandLineOption fileOPsForceOverwriteFlag({"force", "overwrite"}, "Overwrite existing images without asking.");
static const QCommandLineOption fileOPsCreateDirectoriesFlag({"mkdirs", "create-directories"}, "Creates missing directories.");
static const QCommandLineOption fileOPsDryRunFlag({"dry-run", "noop"}, "Simulate every file operation without actually doing it.");

// task flags
static const QCommandLineOption renameFormatOption("scheme", "Date time format for renaming the images. Default is UPA scheme: yyyyMMdd_HHmmsszzz", "datetime-format", "yyyyMMdd_HHmmsszzz");
static const QCommandLineOption groupLocationOption("location", "Location name for folder grouping.", "location");
static const QCommandLineOption groupEventOption("event", "Event name for folder grouping.", "event");
static const QCommandLineOption transformCopySuffixOption("suffix", "Keeps the original image and works on a renamed copy with suffixed file base name. Default: _new", "suffix", "_new");
static const QCommandLineOption transformShrinkWidthOption({"w", "width"}, "New image width in px.", "width");
static const QCommandLineOption transformShrinkHeightOption({"h", "height"}, "New image height in px.", "height");
static const QCommandLineOption transformFormatOption("format", "New image format (check supported formats with --supported-formats).", "format");
static const QCommandLineOption transformQualityOption("quality", "New image quality between 0 and 100. Default: 70", "quality", "70");

void registerFileListingSettings(QCommandLineParser & parser) {
    parser.addOptions({sourceDirectoryOption, searchRecursivelyFlag});
    parser.addOptions({filterRegexOption, filterDateFormatOption, filterDateTimeAfterOption, filterDateTimeBeforeOption});
}
void parseFileListingSettings(const QCommandLineParser & parser) {
    using namespace IOSettings;
    recursive = parser.isSet(searchRecursivelyFlag);
    sourceDirectories = parser.values(sourceDirectoryOption);
    if(parser.isSet(filterRegexOption)) {
        QString regexString = parser.value(filterRegexOption);
        filterRegex = QRegExp(regexString);
        if(!filterRegex.isValid()) {
            abnormalExit(QString("Invalid regex given \"%1\"").arg(regexString), 3);
        }
        _debug() << QString("Filtering directories using regex=\"%1\"").arg(regexString);
    }
    if(parser.isSet(filterDateFormatOption)) {
        filterDateFormat = parser.value(filterDateFormatOption);
        _debug() << QString("Using filter datetime format=\"%1\"").arg(filterDateFormat);
    }
    if(parser.isSet(filterDateTimeAfterOption)) {
        QString datetimeString = parser.value(filterDateTimeAfterOption);
        filterDateTimeAfter = QDateTime::fromString(datetimeString, filterDateFormat);
        if(!filterDateTimeAfter.isValid()) {
            _warn() << QString("Invalid filter after datetime \"%1\" using format \"%2\"")
                .arg(datetimeString).arg(filterDateFormat);
        }
        _debug() << QString("Filtering directories using date after=\"%1\"").arg(datetimeString);
    }
    if(parser.isSet(filterDateTimeBeforeOption)) {
        QString datetimeString = parser.value(filterDateTimeBeforeOption);
        filterDateTimeBefore = QDateTime::fromString(datetimeString, filterDateFormat);
        if(!filterDateTimeBefore.isValid()) {
            _warn() << QString("Invalid filter before datetime \"%1\" using format \"%2\"")
                .arg(datetimeString).arg(filterDateFormat);
        }
        _debug() << QString("Filtering directories using date before=\"%1\"").arg(datetimeString);
    }
}
void registerIOSettings(QCommandLineParser & parser) {
    parser.addOptions({targetDirectoryOption, fileOPsCreateDirectoriesFlag});
    parser.addOptions({progressBarFlag, fileOPsDryRunFlag, fileOPsForceOverwriteFlag, fileOPsSkipExistingFlag});
}
void parseIOSettings(const QCommandLineParser & parser) {
    using namespace IOSettings;
    progressBar = parser.isSet(progressBarFlag);
    dryRun = parser.isSet(fileOPsDryRunFlag);
    forceOverwrite = parser.isSet(fileOPsForceOverwriteFlag);
    skipExisting = parser.isSet(fileOPsSkipExistingFlag);
    if(parser.isSet(targetDirectoryOption)) {
        targetDirectory = parser.value(targetDirectoryOption);
        if(!targetDirectory.endsWith(QDir::separator())) {
            targetDirectory.append(QDir::separator());
        }
        using namespace lib_utils::io_ops;
        if(!isPathExisting(targetDirectory)) {
            if(!parser.isSet(fileOPsCreateDirectoriesFlag)) {
                // todo add user confirmation
                abnormalExit(QString("Target directory does not exist \"%1\"").arg(targetDirectory), 4);
            }
            if(createDirectories(targetDirectory)) {
                _debug() << QString("Created target directory \"%1\"").arg(targetDirectory);
            } else {
                abnormalExit(QString("Target directory could not be created \"%1\"").arg(targetDirectory), 5);
            }
        }
    }
}

static const QMap<QString, Task> tasks({
    std::make_pair("list", Task {
        [](QCommandLineParser & parser) {
            parser.clearPositionalArguments();
            parser.addPositionalArgument("list", "Lists all (filtered) images found in the source directories.", "list [list-options]");
            registerFileListingSettings(parser);
        },
        [](QCommandLineParser & parser) {
            parseFileListingSettings(parser);
            auto fileList = lib_utils::io_ops::listFiles();
            const int total = fileList.count();
            for(int i = 0; i < total; ++i) {
                _info() << progressMessage(i+1, total, fileList[i]);
            }
            // try {
            //     _debug() << lib_utils::io_ops::filepath_ops::pathSetDatedFileBaseName("./", "yyyyMMdd_HHmmsszzz", QDateTime::currentDateTime());
            //     _debug() << lib_utils::io_ops::filepath_ops::pathInsertDatedDirectory("./", "yyyyMMdd", QDate::currentDate());
            // } catch (const QString & ex) {
            //     fatalExit(ex);
            // }
            return 0;
        }
    }),
    std::make_pair("copy", Task {
        [](QCommandLineParser & parser) {
            parser.clearPositionalArguments();
            parser.addPositionalArgument("copy", "Copy all (filtered) images found in the source directories to the target directory.", "copy [copy-options]");
            registerFileListingSettings(parser);
            registerIOSettings(parser);
        },
        [](QCommandLineParser & parser) {
            parseFileListingSettings(parser);
            parseIOSettings(parser);
            using namespace lib_utils::io_ops;
            using namespace IOSettings;
            auto fileList = listFiles();
            const int total = fileList.count();
            auto problemFileList = multiFileOperation(fileList,
                [](const QString & filepath) {
                    return QString("Copying %1").arg(filepath);
                },
                [](const QString & filepath) {
                    const auto targetpath = targetDirectory + filepath_ops::fileName(filepath);
                    return copyFile(filepath, targetpath, forceOverwrite, false);
                }
            );
            if(!problemFileList.empty()) {
                _info() << QString("Found %1 files with problems").arg(problemFileList.count());
            }
            auto finalProblems = multiFileOperation(problemFileList,
                [](const QString & filepath) {
                    return QString("Retry copying %1").arg(filepath);
                },
                [](const QString & filepath) {
                    const auto targetpath = targetDirectory + filepath_ops::fileName(filepath);
                    return copyFile(filepath, targetpath, false, true);
                },
                true
            );
            _info() << QString("Copying completed (%1 file(s) copied)!")
                .arg(total - finalProblems.count());
            return 0;
        }
    }),
    std::make_pair("move", Task {
        [](QCommandLineParser & parser) {
            _debug() << "init move";
        },
        [](QCommandLineParser & parser) {
            _debug() << "move";
            return 0;
        }
    }),
    std::make_pair("rename", Task {
        [](QCommandLineParser & parser) {
            _debug() << "init rename";
        },
        [](QCommandLineParser & parser) {
            _debug() << "rename";
            return 0;
        }
    }),
    std::make_pair("group", Task {
        [](QCommandLineParser & parser) {
            _debug() << "init group";
        },
        [](QCommandLineParser & parser) {
            _debug() << "group";
            return 0;
        }
    }),
    std::make_pair("transform", Task {
        [](QCommandLineParser & parser) {
            _debug() << "init format";
        },
        [](QCommandLineParser & parser) {
            _debug() << "format";
            return 0;
        }
    })
});

void initParserAndLogging(const QCoreApplication & app, QCommandLineParser & parser) {
	parser.addVersionOption();
	parser.addHelpOption();
    parser.addOptions({quietFlag, verboseFlag, suppressWarningsFlag});
    parser.addPositionalArgument("task", QString("One of the following:\n%1")
        .arg(QStringList(tasks.keys()).join(", ")), "<task> [...]");
    parser.setApplicationDescription(QString("Welcome to %1 - your simple command line UPA.").arg(APP_NAME));
    if(app.arguments().count() <= 1) {
        // exit with error code 1 because the user didn't supply any arguments
        parser.showHelp(1);
    }

    parser.parse(app.arguments()); // ignore unknown options for now
    Logging::quiet = parser.isSet(quietFlag);
    Logging::verbose = parser.isSet(verboseFlag);
    if(Logging::quiet && Logging::verbose) {
        abnormalExit("Invalid arguments: --verbose cannot be set at the same time with --quiet");
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
            abnormalExit("Invalid arguments: --quiet suppresses output of --supported-formats");
        }
        showSupportedFormats();
        return 0;
    }

    QStringList args = parser.positionalArguments();
    if(args.empty()) {
        // exit with error code 1 because the user didn't supply any arguments
        parser.showHelp(1);
    }
    const auto taskName = args.first().toLower();
    const auto task = tasks.value(taskName, Task {
        [](QCommandLineParser & parser) {},
        [&taskName](QCommandLineParser & parser) {
            _warn() << QString("Unknown task: \"%1\"").arg(taskName);
            // exit with error code 2 for unknown task
            parser.showHelp(2);
            return 2;
        },
        true
    });
    if(!task.unknown) {
        _debug() << QString("Starting task \"%1\"").arg(taskName);
        task.parameterInitializer(parser);
	    parser.process(app);
    }
    return task.taskHandler(parser);
    // return app.exec();
}

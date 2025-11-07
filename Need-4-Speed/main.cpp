#include <QApplication>
#include <QCoreApplication>
#include <QLoggingCategory>
#include <QByteArray>
#include <QProcessEnvironment>
#include <QWindow>
#include <QPixmapCache>
#include "mainwindow.h"

#ifdef _WIN32
#include <io.h>
#include <fcntl.h>
#endif


static void quietMultimediaLogs(QtMsgType, const QMessageLogContext& ctx, const QString& msg) {
    const QByteArray cat(ctx.category);
    if (cat.startsWith("qt.multimedia"))
        return;
    if (msg.contains("[mp3 @") || msg.contains("mp3float") || msg.contains("FFmpeg"))
        return;
    QByteArray s = msg.toLocal8Bit();
    fwrite(s.constData(), 1, size_t(s.size()), stderr);
    fwrite("\n", 1, 1, stderr);
}

#ifdef _WIN32
class ScopedSilenceStderr {
    int saved = -1;
    int nulFd = -1;
public:
    ScopedSilenceStderr() {
        saved = _dup(_fileno(stderr));
        nulFd = _open("NUL", _O_WRONLY);
        if (nulFd >= 0) _dup2(nulFd, _fileno(stderr));
    }
    ~ScopedSilenceStderr() {
        if (saved >= 0) _dup2(saved, _fileno(stderr));
        if (nulFd >= 0) _close(nulFd);
        if (saved >= 0) _close(saved);
    }
};
#else
#include <unistd.h>
#include <fcntl.h>
class ScopedSilenceStderr {
    int saved = -1;
    int nulFd = -1;
public:
    ScopedSilenceStderr() {
        saved = dup(STDERR_FILENO);
        nulFd = open("/dev/null", O_WRONLY);
        if (nulFd >= 0) dup2(nulFd, STDERR_FILENO);
    }
    ~ScopedSilenceStderr() {
        if (saved >= 0) dup2(saved, STDERR_FILENO);
        if (nulFd >= 0) close(nulFd);
        if (saved >= 0) close(saved);
    }
};
#endif

int main(int argc, char *argv[]) {
    qputenv("QT_FFMPEG_LOG_LEVEL", "quiet");
    qputenv("QT_LOGGING_RULES", "qt.multimedia.ffmpeg.debug=false;qt.multimedia.*=false");
    qputenv("QT_NO_FFMPEG_LOG", "1");

    qInstallMessageHandler(quietMultimediaLogs);

    QApplication app(argc, argv);

    QObject::connect(&app, &QCoreApplication::aboutToQuit, []{
        static ScopedSilenceStderr s;
    });

    QPixmapCache::setCacheLimit(128 * 4096);
    MainWindow w;
    w.show();

    return app.exec();
}//

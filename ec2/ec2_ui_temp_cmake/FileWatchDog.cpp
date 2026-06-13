#include "FileWatchDog.h"

#include <QDir>
#include <QFileInfo>

FileWatchDog::FileWatchDog(const QString &watchDir,
                           int stableChecks,
                           int scanIntervalMs,
                           QObject *parent)
    : QObject(parent)
    , m_watchDir(watchDir)
    , m_stableChecks(stableChecks)
{
    // 确保目录存在
    QDir dir(m_watchDir);
    if (!dir.exists()) {
        dir.mkpath(QStringLiteral("."));
    }

    // 文件系统监控：目录内有文件创建/移动时触发扫描
    m_dirWatcher = new QFileSystemWatcher(this);
    m_dirWatcher->addPath(m_watchDir);
    connect(m_dirWatcher, &QFileSystemWatcher::directoryChanged,
            this, &FileWatchDog::onDirChanged);

    // 周期扫描定时器：兜底机制，确保即使 inotify 丢失事件也能检测到
    m_scanTimer = new QTimer(this);
    m_scanTimer->setSingleShot(false);
    connect(m_scanTimer, &QTimer::timeout,
            this, &FileWatchDog::onScanTick);
    m_scanTimer->start(scanIntervalMs);

    // 启动时立即扫描一次，处理目录中已有的文件
    scanDirectory();
}

FileWatchDog::~FileWatchDog()
{
    if (m_scanTimer) {
        m_scanTimer->stop();
    }
}

void FileWatchDog::onDirChanged(const QString &path)
{
    Q_UNUSED(path);
    scanDirectory();
}

void FileWatchDog::onScanTick()
{
    scanDirectory();
}

void FileWatchDog::scanDirectory()
{
    QDir dir(m_watchDir);
    if (!dir.exists()) {
        return;
    }

    // 获取目录中所有常规文件（排除隐藏文件）
    const QFileInfoList entries = dir.entryInfoList(QDir::Files | QDir::NoDotAndDotDot);

    // ---- 1) 检查已有跟踪文件的大小变化 ----
    for (auto it = m_trackedFiles.begin(); it != m_trackedFiles.end(); ) {
        const QString &fileName = it.key();
        QFileInfo fi(m_watchDir + QStringLiteral("/") + fileName);

        if (!fi.exists()) {
            // 文件被移走或删除，停止跟踪
            it = m_trackedFiles.erase(it);
            continue;
        }

        const qint64 currentSize = fi.size();
        QPair<qint64, int> &tracked = it.value();

        if (currentSize == tracked.first && currentSize > 0) {
            // 大小不变，计数 +1
            tracked.second++;
            if (tracked.second >= m_stableChecks) {
                // 文件写入完成，发出信号
                m_processedFiles.insert(fileName);
                emit fileReady(fi.absoluteFilePath(), fileName);
                it = m_trackedFiles.erase(it);
                continue;
            }
        } else {
            // 大小变化，重置计数
            tracked.first = currentSize;
            tracked.second = 0;
        }
        ++it;
    }

    // ---- 2) 发现新文件，开始跟踪 ----
    for (const QFileInfo &fi : entries) {
        const QString fileName = fi.fileName();

        // 跳过已处理的文件
        if (m_processedFiles.contains(fileName)) {
            continue;
        }

        // 跳过已在跟踪的文件
        if (m_trackedFiles.contains(fileName)) {
            continue;
        }

        // 跳过隐藏文件
        if (fileName.startsWith(QLatin1Char('.'))) {
            continue;
        }

        // 开始跟踪：记录当前大小，计数为 0
        m_trackedFiles.insert(fileName, qMakePair(fi.size(), 0));
    }
}

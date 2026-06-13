#ifndef FILEWATCHDOG_H
#define FILEWATCHDOG_H

#include <QObject>
#include <QTimer>
#include <QFileSystemWatcher>
#include <QSet>
#include <QMap>
#include <QString>
#include <QPair>

/**
 * FileWatchDog - 文件目录监控器
 *
 * 监控指定目录中的新文件，当文件写入完成后（大小连续多次扫描不变）发出 fileReady 信号。
 * 用于无人机端自动发送场景：将文件放入 FileSend 目录，写入完成后自动触发发送。
 */
class FileWatchDog : public QObject
{
    Q_OBJECT

public:
    /**
     * @param watchDir      要监控的目录路径
     * @param stableChecks  文件大小连续不变的扫描次数，达到后判定写入完成（默认 2）
     * @param scanIntervalMs 扫描间隔毫秒（默认 2000）
     * @param parent
     */
    explicit FileWatchDog(const QString &watchDir,
                          int stableChecks = 2,
                          int scanIntervalMs = 2000,
                          QObject *parent = nullptr);
    ~FileWatchDog();

signals:
    /** 文件写入完成，可以发送 */
    void fileReady(const QString &absolutePath, const QString &fileName);

private slots:
    void onScanTick();
    void onDirChanged(const QString &path);

private:
    void scanDirectory();

    QFileSystemWatcher *m_dirWatcher = nullptr;
    QTimer             *m_scanTimer  = nullptr;
    QString             m_watchDir;
    int                 m_stableChecks;

    /** 已发出 fileReady 信号的文件名集合，避免重复发送 */
    QSet<QString> m_processedFiles;

    /** 正在跟踪的文件：fileName → {最近一次文件大小, 连续不变计数} */
    QMap<QString, QPair<qint64, int>> m_trackedFiles;
};

#endif // FILEWATCHDOG_H

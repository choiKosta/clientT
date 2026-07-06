#pragma once

#include <QImage>
#include <QMutex>
#include <QThread>

#include <atomic>

class VideoWorker : public QThread {
    Q_OBJECT

public:
    explicit VideoWorker(QObject *parent = nullptr);
    ~VideoWorker() override;

    void startStreaming(const QString &streamUrl);
    void stopStreaming();

signals:
    void frameReady(const QImage &image);
    void statusChanged(const QString &message);
    void streamError(const QString &message);

protected:
    void run() override;

private:
    QString currentUrl() const;
    bool shouldKeepRunning() const;

    mutable QMutex mutex_;
    QString streamUrl_;
    std::atomic<bool> keepRunning_{false};
    std::atomic<bool> restartRequested_{false};
};
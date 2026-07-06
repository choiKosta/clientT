#include "VideoWorker.h"

#include <QMutexLocker>

#include <opencv2/imgproc.hpp>
#include <opencv2/videoio.hpp>

#include <chrono>
#include <thread>

namespace {
QImage matToImage(const cv::Mat &frame) {
    cv::Mat rgbFrame;
    cv::cvtColor(frame, rgbFrame, cv::COLOR_BGR2RGB);
    return QImage(rgbFrame.data,
                  rgbFrame.cols,
                  rgbFrame.rows,
                  static_cast<qsizetype>(rgbFrame.step),
                  QImage::Format_RGB888)
        .copy();
}
}

VideoWorker::VideoWorker(QObject *parent)
    : QThread(parent) {
}

VideoWorker::~VideoWorker() {
    stopStreaming();
}

void VideoWorker::startStreaming(const QString &streamUrl) {
    {
        QMutexLocker locker(&mutex_);
        streamUrl_ = streamUrl;
    }
    restartRequested_.store(true);
    keepRunning_.store(true);

    if (!isRunning()) {
        start();
    }
}

void VideoWorker::stopStreaming() {
    keepRunning_.store(false);
    restartRequested_.store(true);
    wait();
}

void VideoWorker::run() {
    while (shouldKeepRunning()) {
        const QString url = currentUrl();
        if (url.isEmpty()) {
            emit statusChanged(QStringLiteral("RTSP URL is empty."));
            return;
        }

        restartRequested_.store(false);
        emit statusChanged(QStringLiteral("Opening stream: %1").arg(url));

        cv::VideoCapture capture;
        capture.set(cv::CAP_PROP_BUFFERSIZE, 1);

        if (!capture.open(url.toStdString(), cv::CAP_FFMPEG)) {
            if (!capture.open(url.toStdString())) {
                emit streamError(QStringLiteral("RTSP connection failed. Retrying in 3 seconds."));
                for (int retryStep = 0; retryStep < 30 && shouldKeepRunning() && !restartRequested_.load(); ++retryStep) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(100));
                }
                continue;
            }
        }

        emit statusChanged(QStringLiteral("Streaming started."));

        cv::Mat frame;
        while (shouldKeepRunning() && !restartRequested_.load()) {
            if (!capture.read(frame) || frame.empty()) {
                emit streamError(QStringLiteral("Stream disconnected or timed out. Retrying in 3 seconds."));
                break;
            }

            emit frameReady(matToImage(frame));
            msleep(10);
        }

        capture.release();

        if (shouldKeepRunning() && !restartRequested_.load()) {
            for (int retryStep = 0; retryStep < 30 && shouldKeepRunning() && !restartRequested_.load(); ++retryStep) {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
        }
    }

    emit statusChanged(QStringLiteral("Streaming stopped."));
}

QString VideoWorker::currentUrl() const {
    QMutexLocker locker(&mutex_);
    return streamUrl_;
}

bool VideoWorker::shouldKeepRunning() const {
    return keepRunning_.load();
}
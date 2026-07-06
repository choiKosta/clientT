#pragma once

#include <QNetworkAccessManager>
#include <QJsonObject>
#include <QObject>
#include <QStringList>
#include <QUrl>

#include <functional>

struct VideoSettings {
    QString resolution;
    int frameRate = 30;
    QString codec = QStringLiteral("H.264");
};

struct SessionInfo {
    QString sessionId;
    QUrl rtspUrl;
    VideoSettings video;
    QString createdAt;
    QString state;
};

class ServerApiClient : public QObject {
    Q_OBJECT

public:
    explicit ServerApiClient(QObject *parent = nullptr);

    void createSession(const QUrl &baseUrl,
                       const QString &clientName,
                       const VideoSettings &requestedVideo,
                       std::function<void(const SessionInfo &)> onSuccess,
                       std::function<void(const QString &)> onError);

    void updateVideo(const QUrl &baseUrl,
                     const QString &sessionId,
                     const VideoSettings &video,
                     std::function<void(const VideoSettings &)> onSuccess,
                     std::function<void(const QString &)> onError);

    void getSessionStatus(const QUrl &baseUrl,
                          const QString &sessionId,
                          std::function<void(const SessionInfo &)> onSuccess,
                          std::function<void(const QString &)> onError);

    void getVideoDevices(const QUrl &baseUrl,
                         std::function<void(const QStringList &)> onSuccess,
                         std::function<void(const QString &)> onError);

    void selectCamera(const QUrl &baseUrl,
                      const QString &device,
                      std::function<void()> onSuccess,
                      std::function<void(const QString &)> onError);

    void deleteSession(const QUrl &baseUrl,
                       const QString &sessionId,
                       std::function<void()> onSuccess,
                       std::function<void(const QString &)> onError);

    void echo(const QUrl &baseUrl,
              const QString &sessionId,
              const QString &message,
              std::function<void(const QString &)> onSuccess,
              std::function<void(const QString &)> onError);

private:
    QUrl apiUrl(const QUrl &baseUrl, const QString &path) const;
    QString parseErrorMessage(const QByteArray &payload, const QString &fallback) const;
    VideoSettings parseVideoSettings(const QJsonObject &object) const;

    QNetworkAccessManager networkManager_;
};
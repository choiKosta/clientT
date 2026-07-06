#include "ServerApiClient.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QNetworkRequest>

ServerApiClient::ServerApiClient(QObject *parent)
    : QObject(parent) {
}

void ServerApiClient::createSession(const QUrl &baseUrl,
                                    const QString &clientName,
                                    const VideoSettings &requestedVideo,
                                    std::function<void(const SessionInfo &)> onSuccess,
                                    std::function<void(const QString &)> onError) {
    QJsonObject payload{
        {QStringLiteral("clientName"), clientName},
        {QStringLiteral("video"), QJsonObject{
            {QStringLiteral("resolution"), requestedVideo.resolution},
            {QStringLiteral("frameRate"), requestedVideo.frameRate}
        }}
    };

    QNetworkRequest request(apiUrl(baseUrl, QStringLiteral("/api/v1/session")));
    request.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json; charset=UTF-8"));

    QNetworkReply *reply = networkManager_.post(request, QJsonDocument(payload).toJson(QJsonDocument::Compact));
    connect(reply, &QNetworkReply::finished, this, [this, reply, onSuccess = std::move(onSuccess), onError = std::move(onError)]() mutable {
        const QByteArray payload = reply->readAll();
        const int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

        if (reply->error() != QNetworkReply::NoError || statusCode != 201) {
            onError(parseErrorMessage(payload, reply->errorString()));
            reply->deleteLater();
            return;
        }

        const QJsonObject root = QJsonDocument::fromJson(payload).object();
        SessionInfo sessionInfo;
        sessionInfo.sessionId = root.value(QStringLiteral("sessionId")).toString();
        sessionInfo.rtspUrl = QUrl(root.value(QStringLiteral("rtspUrl")).toString());
        sessionInfo.video = parseVideoSettings(root.value(QStringLiteral("video")).toObject());
        sessionInfo.createdAt = root.value(QStringLiteral("createdAt")).toString();
        sessionInfo.state = root.value(QStringLiteral("state")).toString(QStringLiteral("CONNECTED"));
        onSuccess(sessionInfo);
        reply->deleteLater();
    });
}

void ServerApiClient::updateVideo(const QUrl &baseUrl,
                                  const QString &sessionId,
                                  const VideoSettings &video,
                                  std::function<void(const VideoSettings &)> onSuccess,
                                  std::function<void(const QString &)> onError) {
    QJsonObject payload{
        {QStringLiteral("resolution"), video.resolution},
        {QStringLiteral("frameRate"), video.frameRate}
    };

    const QString path = QStringLiteral("/api/v1/session/%1/video").arg(sessionId);
    QNetworkRequest request(apiUrl(baseUrl, path));
    request.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json; charset=UTF-8"));

    QNetworkReply *reply = networkManager_.sendCustomRequest(
        request,
        QByteArrayLiteral("PATCH"),
        QJsonDocument(payload).toJson(QJsonDocument::Compact));

    connect(reply, &QNetworkReply::finished, this, [this, reply, onSuccess = std::move(onSuccess), onError = std::move(onError)]() mutable {
        const QByteArray payload = reply->readAll();
        const int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

        if (reply->error() != QNetworkReply::NoError || statusCode != 200) {
            onError(parseErrorMessage(payload, reply->errorString()));
            reply->deleteLater();
            return;
        }

        const QJsonObject root = QJsonDocument::fromJson(payload).object();
        onSuccess(parseVideoSettings(root.value(QStringLiteral("video")).toObject()));
        reply->deleteLater();
    });
}

void ServerApiClient::getSessionStatus(const QUrl &baseUrl,
                                       const QString &sessionId,
                                       std::function<void(const SessionInfo &)> onSuccess,
                                       std::function<void(const QString &)> onError) {
    const QString path = QStringLiteral("/api/v1/session/%1").arg(sessionId);
    QNetworkRequest request(apiUrl(baseUrl, path));
    request.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json; charset=UTF-8"));

    QNetworkReply *reply = networkManager_.get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply, onSuccess = std::move(onSuccess), onError = std::move(onError)]() mutable {
        const QByteArray payload = reply->readAll();
        const int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

        if (reply->error() != QNetworkReply::NoError || statusCode != 200) {
            onError(parseErrorMessage(payload, reply->errorString()));
            reply->deleteLater();
            return;
        }

        const QJsonObject root = QJsonDocument::fromJson(payload).object();
        SessionInfo sessionInfo;
        sessionInfo.sessionId = root.value(QStringLiteral("sessionId")).toString();
        sessionInfo.state = root.value(QStringLiteral("state")).toString(QStringLiteral("CONNECTED"));
        sessionInfo.video = parseVideoSettings(root.value(QStringLiteral("video")).toObject());
        onSuccess(sessionInfo);
        reply->deleteLater();
    });
}

void ServerApiClient::deleteSession(const QUrl &baseUrl,
                                    const QString &sessionId,
                                    std::function<void()> onSuccess,
                                    std::function<void(const QString &)> onError) {
    const QString path = QStringLiteral("/api/v1/session/%1").arg(sessionId);
    QNetworkRequest request(apiUrl(baseUrl, path));
    request.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json; charset=UTF-8"));

    QNetworkReply *reply = networkManager_.deleteResource(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply, onSuccess = std::move(onSuccess), onError = std::move(onError)]() mutable {
        const QByteArray payload = reply->readAll();
        const int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

        if (reply->error() != QNetworkReply::NoError || statusCode != 204) {
            onError(parseErrorMessage(payload, reply->errorString()));
            reply->deleteLater();
            return;
        }

        onSuccess();
        reply->deleteLater();
    });
}

void ServerApiClient::getVideoDevices(const QUrl &baseUrl,
                                      std::function<void(const QStringList &)> onSuccess,
                                      std::function<void(const QString &)> onError) {
    QNetworkRequest request(apiUrl(baseUrl, QStringLiteral("/api/v1/stream/devices")));
    request.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json; charset=UTF-8"));

    QNetworkReply *reply = networkManager_.get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply, onSuccess = std::move(onSuccess), onError = std::move(onError)]() mutable {
        const QByteArray payload = reply->readAll();
        const int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

        if (reply->error() != QNetworkReply::NoError || statusCode != 200) {
            onError(parseErrorMessage(payload, reply->errorString()));
            reply->deleteLater();
            return;
        }

        const QJsonObject root = QJsonDocument::fromJson(payload).object();
        const QJsonArray devicesJson = root.value(QStringLiteral("videoDevices")).toArray();
        QStringList devices;
        devices.reserve(devicesJson.size());
        for (const QJsonValue &value : devicesJson) {
            const QString device = value.toString().trimmed();
            if (!device.isEmpty()) {
                devices.append(device);
            }
        }

        onSuccess(devices);
        reply->deleteLater();
    });
}

void ServerApiClient::selectCamera(const QUrl &baseUrl,
                                   const QString &device,
                                   std::function<void()> onSuccess,
                                   std::function<void(const QString &)> onError) {
    QNetworkRequest request(apiUrl(baseUrl, QStringLiteral("/api/v1/stream/camera")));
    request.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json; charset=UTF-8"));

    const QJsonObject payload{{QStringLiteral("device"), device}};
    QNetworkReply *reply = networkManager_.sendCustomRequest(
        request,
        QByteArrayLiteral("PUT"),
        QJsonDocument(payload).toJson(QJsonDocument::Compact));

    connect(reply, &QNetworkReply::finished, this, [this, reply, onSuccess = std::move(onSuccess), onError = std::move(onError)]() mutable {
        const QByteArray payload = reply->readAll();
        const int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

        if (reply->error() != QNetworkReply::NoError || statusCode != 200) {
            onError(parseErrorMessage(payload, reply->errorString()));
            reply->deleteLater();
            return;
        }

        onSuccess();
        reply->deleteLater();
    });
}

void ServerApiClient::echo(const QUrl &baseUrl,
                           const QString &sessionId,
                           const QString &message,
                           std::function<void(const QString &)> onSuccess,
                           std::function<void(const QString &)> onError) {
    const QString path = QStringLiteral("/api/v1/session/%1/echo").arg(sessionId);
    QNetworkRequest request(apiUrl(baseUrl, path));
    request.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json; charset=UTF-8"));

    const QJsonObject payload{{QStringLiteral("message"), message}};
    QNetworkReply *reply = networkManager_.post(request, QJsonDocument(payload).toJson(QJsonDocument::Compact));

    connect(reply, &QNetworkReply::finished, this, [this, reply, onSuccess = std::move(onSuccess), onError = std::move(onError)]() mutable {
        const QByteArray payload = reply->readAll();
        const int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

        if (reply->error() != QNetworkReply::NoError || statusCode != 200) {
            onError(parseErrorMessage(payload, reply->errorString()));
            reply->deleteLater();
            return;
        }

        const QJsonObject root = QJsonDocument::fromJson(payload).object();
        onSuccess(root.value(QStringLiteral("message")).toString());
        reply->deleteLater();
    });
}

QUrl ServerApiClient::apiUrl(const QUrl &baseUrl, const QString &path) const {
    QUrl url(baseUrl);
    url.setPath(path);
    return url;
}

QString ServerApiClient::parseErrorMessage(const QByteArray &payload, const QString &fallback) const {
    const QJsonDocument document = QJsonDocument::fromJson(payload);
    if (document.isObject()) {
        const QJsonObject errorObject = document.object().value(QStringLiteral("error")).toObject();
        const QString code = errorObject.value(QStringLiteral("code")).toString();
        const QString message = errorObject.value(QStringLiteral("message")).toString();
        if (!message.isEmpty()) {
            if (!code.isEmpty()) {
                return QStringLiteral("%1: %2").arg(code, message);
            }
            return message;
        }
    }
    return fallback;
}

VideoSettings ServerApiClient::parseVideoSettings(const QJsonObject &object) const {
    VideoSettings settings;
    settings.resolution = object.value(QStringLiteral("resolution")).toString();
    settings.frameRate = object.value(QStringLiteral("frameRate")).toInt(30);
    settings.codec = object.value(QStringLiteral("codec")).toString(QStringLiteral("H.264"));
    return settings;
}
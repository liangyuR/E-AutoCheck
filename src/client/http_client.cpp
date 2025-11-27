#include "client/http_client.h"

#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QDebug>

namespace client{
HttpClient::HttpClient(QObject* parent)
    : QObject(parent)
{
}

void HttpClient::setBaseUrl(const QUrl& baseUrl)
{
    baseUrl_ = baseUrl;
}

QUrl HttpClient::buildUrl(const QString& path) const
{
    if (!baseUrl_.isValid()) {
        return QUrl{path};
    }

    // 使用 resolved 自动处理斜杠
    return baseUrl_.resolved(QUrl(path));
}

void HttpClient::getJson(const QString& path, const QString& requestId)
{
    const QUrl url = buildUrl(path);
    QNetworkRequest req(url);
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QNetworkReply* reply = nam_.get(req);

    PendingInfo info;
    info.requestId = requestId;
    info.method = QStringLiteral("GET");
    info.path = path;
    pending_.insert(reply, info);

    connect(reply, &QNetworkReply::finished,
            this, &HttpClient::onReplyFinished);
}

void HttpClient::postJson(const QString& path,
                          const QJsonObject& payload,
                          const QString& requestId)
{
    const QUrl url = buildUrl(path);
    QNetworkRequest req(url);
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    const QJsonDocument doc(payload);
    const QByteArray body = doc.toJson(QJsonDocument::Compact);

    QNetworkReply* reply = nam_.post(req, body);

    PendingInfo info;
    info.requestId = requestId;
    info.method = QStringLiteral("POST");
    info.path = path;
    pending_.insert(reply, info);

    connect(reply, &QNetworkReply::finished,
            this, &HttpClient::onReplyFinished);
}

void HttpClient::onReplyFinished()
{
    auto* reply = qobject_cast<QNetworkReply*>(sender());
    if (reply == nullptr) {
        return;
    }

    const auto iter = pending_.find(reply);
    PendingInfo info;
    if (iter != pending_.end()) {
        info = iter.value();
        pending_.erase(iter);
    }

    // 先拿 HTTP 状态码
    const QVariant statusAttr = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute);
    const int httpStatus = statusAttr.isValid() ? statusAttr.toInt() : 0;

    // 网络层错误（连不上、超时等）
    if (reply->error() != QNetworkReply::NoError) {
        const QString err = reply->errorString();
        emit requestFailed(info.requestId, httpStatus, err);
        reply->deleteLater();
        return;
    }

    // 读取 body
    const QByteArray data = reply->readAll();
    reply->deleteLater();

    if (data.isEmpty()) {
        // 没 body，当空 JSON
        emit requestSucceeded(info.requestId, httpStatus, QJsonObject{});
        return;
    }

    QJsonParseError parseErr;
    const QJsonDocument doc = QJsonDocument::fromJson(data, &parseErr);
    if (parseErr.error != QJsonParseError::NoError || !doc.isObject()) {
        const QString err = QStringLiteral("JSON parse error: %1")
                                .arg(parseErr.errorString());
        emit requestFailed(info.requestId, httpStatus, err);
        return;
    }

    const QJsonObject obj = doc.object();

    // 如果你希望 2xx 才算 success，可以这样改：
    // if (httpStatus < 200 || httpStatus >= 300) {
    //     emit requestFailed(info.requestId, httpStatus,
    //                        QStringLiteral("HTTP error, status=%1").arg(httpStatus));
    // } else {
    //     emit requestSucceeded(info.requestId, httpStatus, obj);
    // }

    // 当前策略：只要网络 OK 且 JSON OK，就给 success，由业务判断 httpStatus
    emit requestSucceeded(info.requestId, httpStatus, obj);
}
}
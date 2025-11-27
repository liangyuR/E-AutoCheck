#pragma once

#include <QObject>
#include <QNetworkAccessManager>
#include <QUrl>
#include <QJsonObject>

class QNetworkReply;

class HttpClient : public QObject {
    Q_OBJECT
    
public:
    explicit HttpClient(QObject* parent = nullptr);

    void setBaseUrl(const QUrl& baseUrl);
    QUrl baseUrl() const { return baseUrl_; }

    void getJson(const QString& path, const QString& requestId = {});
    void postJson(const QString& path, const QJsonObject& payload, const QString& requestId = {});

private slots:
    void onReplyFinished();

signals:
    void requestSucceeded(const QString& requestId,
                          int httpStatus,
                          const QJsonObject& json);

    void requestFailed(const QString& requestId,
                       int httpStatus,
                       const QString& errorString);


private:

    struct PendingInfo {
        QString requestId;
        QString method;
        QString path;
    };

    QNetworkAccessManager nam_;
    QUrl baseUrl_;
    QHash<QNetworkReply*, PendingInfo> pending_;

    QUrl buildUrl(const QString& path) const;
};

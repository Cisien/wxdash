#pragma once

#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QObject>
#include <QTimer>
#include <QUrl>

/**
 * JsonPoller — abstract base class for HTTP JSON pollers.
 *
 * Provides the common start/poll/reply lifecycle:
 *   - start(): create QNAM + QTimer, fire first poll, start timer
 *   - poll(): abort pending reply, issue GET, connect finished->onReply
 *   - onReply(): check error, call handleResponse()
 *
 * Subclasses override handleResponse() to parse and emit typed signals.
 * Optional overrides: transferTimeoutMs(), configureRequest(), validateReply().
 *
 * Must be moved to a QThread before start() is called.
 */
class JsonPoller : public QObject {
    Q_OBJECT

public:
    explicit JsonPoller(const QUrl &url, int pollIntervalMs, QObject *parent = nullptr);

    void setVerbose(bool v) { m_verbose = v; }

    static constexpr int kMaxConsecutiveErrors = 5;
    static constexpr int kMaxRetries = 3;
    static constexpr int kRetryDelayMs = 500;

public slots:
    void start();

protected:
    virtual int transferTimeoutMs() const { return 5000; }
    virtual void configureRequest(QNetworkRequest &req) { Q_UNUSED(req); }
    virtual bool validateReply(QNetworkReply *reply) { Q_UNUSED(reply); return true; }
    virtual void handleResponse(const QByteArray &data) = 0;

private slots:
    void poll();
    void onReply();
    void retry();

private:
    void sendRequest();
    void resetNam();

    QUrl m_url;
    int m_pollIntervalMs;
    QNetworkAccessManager *m_nam = nullptr;
    QTimer *m_pollTimer = nullptr;
    QNetworkReply *m_pendingReply = nullptr;
    int m_consecutiveErrors = 0;
    int m_retryCount = 0;
    bool m_verbose = false;
};

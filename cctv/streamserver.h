#ifndef STREAMSERVER_H
#define STREAMSERVER_H

#include <QObject>
#include <QList>
#include <QImage>

class QWebSocketServer;
class QWebSocket;

class StreamServer : public QObject
{
    Q_OBJECT
public:
    explicit StreamServer(quint16 port, QObject *parent = nullptr);
    ~StreamServer();

public slots:
    // MotionDetector로부터 새로운 프레임을 받을 슬롯
    void onNewFrame(const QImage &frame);

private slots:
    // 새로운 웹 클라이언트가 접속했을 때
    void onNewConnection();
    // 웹 클라이언트의 연결이 끊어졌을 때
    void onSocketDisconnected();

private:
    QWebSocketServer* m_server;
    QList<QWebSocket*> m_clients;
};

#endif // STREAMSERVER_H

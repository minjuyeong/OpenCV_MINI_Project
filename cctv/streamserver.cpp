#include "streamserver.h"
#include <QWebSocketServer>
#include <QWebSocket>
#include <QBuffer>
#include <QDebug>

StreamServer::StreamServer(quint16 port, QObject *parent) : QObject(parent)
{
    m_server = new QWebSocketServer("CCTV Stream Server", QWebSocketServer::NonSecureMode, this);
    if (m_server->listen(QHostAddress::Any, port)) {
        qDebug() << "Stream server listening on port" << port;
        connect(m_server, &QWebSocketServer::newConnection, this, &StreamServer::onNewConnection);
    } else {
        qWarning() << "Failed to start stream server on port" << port;
    }
}

StreamServer::~StreamServer()
{
    m_server->close();
    qDeleteAll(m_clients.begin(), m_clients.end());
}

void StreamServer::onNewFrame(const QImage &frame)
{
    if (m_clients.isEmpty()) return; // 접속한 클라이언트가 없으면 아무것도 안 함

    QByteArray byteArray;
    QBuffer buffer(&byteArray);
    buffer.open(QIODevice::WriteOnly);
    // 프레임을 JPEG 형식, 70% 품질로 압축하여 용량 줄이기
    frame.save(&buffer, "JPEG", 70);

    // 연결된 모든 클라이언트에게 압축된 이미지 데이터 전송
    for (QWebSocket *client : m_clients) {
        client->sendBinaryMessage(byteArray);
    }
}

void StreamServer::onNewConnection()
{
    QWebSocket *client = m_server->nextPendingConnection();
    if (client) {
        qDebug() << "New client connected:" << client->peerAddress().toString();
        connect(client, &QWebSocket::disconnected, this, &StreamServer::onSocketDisconnected);
        m_clients << client;
    }
}

void StreamServer::onSocketDisconnected()
{
    QWebSocket *client = qobject_cast<QWebSocket *>(sender());
    if (client) {
        qDebug() << "Client disconnected:" << client->peerAddress().toString();
        m_clients.removeAll(client);
        client->deleteLater();
    }
}

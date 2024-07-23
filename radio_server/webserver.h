#ifndef WEBSERVER_H
#define WEBSERVER_H

#include <QtCore/QList>
#include <QtCore/QObject>
#include "clientai.h"
#include "radar_core.h"

#include <QHash>


QT_FORWARD_DECLARE_CLASS(QWebSocketServer)
QT_FORWARD_DECLARE_CLASS(QWebSocket)
QT_FORWARD_DECLARE_CLASS(QString)

class WebServer : public QObject {
  Q_OBJECT
public:
  explicit WebServer(quint16 port, QObject *parent = nullptr);
  ~WebServer() override;
  void handlerCmd(QString command, QJsonArray *params, QWebSocket *pSender);
  void setCmd(QString &message, QWebSocket *pSender, int &cmd_id);
  void loadRadarData(QJsonDocument &doc, QJsonObject &jsonObject, QWebSocket *pSender);
  void userVerification(QString login, QString password, QWebSocket *pSender);
  bool task_kill(QWebSocket *pSender);
  QString GetRandomString();
  uint GetCoreID(QJsonDocument &doc);

private slots:
  void onNewConnection();
  void socketDisconnected();
  void processMessage(const QString &message);
  void kill_task(QWebSocket *pSender);
  //  void processBinaryMessage(QByteArray message);

public slots:
  void send_calc_radar_result(QString doc, QWebSocket *Client);
  void message_calc_radar(QString doc, QWebSocket *Client);

signals:
  void pause();

private:
  bool SAVE_INPUT_DATA_TO_FILES;
  bool SAVE_MODEL;
  bool SAVE_SCAT_FIELD;
  bool SAVE_FFT_FIELD;
  bool SERIALIZE_MODEL;
  bool READ_RESULT;
  bool SAVE_MESSAGE_RESULT;
  QWebSocketServer *m_pWebSocketServer;
  QList<QWebSocket *> m_clients;
  QList<clientAI *> m_client_list;
  QList<radarCore *> task_list;
};

#endif // SERVER_H

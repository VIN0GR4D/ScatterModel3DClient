#ifndef RADAR_CORE_H
#define RADAR_CORE_H

#include <QObject>
#include <QDebug>
#include <QTest>
#include "clientai.h"
#include <QJsonDocument>
#include <QJsonObject>
#include "Calc_Radar/CulcRadar.h"

class radarCore : public culcradar
{
   Q_OBJECT
public:
    explicit radarCore():
        id(0), SAVE_MESSAGE_TO_FILE(0), model_id(0), m_running(0), RUN(1),
        TEST(0), comm(1), m_clientRadar(0), m_Client(0){}
    ~radarCore();
public:
    int id;
    bool SAVE_MESSAGE_TO_FILE;
private:
   uint model_id;
   bool m_running;
   bool RUN;
   bool TEST;
   int comm;
   clientAI *m_clientRadar;
   QWebSocket *m_Client;
   QJsonDocument m_doc;

public slots:
    void run();
    void sendText();     //method of sending text messages
    void sendProgressBar();
    void pause_core();
    void stop();

signals:
  void finished();
  void task_kill(QWebSocket *Client);
  void send_text(QString doc, QWebSocket *Client);
  void send_progress_bar (QString doc, QWebSocket *Client);
  void send_result (QString doc, QWebSocket *Client);

public:
  bool isRunning(){return m_running;}
  void setModelId(uint id) {model_id = id;}
  void setRunning(bool running) {m_running = running;}
  void setRadarParam(QJsonDocument &doc, clientAI *client, QWebSocket *pClient);
  clientAI* getClientRadar() {return m_clientRadar;}
  uint getModelId() {return model_id;}

  void continue_core();
  void testing(bool test);

  void parseJSONtoRadar(QHash<uint, node> &Node, QHash<uint,edge> &Edge);
  void calcRadar();
  void calcRadarResult();
};

#endif // RADAR_CORE_H

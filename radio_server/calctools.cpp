#include "calctools.h"


void clogs(QString msg, QString type, QString Source) {
  QTextStream(stdout);
  QDate cd = QDate::currentDate(); // ?????????? ??????? ????
  QTime ct = QTime::currentTime();
  QString typeMsg = "";

  if (type == "err")
    typeMsg = "<ERROR> ";
  if (type == "wrn")
    typeMsg = "<WARNING> ";
  if (Source == "")
    Source = "сервер";
  Source = " (" + Source + ") ";

  QString logmsg = "[" + cd.toString("dd.MM.yyyy") + " " +
                   ct.toString("hh.mm.ss") + "] " + typeMsg + Source + msg +
                   "\n";
  QTextStream out(m_logFile.data());
  // ?????????? ???? ??????
  out << logmsg;
  out.flush();

  QTextStream(stdout) << logmsg;

};

void webServerAnswer(QString answer, QWebSocket *pClient) {
  QJsonObject Echo;
  Echo.insert("type", QJsonValue::fromVariant("answer"));
  Echo.insert("msg", QJsonValue::fromVariant(answer));
  QJsonDocument doc(Echo);
  pClient->sendTextMessage(doc.toJson());
}

#include "webserver.h"
#include <QtWebSockets>
#include "calctools.h"
#include "radar_thread.h"


//конструктор
WebServer::WebServer(quint16 port, QObject *parent)
    : QObject(parent), m_pWebSocketServer(new QWebSocketServer(
                           QStringLiteral("Computing Server"),
                           QWebSocketServer::NonSecureMode, this)) {
  if (m_pWebSocketServer->listen(QHostAddress::Any, port)) {
    clogs("старт сервера", "", "");
    clogs("версия приложения 1.0", "", "");
    clogs("АО НЦПЭ 2022, все права защищены", "", "");
    clogs("идет прослушивание порта " + QString::number(port), "", "");

    connect(m_pWebSocketServer, &QWebSocketServer::newConnection, this,
            &WebServer::onNewConnection);

    SAVE_INPUT_DATA_TO_FILES = false; //признак записи ИД от КП в файл
    SAVE_MODEL = false;               //признак записи модели в файл
    SERIALIZE_MODEL = false;          //признак сериализации модели в бинарный файл
    SAVE_SCAT_FIELD = false;          //признак записи рассеянного поля в файл
    SAVE_FFT_FIELD = false;           //признак записи рассеянного поля после fft в файл
    READ_RESULT = false;              //признак чтения бинарного файла с результатами поля рассеяния
    SAVE_MESSAGE_RESULT = false;      //признак записи сформированных сообщений с результатами в файл
  }
}

//деструктор
WebServer::~WebServer() {
    m_pWebSocketServer->close();

}

//создание нового подключения
void WebServer::onNewConnection() {
  auto pSocket = m_pWebSocketServer->nextPendingConnection();
  if (!m_clients.contains(pSocket)) {

    QDate cd = QDate::currentDate();
    QTime ct = QTime::currentTime();

    clientAI *newClient = new clientAI;
    QVariant var((uint64_t)newClient);
    pSocket->setProperty("client_info", var);

    newClient->id = QString(this->GetRandomString());
    newClient->dateConnect = QDate(cd);
    newClient->timeConnect = QTime(ct);
    newClient->id = QString(this->GetRandomString());
    newClient->pClient = pSocket;

    QTimer *tmr;
    tmr = new QTimer;
    tmr->setSingleShot(true);
    tmr->setInterval(1000);
    connect(tmr, &QTimer::timeout, newClient, &clientAI::auth);
    tmr->start();

    //Включение режима тестирования, если имеется соотв. файл
    QFile file0("testing.json");
    clientAI *clientInfo =
        (clientAI *)newClient->pClient->property("client_info").toULongLong();
    if(!file0.open(QIODevice::ReadOnly)) {
        clogs("автоматический режим [" + clientInfo->id + "] включен","","");
    } else {
        clogs("режим тестирования [" + clientInfo->id + "] включен","","");
    }
    file0.close();

    clogs("новый сеанс подключения [" + newClient->id + "]", "", "");
    clogs("Адрес " + pSocket->peerAddress().toString() + " Имя " +
              pSocket->peerName() + " Порт " +
              QString::number(pSocket->peerPort()),
          "", "");

    connect(pSocket, &QWebSocket::disconnected, this,
            &WebServer::socketDisconnected);
    connect(pSocket, &QWebSocket::textMessageReceived, this,
            &WebServer::processMessage);

    webServerAnswer("Соединение установлено. Ваш ID [" +
                        newClient->id + "]", pSocket);
    delete tmr;
    m_clients << pSocket;
    m_client_list.push_back(newClient);
  } else
    clogs("клиент уже подключен", "", "");
}

//случайное имя канала связи
QString WebServer::GetRandomString() {
  const QString possibleCharacters(
      "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789");
  const int randomStringLength =
      12; // assuming you want random strings of 12 characters

  QString randomString;
  for (int i = 0; i < randomStringLength; ++i) {
    //int index = qrand() % possibleCharacters.length();
    int index = QRandomGenerator::global()->bounded(possibleCharacters.size()-1);
    QChar nextChar = possibleCharacters.at(index);
    randomString.append(nextChar);
  }
  return randomString;
}

//отключение клиента
void WebServer::socketDisconnected() {
  QWebSocket *pClient = qobject_cast<QWebSocket *>(sender());
  clientAI *clientInfo =
      (clientAI *)pClient->property("client_info").toULongLong();
  clogs("клиент [" + clientInfo->id + "] " + "отключен", "", "");
  if (pClient) {   
     m_clients.removeAll(pClient);
     task_kill(pClient);
     QTest::qSleep(1000);
     pClient->deleteLater();
     m_client_list.removeAll(clientInfo);
     clientInfo->deleteLater();
  }
}

//прием входящего сообщения
void WebServer::processMessage(const QString &message) {

  QWebSocket *pSender = qobject_cast<QWebSocket *>(sender());
  clientAI *clientInfo =
      (clientAI *)pSender->property("client_info").toULongLong();
  clogs("прием сообщения [" + clientInfo->id + "]", "", "");
  clogs("декодирование сообщения [" + clientInfo->id + "]", "", "");

  QJsonDocument jsonResponse = QJsonDocument::fromJson(message.toUtf8());
  // qDebug() << "Received message:" << jsonResponse.toJson(); // Вывод полученных данных

  //Начало кода тестирования
  QFile file0("testing.json");
  QString testing;
  QJsonDocument testing_comm;
  QJsonObject jsonObject;
  if (file0.open(QIODevice::ReadOnly | QIODevice::Text)) { //для тестирования
     clogs("чтение файла с настройками тестирования","","");
     testing = file0.readAll();
     testing_comm = QJsonDocument::fromJson(testing.toUtf8());
     if (testing_comm.isEmpty()) {
        clogs("файл с настройками тестирования пустой","","");
     }
     else {
         jsonObject = testing_comm.object();
         if (jsonObject.contains("forming_file_bin_message")) {
             if(jsonObject.value("forming_file_bin_message") == "true"){
                 clogs("команда на запись бинарного сообщения от клиента в файл message.bin","","");
                 QFile file("message.bin");
                 if(!file.open(QIODevice::WriteOnly)) {
                     clogs("ошибка открытия файла message.bin для записи","","");
                 }
                 else {
                     clogs("запись файла message.bin","","");
                     file.resize(0);
                     QDataStream stream(&file);
                     stream. setVersion(QDataStream::Qt_4_2);
                     stream << message;
                 }
                 file.close();
              }
             else {
                 clogs("команда на запись бинарного сообщения от клиента в файл message.bin не распознана","","");
             }
          }
          else {
              clogs("команда тестирования не распознана","","");
          }
       }
     file0.close();
     }
  //конец кода тестирования

  if (!jsonResponse.isEmpty()) {
    QJsonObject jsonObject = jsonResponse.object();

    if (clientInfo->authStatus) {

      if (jsonObject.value("type").toString() == "triangles") { //radardata!!!
            clogs("исходные данные приняты", "", "");
            webServerAnswer("исходные данные приняты", pSender);
            this->loadRadarData(jsonResponse, jsonObject, pSender);
      }
      else if (jsonObject.value("type").toString() == "cmd") {
          QJsonArray cmdParam = jsonObject.value("param").toArray();
          webServerAnswer(
              "принята команда " + jsonObject.value("cmd").toString(), pSender);
          this->handlerCmd(jsonObject.value("cmd").toString(), &cmdParam,  //command handler
                         pSender);
      }
      else {
          webServerAnswer("принятое сообщение имеет неизвестный тип", pSender);
          return;
      }
    }
    else if (jsonObject.value("type").toString() == "auth") {
      QJsonArray cmdParam = jsonObject.value("param").toArray();
      webServerAnswer("производится аутентификация клиента", pSender);
      this->userVerification(jsonObject.value("login").toString(),
                             jsonObject.value("password").toString(), pSender);
    }
    else {
        webServerAnswer("принятое сообщение имеет неизвестный тип", pSender);
        return;
    }
  }
  else {
     webServerAnswer("принятое сообщение не содержит данных", pSender);
     return;
  }

}

//авторизация клиента
void WebServer::userVerification(QString login, QString password,
                                 QWebSocket *pSender) {
  clientAI *clientInfo =
      (clientAI *)pSender->property("client_info").toULongLong();
  QString answer;

  //Начало кода тестирования
  QFile file0("users.json");
  QString auth_data;
  QJsonDocument auth_json;
  QJsonObject jsonObject;
  if (file0.open(QIODevice::ReadOnly | QIODevice::Text)) { //для тестирования
      auth_data = file0.readAll();
      auth_json = QJsonDocument::fromJson(auth_data.toUtf8());
      if (auth_json.isEmpty()) {
         clogs("файл users.json пустой","","");
      }
       else { //без тестирования
          jsonObject = auth_json.object();
          QString pass;
          if (jsonObject.contains(login) && password == jsonObject.value(login).toString()) {
            answer = "клиент подключен";
            clogs("клиент [" + login + "] подключен", "", "");
            clogs("клиент [" + login + "] = [" + clientInfo->id + "]", "", "");
            clientInfo->login = login;
            clientInfo->authStatus = true;
          } else {
            answer = "клиент не авторизован";
            clogs("клиент [" + login + "] не авторизован", "", "");
          }
          webServerAnswer(answer, pSender);
      }
      file0.close();
  }
  //Конец кода тестирования
  else {
      if (login == "user" && password == "user") {
          answer = "клиент подключен";
          clogs("клиент [" + login + "] подключен", "", "");
          clogs("клиент [" + login + "] = [" + clientInfo->id + "]", "", "");
          clientInfo->login = login;
          clientInfo->authStatus = true;
      }
      else {
          answer = "клиент не авторизован";
          clogs("клиент [" + login + "] не авторизован", "", "");
      }
      webServerAnswer(answer, pSender);
  }

}

//прием исходных данных и создание вычислительного ядра
void WebServer::loadRadarData(QJsonDocument &doc, QJsonObject &jsonObject, QWebSocket *pSender) {
    clientAI *clientInfo =
        (clientAI *)pSender->property("client_info").toULongLong();
    int size = task_list.size();
    uint id = GetCoreID(doc);
    for (int i = 0; i < size; i++) {
       if (task_list.at(i)->getClientRadar()->id == clientInfo->id) { //канал связи совпал
          if (task_list.at(i)->getModelId() == id) { //входные данные совпали
              clogs("ресурс [" + clientInfo->id + "] не выделен", "", "");
              QString QAnswer;
              QAnswer = "задача для oбъекта " + QString::number(jsonObject.value("id").toInt());
              QAnswer = QAnswer + " [ " + clientInfo->id   + "] уже запущена";
              webServerAnswer(QAnswer, pSender);
              return;
          }
          else {
              //socketDisconnected();
              task_kill(pSender); //другие входные данные, удаление старой задачи
          }
       }
    }
    QTest::qSleep(1000);
    //создание потока с вычислительным ядром
    QThread *pthread_radar = new QThread(this);
    if (pthread_radar != 0x0) {
        radarCore *pCore = new radarCore;
        pCore->moveToThread(pthread_radar);
        //соединение их сигналов-слотов
        connect(pthread_radar, &QThread::started, pCore, &radarCore::run);
        connect(pCore, &radarCore::finished, pthread_radar, &QThread::quit);
        connect(pCore, &radarCore::finished, pCore, &radarCore::deleteLater);
        connect(pthread_radar, &QThread::finished, pthread_radar, &QThread::deleteLater);

        connect(pCore, &radarCore::task_kill, this, &WebServer::kill_task);
        connect(pCore, &radarCore::send_text, this, &WebServer::message_calc_radar);
        connect(pCore, &radarCore::send_progress_bar, this, &WebServer::message_calc_radar);
        connect(pCore, &radarCore::send_result, this, &WebServer::send_calc_radar_result);
        connect(this, &WebServer::pause, pCore, &radarCore::pause_core);

        //добавление новой задачи в список задач
        pCore->setRadarParam(doc, clientInfo, pSender);
        pCore->id = jsonObject.value("id").toInt();
        pCore->setModelId(id);
        pthread_radar->start();

        task_list.push_back(pCore);
        clogs("ресурс [" + clientInfo->id + "] выделен", "", "");
        qDebug() << "Radar calculation started for client:" << clientInfo->id;

        //Запись в файл
        if (SAVE_INPUT_DATA_TO_FILES) {
            QString QAnswer;
            QAnswer = "запись ИД в файл";
            webServerAnswer(QAnswer, pSender);
            clogs("запись ИД в файл для [" + clientInfo->id + "]","","");

            QFile file1("input_data.bin");
            QFile file2("input_data.json");

            if(!file1.open(QIODevice::WriteOnly)) {
                clogs("ошибка открытия файла input_data.bin для записи","","");
            }
            else {
                clogs("запись бинарного файла input_data.bin","","");
                file1.resize(0);
                QDataStream stream(&file1);
                stream. setVersion(QDataStream::Qt_4_2);
                stream << doc;
            }
            file1.close();
            if (!SERIALIZE_MODEL) {
                if(!file2.open(QIODevice::WriteOnly)) {
                    clogs("ошибка открытия файла input_data.json для записи","","");
                }
                else {
                    clogs("запись текстового файла input_data.json","","");
                    file2.resize(0);
                    file2.write(doc.toJson());
                }
            }
            file2.close();

            SAVE_INPUT_DATA_TO_FILES = false;
            SERIALIZE_MODEL = false;
        }
        if (SAVE_MODEL){
            QString QAnswer;
            QAnswer = "запись модели в файл";
            webServerAnswer(QAnswer, pSender);
            clogs("запись модели в файл для [" + clientInfo->id + "]","","");
            pCore->SAVE_MODEL_TO_FILE = true;
            SAVE_MODEL = false;
        }
        if (SAVE_SCAT_FIELD){
            QString QAnswer;
            QAnswer = "запись рассеянного поля в файл";
            webServerAnswer(QAnswer, pSender);
            clogs("запись рассеянного поля в файл для [" + clientInfo->id + "]","","");
            pCore->SCAT_FIELD_TO_FILE = true;
            SAVE_SCAT_FIELD = false;
        }
        if (SAVE_FFT_FIELD){
            QString QAnswer;
            QAnswer = "запись fft-поля в файл";
            webServerAnswer(QAnswer, pSender);
            clogs("запись fft-поля в файл для [" + clientInfo->id + "]","","");
            pCore->FFT_FIELD_TO_FILE = true;
            SAVE_FFT_FIELD = false;
        }
        if (READ_RESULT){
            QString QAnswer;
            QAnswer = "чтение fft-поля из файла";
            webServerAnswer(QAnswer, pSender);
            clogs("чтение fft-поля из файла для [" + clientInfo->id + "]","","");
            pCore->RESULT_FROM_FILE = true;
            READ_RESULT = false;
        }
        if (SAVE_MESSAGE_RESULT) {
            QString QAnswer;
            QAnswer = "запись сообщения в файл";
            webServerAnswer(QAnswer, pSender);
            clogs("запись сообщения в файл для [" + clientInfo->id + "]","","");
            pCore->SAVE_MESSAGE_TO_FILE = true;
            SAVE_MESSAGE_RESULT = false;
        }
    }
    else clogs("память для [" + clientInfo->id + "] не выделена","","");
}

//прием сигналов управления
void WebServer::handlerCmd(QString command, QJsonArray *params,
                           QWebSocket *pSender) {
    clogs("поступила команда [" + command + " " + params->at(0).toString() + "]", "",
          "");
    if (command == "server") {
        if (params->at(0).toString() == "status") { //display server status
          webServerAnswer(
              "Статус: [OK] Версия: 1.0 Клиент ID: " +
                  pSender->property("id").toString() + " Плагины: " +
                  "#include <Radar Core> " + "АО 'НЦ ПЭ' 2022, все права защищены",
              pSender);
        }
        else if (params->at(0).toString() == "whoconnect") { //list connected clients
          QString QAnswer;
          QAnswer = "Подключены клиенты: ";
          for (int i = 0; i < m_clients.size(); i++) {
            clientAI *clientInfo =
                (clientAI *)m_clients[i]->property("client_info").toULongLong();
            QAnswer = QAnswer + QString::number(i) + ". " + clientInfo->login +
                      " [ " + clientInfo->id + " ] <" +
                      clientInfo->timeConnect.toString("hh.mm.ss") + " " +
                      clientInfo->dateConnect.toString("dd.MM.yyyy") + " ";
          }
          webServerAnswer(QAnswer, pSender);
        }
        else if (params->at(0).toString() == "pause") { //поставить вычисление на паузу
            QString QAnswer;
            QAnswer = " пауза для: ";
            int cmd_id = 2;
            setCmd(QAnswer,pSender,cmd_id);
        }
        else if (params->at(0).toString() == "continue") { //поставить вычисление на паузу
            QString QAnswer;
            QAnswer = " продолжение вычислений для: ";
            int cmd_id = 3;
            setCmd(QAnswer,pSender,cmd_id);
        }
        else if (params->at(0).toString() == "stop") { //поставить вычисление на паузу
            clientAI *clientInfo =
                (clientAI *)pSender->property("client_info").toULongLong();
            QString QAnswer;
            QAnswer = " принудительное завершение вычислений для: ";
            int cmd_id = 4;
            setCmd(QAnswer,pSender,cmd_id);
            clogs("автоматический режим для [" + clientInfo->id + "] включен","","");
        }
        else if (params->at(0).toString() == "start_test") { //поставить вычисление на паузу
            clientAI *clientInfo =
                (clientAI *)pSender->property("client_info").toULongLong();
            QJsonDocument doc;
            QJsonObject jsonObject;
            loadRadarData(doc,jsonObject,pSender);
            QString QAnswer;
            QAnswer = " перевод в режим тестирования для: ";
            int cmd_id = 5;
            setCmd(QAnswer,pSender,cmd_id);
            clogs("режим тестирования для [" + clientInfo->id + "] включен","","");
        }
        else if (params->at(0).toString() == "save_file_on") {
            clientAI *clientInfo =
                (clientAI *)pSender->property("client_info").toULongLong();
            SAVE_INPUT_DATA_TO_FILES = true;
            QString QAnswer;
            QAnswer = "запись ИД в файл включена";
            webServerAnswer(QAnswer, pSender);
            clogs("запись ИД в файл для [" + clientInfo->id + "] включена","","");
        }
        else if (params->at(0).toString() == "save_model_on") {
            clientAI *clientInfo =
                (clientAI *)pSender->property("client_info").toULongLong();
            SAVE_MODEL = true;
            QString QAnswer;
            QAnswer = "запись модели в файл включена";
            webServerAnswer(QAnswer, pSender);
            clogs("запись модели в файл для [" + clientInfo->id + "] включена","","");
        }
        else if (params->at(0).toString() == "save_scat_on") {
            clientAI *clientInfo =
                (clientAI *)pSender->property("client_info").toULongLong();
            SAVE_SCAT_FIELD = true;
            QString QAnswer;
            QAnswer = "запись рассеянного поля в файл включена";
            webServerAnswer(QAnswer, pSender);
            clogs("запись рассеянного поля в файл для [" + clientInfo->id + "] включена","","");
        }
        else if (params->at(0).toString() == "save_fft_on") {
            clientAI *clientInfo =
                (clientAI *)pSender->property("client_info").toULongLong();
            SAVE_FFT_FIELD = true;
            QString QAnswer;
            QAnswer = "запись fft-поля в файл включена";
            webServerAnswer(QAnswer, pSender);
            clogs("запись fft-поля в файл для [" + clientInfo->id + "] включена","","");
        }
        else if (params->at(0).toString() == "serialize_model_on") {
            clientAI *clientInfo =
                (clientAI *)pSender->property("client_info").toULongLong();
            SERIALIZE_MODEL = true;
            SAVE_INPUT_DATA_TO_FILES = true;
            QString QAnswer;
            QAnswer = "сериализация модели в бинарный файл включена";
            webServerAnswer(QAnswer, pSender);
            clogs("сериализация модели в бинарный файл для [" + clientInfo->id + "] включена","","");
        }
        else if (params->at(0).toString() == "read_model_on") {
            clientAI *clientInfo =
                (clientAI *)pSender->property("client_info").toULongLong();
            QString QAnswer;
            QAnswer = "чтение модели из бинарного файла включено";
            webServerAnswer(QAnswer, pSender);
            clogs("чтение модели из бинарного файла для [" + clientInfo->id + "] включено","","");
            QJsonDocument doc;
            QJsonObject jsonObject;
            QFile file1("input_data.bin");
            if(!file1.open(QIODevice::ReadOnly)) {
                clogs("ошибка открытия файла input_data.bin для записи","","");
            }
            else {
                clogs("чтение файла input_data.bin","","");
                //file1.resize(0);
                QDataStream stream(&file1);
                stream.setVersion(QDataStream::Qt_4_2);
                stream >> doc;
            }
            file1.close();
            jsonObject = doc.object();
            loadRadarData(doc,jsonObject,pSender);
        }
        else if (params->at(0).toString() == "send_model") {
            clientAI *clientInfo =
                (clientAI *)pSender->property("client_info").toULongLong();
            //загрузка модели из файла и передача ее клиенту
            //...
            QString QAnswer;
            QAnswer = "передача модели клиенту начата";
            webServerAnswer(QAnswer, pSender);
            clogs("передача модели клиенту [" + clientInfo->id + "]","","");
        }
        else if (params->at(0).toString() == "read_result") {
            clientAI *clientInfo =
                (clientAI *)pSender->property("client_info").toULongLong();
            //загрузка поля рассеяния из файла и передача его клиенту
            READ_RESULT = true;
            QString QAnswer;
            QAnswer = "чтение бинарного файла с результатами расчета";
            webServerAnswer(QAnswer, pSender);
            clogs("чтение бинарного файла с результатами расчета для [" + clientInfo->id + "]","","");
        }
        else if (params->at(0).toString() == "save_message") {
            clientAI *clientInfo =
                (clientAI *)pSender->property("client_info").toULongLong();
            //запись сообщения с результатами расчета в файл
            SAVE_MESSAGE_RESULT = true;
            QString QAnswer;
            QAnswer = "сохранение сообщения с результатами расчета";
            webServerAnswer(QAnswer, pSender);
            clogs("сохранение сообщения с результатами расчета для [" + clientInfo->id + "]","","");
        }
        else {
            clientAI *clientInfo =
                (clientAI *)pSender->property("client_info").toULongLong();
            //неизвестный тип команды
            QString QAnswer;
            QAnswer = "команда не зарегистрирована";
            webServerAnswer(QAnswer, pSender);
            clogs("команда не зарегистрирована [" + clientInfo->id + "]","","");
        }
    }
   return;
}

//генерирование команд управления вычислительным ядром
void WebServer::setCmd(QString &message, QWebSocket *pSender, int &cmd_id) {
    QString QAnswer;
    QAnswer = message;
    int size = task_list.size();
    if (size == 0) {
       QAnswer = "Ошибка. Текущих вычислений нет.";
       webServerAnswer(QAnswer, pSender);
       return;
    }
    for (int i = 0; i < size; i++) {
        clientAI *clientInfo =
            (clientAI *)pSender->property("client_info").toULongLong();
        if (task_list.at(i)->getClientRadar()->id == clientInfo->id) {
            QAnswer = QAnswer + QString::number(i) + ". " + clientInfo->login +
                    " [ " + clientInfo->id + " ]" + " Статус: [OK] ";
            webServerAnswer(QAnswer, pSender);
            if (cmd_id == 2) {
               QAnswer = "приостановка вычислений";
               webServerAnswer(QAnswer, pSender);
               task_list.at(i)->pause_core();
               //emit pause();
            }
            if (cmd_id == 3) {
               QAnswer = "продолжение вычислений";
               webServerAnswer(QAnswer, pSender);
               task_list.at(i)->continue_core();
            }
            if (cmd_id == 4) {
               QAnswer = "завершение вычислений";
               webServerAnswer(QAnswer, pSender);
               task_list.at(i)->testing(false);
               task_kill(pSender);
               return;
            }
            if (cmd_id == 5) {
               QAnswer = "включение тестирования";
               webServerAnswer(QAnswer, pSender);
               task_list.at(i)->testing(true);
            }
        }
    }
}

//слот уничтожения задач вычислительному ядру
void WebServer::kill_task(QWebSocket *pSender) {
    task_kill(pSender);
}

//процедура уничтожения задач и вычислительного ядра
bool WebServer::task_kill(QWebSocket *pSender) {
    QString QAnswer;
    int size = task_list.size();
    if (size == 0) {
       QAnswer = "список задач пуст";
       webServerAnswer(QAnswer, pSender);
       return true;
    }
    clientAI *clientInfo =
        (clientAI *)pSender->property("client_info").toULongLong();
    int n = 0;
    for (int i = 0; i < size; i++) {
        if (task_list.at(i)->getClientRadar()->id == clientInfo->id) {
            radarCore *Core = task_list.at(i);
            QAnswer = QString::number(n) + ". " + clientInfo->login +
                    " [<b>" + clientInfo->id + "</b>]";
            webServerAnswer(QAnswer, pSender);
            clogs("освобождение ресурса [" + clientInfo->id + "]", "", "");
            QAnswer = "ресурс освобожден";
            webServerAnswer(QAnswer, pSender);
            task_list.at(i)->setRunning(false);
            task_list.at(i)->stop();
//            QTest::qSleep(1000);
            task_list.at(i)->finished();
            task_list.removeAll(Core);
            //Core->deleteLater();
        }
        n++;
    }
    return true;
}

//слот передачи текстового сообщения клиенту
void WebServer::message_calc_radar(QString doc, QWebSocket *Client) {
  Client->sendTextMessage(doc);
}

//слот передачи результата расчета клиенту
void WebServer::send_calc_radar_result(QString doc, QWebSocket *Client) {
    clientAI *clientInfo = (clientAI *)Client->property("client_info").toULongLong();
    clogs("передача результата [" + clientInfo->id + "]", "", "");

    QJsonObject resultObj;
    resultObj.insert("type", QJsonValue::fromVariant("result"));
    resultObj.insert("content", QJsonDocument::fromJson(doc.toUtf8()).object());
    QJsonDocument resultDoc(resultObj);

    // qDebug() << "Sending result to client:" << resultDoc.toJson(QJsonDocument::Compact);

    Client->sendTextMessage(resultDoc.toJson(QJsonDocument::Compact));
    webServerAnswer("вычисления окончены", Client);
}

//генерирование идентификатора исходных данных
uint WebServer::GetCoreID(QJsonDocument &doc){
   QJsonObject jsonObject = doc.object();
   uint y = qHash(jsonObject);
   return y;
}

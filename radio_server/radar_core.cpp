#include "radar_core.h"
#include <QDataStream>


QString Txt;


radarCore::~radarCore() {
    qDebug() << "Radar_core destructor";
    if (m_timer.isRunning()) m_timer.stop();
    progress = 100;
    sendProgressBar();
    QTest::qSleep(1000);
}


void radarCore::run() { //запуск главного процесса

    QHash<uint, node> Node;
    QHash<uint,edge> Edge;

    m_running = true;
    try {
        while (m_running) {
            QTest::qSleep(1000);
            while (TEST) {
                QTest::qSleep(1000);
                Txt = "тестирование вычислительного ядра";
                sendText();
                while (!RUN) {
                   Txt = "вычислительное ядро на паузе";
                   sendText();
                   QTest::qSleep(1000);
                }
            }

            //Прохождение методов расчета радиопортрета
//            if (!RESULT_FROM_FILE){
               parseJSONtoRadar(Node,Edge);
               calcRadar();
//            }
            calcRadarResult();
            m_running = false;
        }
        QTest::qSleep(1000);
        emit finished();
        emit task_kill(m_Client);;
    }
    catch (int a) {
        if (a == -1) {
           Txt = "ядро остановлено"; sendText();
           QTest::qSleep(1000);
           emit finished();
           emit task_kill(m_Client);
        }
    }
    catch (std::bad_alloc &ba) {
        // обработка ошибки выделения памяти
        Txt = "нехватка памяти"; sendText();
        QTest::qSleep(1000);
        emit finished();
        emit task_kill(m_Client);
    }

}

void radarCore::stop() {
    Exit();
}

void radarCore::setRadarParam(QJsonDocument &doc, clientAI *client,
                                     QWebSocket *pClient) {
  this->m_doc = doc;
  this->m_clientRadar = client;
  this->m_Client = pClient;
}


void radarCore::pause_core() {
    RUN = false;
    return;
}


void radarCore::continue_core() {
    RUN = true;
    return;
}


void radarCore::testing(bool test) {
    TEST = test;
}


void radarCore::sendText() {
    QJsonObject Echo;
    Echo.insert("type", QJsonValue::fromVariant("answer"));
    Echo.insert("msg", QJsonValue::fromVariant(Txt));
    QJsonDocument doc(Echo);
    emit send_text(doc.toJson(), m_Client);
}


void radarCore::parseJSONtoRadar(QHash<uint, node> &Node, QHash<uint,edge> &Edge){

    QJsonObject jsonObject = m_doc.object();
    Txt = "преобразование входных данных..."; sendText();
    int err;
    err = build_Model(jsonObject,Node,Edge); sendText();
    if (err == 0) {
       Txt = "входные данные преобразованы успешно"; sendText();
    }
    else if(err == 1) {
       Txt = "в переданном сообщении отсутствуют данные по координатам"; sendText();
       throw -1;
    }
    else if(err == 2) {
       Txt = "в переданном сообщении отсутствуют данные по освещенности"; sendText();
       throw -1;
    }
    else if(err == 3) {
       Txt = "в переданном сообщении отсутствуют данные по направлению падения волны"; sendText();
       throw -1;
    }
    else if(err == 4) {
       Txt = "проверьте поляризацию"; sendText();
       throw -1;
    }
    else if(err == 5) {
       Txt = "тип радиопортрета не задан"; sendText();
       throw -1;
    }
    return;
}


void radarCore::calcRadar() {

    connect(this, &culcradar::signal_send_progress_bar_culcradar,
            this, &radarCore::sendProgressBar);

    Txt = "расчет радиопортрета..."; sendText();
    int err;
    err = culc_Eout();
    if(err < 0) {
       Txt = "расчет радиопортрета завершен с ошибкой"; sendText();
       throw -1;
    }
    else {
       Txt = "расчет радиопортрета завершен успешно"; sendText();
    }

    disconnect(this, &culcradar::signal_send_progress_bar_culcradar,
            this, &radarCore::sendProgressBar);
    return;
}


void radarCore::calcRadarResult() {
    Txt = "формирование результата для передачи..."; sendText();

    //Инициализация передаваемого json-документа
    QJsonObject Echo;
    QJsonDocument doc;
    //вставка в JSON-документ служебной информации
    Echo.insert("type", QJsonValue::fromVariant("result"));
    Echo.insert("id", QJsonValue::fromVariant(id));
    Echo.insert("content", QJsonValue::fromVariant("radioportrait"));

    //размеры массивов
    uint size1 = getSizeEoutX();
    uint size2 = getSizeEoutY();
    uint size3 = getSizeEoutZ();

    //инициализация json-массивов для заполнения полем рассеяния
    QJsonArray m_fft_absEout = {};  //длина векторов (abs)
    QJsonArray m_fft_normEout = {}; //норма векторов (norm)
    QJsonArray m_fft_Eout_base = {}; //по основной поляризации (basis)
    QJsonArray m_fft_Eout_cross = {}; //по кросс-поляризации (cross)
    cVect Eout_base, Eout_cross; //E по осн. поляризации и кроссполяризации
    rVect rEin = this->getEin();
    std::complex<double> dotE;

    //вычисление границ объекта
    double Lmax = get_Lmax();
    uint n, m, q;
    n = 0; m = 0; q = 0;
    double stepX, stepY, stepZ;
    stepX = get_stepX();
    stepY = get_stepY();
    stepZ = get_stepZ();
    if (stepX > 0) n = Lmax / get_stepX();
    if (stepY > 0) m = Lmax / get_stepY();
    if (stepZ > 0) q = Lmax / get_stepZ();

    //вычисление отступа от края портрета
    uint n1 = round((size1 - n)/2);
    uint m1 = round((size2 - m)/2);
    uint q1 = round((size3 - q)/2);

    //отмена ограничений разрешения картинки (для мелких объектов)
    if (size1 < 64) {
        n = size1; n1 = 0;
    }
    if (size2 < 64) {
        m = size2; m1 = 0;
    }
    if (size3 < 64) {
        q = size3; q1 = 0;
    }

    //заполнение json-массивов полем рассеяния с учетом "обрезки"
    for(uint i = 0; i < size3; i++) {
//        QJsonArray m_fft_Eout_baseY = {};
//        QJsonArray m_fft_Eout_crossY = {};
        QJsonArray m_fft_absY = {};
        QJsonArray m_fft_normY = {};
        for (uint j = 0; j < size2; j++) {
//            QJsonArray m_fft_Eout_baseX = {};
//            QJsonArray m_fft_Eout_crossX = {};
            QJsonArray m_fft_absX = {};
            QJsonArray m_fft_normX = {};
            for (uint k = 0; k < size1; k++) {
               Eout_base = getEout(k + n1,j + m1,i + q1);
               dotE = dot(Eout_base,rEin);
               Eout_base = dotE * rEin; //расс. поле осн. поляризации
               radar_wave RW = getRWave();
               if (RW.getIncPolariz() != RW.getRefPolariz()) {
                  Eout_cross = Eout_base - getEout(k + n1,j + m1,i + q1); //расс. поле кросс-поляризации
                  Eout_base = Eout_cross;
               }
//               Eout_cross = Eout_base - getEout(k,j,i); //расс. поле кросс-поляризации
//               QJsonObject JSONEout_base, JSONEout_cross;
//               JSONEout_base.insert("X_real", Eout_base.getX().real());
//               JSONEout_base.insert("X_imag", Eout_base.getX().imag());
//               JSONEout_base.insert("Y_real", Eout_base.getY().real());
//               JSONEout_base.insert("Y_imag", Eout_base.getY().imag());
//               JSONEout_base.insert("Z_real", Eout_base.getZ().real());
//               JSONEout_base.insert("Z_imag", Eout_base.getZ().imag());
//               JSONEout_cross.insert("X_real", Eout_cross.getX().real());
//               JSONEout_cross.insert("X_imag", Eout_cross.getX().imag());
//               JSONEout_cross.insert("Y_real", Eout_cross.getY().real());
//               JSONEout_cross.insert("Y_imag", Eout_cross.getY().imag());
//               JSONEout_cross.insert("Z_real", Eout_cross.getZ().real());
//               JSONEout_cross.insert("Z_imag", Eout_cross.getZ().imag());
//               m_fft_Eout_baseX.push_back(JSONEout_base);
//               m_fft_Eout_crossX.push_back(JSONEout_cross);
               m_fft_absX.push_back(Eout_base.length()); //длина векторов (abs)
               m_fft_normX.push_back(Eout_base.norm());  //норма векторов (norm)
               if (k == n) break;
            }
//            m_fft_Eout_baseY.push_back(m_fft_Eout_baseX);
//            m_fft_Eout_crossY.push_back(m_fft_Eout_crossX);
            m_fft_absY.push_back(m_fft_absX);
            m_fft_normY.push_back(m_fft_normX);
            if (j == m) break;
        }
//         m_fft_Eout_base.push_back(m_fft_Eout_baseY);
//         m_fft_Eout_cross.push_back(m_fft_Eout_crossY);
         m_fft_absEout.push_back(m_fft_absY);
         m_fft_normEout.push_back(m_fft_normY);
         if (i == q) break;
    }

    //вставка в json-документ информации о размерах json-массивов
    //(0-одномерн., 1-двумерн., 2-трехмерн., -1-единичн.):
    if ((size3 == 1) && (size2 == 1) && (size1 > 1)) {
       Echo.insert("dimension_type",0);
    }
    else if ((size3 == 1) && (size2 > 1) && (size1 == 1)) {
       Echo.insert("dimension_type",0);
    }
    else if ((size3 > 1) && (size2 == 1) && (size1 == 1)) {
       Echo.insert("dimension_type",0);
    }
    else if ((size3 > 1) && (size2 > 1) && (size1 == 1)) {
       Echo.insert("dimension_type",1);
    }
    else if ((size3 > 1) && (size2 == 1) && (size1 > 1)) {
       Echo.insert("dimension_type",1);
    }
    else if ((size3 == 1) && (size2 > 1) && (size1 > 1)) {
       Echo.insert("dimension_type",1);
    }
    else if ((size3 > 1) && (size2 > 1) && (size1 > 1)) {
        Echo.insert("dimension_type",2);
    }
    else {
       Echo.insert("dimension_type",-1);
    }

    //вставка в json-документ массивов и комментарии к ним
//    Echo.insert("Eout_base",m_fft_Eout_base);
//    Echo.insert("info_Eout_base",QString("fft result, complex vector for base polarization"));
//    Echo.insert("Eout_cross",m_fft_Eout_cross);
//    Echo.insert("info_Eout_cross",QString("fft result, complex vector for cross polarization"));
    Echo.insert("absEout",m_fft_absEout);
    Echo.insert("info_absEout",QString("fft result, absolute value"));
    Echo.insert("normEout",m_fft_normEout);
    Echo.insert("info_normEout",QString("fft result, norm value"));

    Txt = "передача результата клиенту"; sendText();
    //Send document to client
    QJsonDocument doc1(Echo);
    emit send_result(doc1.toJson(), m_Client);

    //Сохранение сообщения клиенту в json и бинарный файл
    if (SAVE_MESSAGE_TO_FILE) {
        //запись сформированных сообщений в json- и бинарной форме
        QFile file1("client_message.json");
        QFile file2("client_message.bin");
        if(!file1.open(QIODevice::WriteOnly)) {
            clogs("файл client_message.json не может быть открыт","","");
        }
        else {
            clogs("сохранение файла client_message.json","","");
            file1.resize(0);
            file1.write(doc1.toJson());
        }

        file1.close();

        if(!file2.open(QIODevice::WriteOnly)) {
           clogs("файл client_message.bin не может быть открыт","","");
        }
        else {
            clogs("сохранение файла client_message.bin","","");
            file2.resize(0);
            QDataStream stream(&file2);
            stream. setVersion(QDataStream::Qt_4_2);
            stream << doc1;
        }
        file2.close();
    }

    return;
}


void radarCore::sendProgressBar() {
    QJsonObject Echo;
    Echo.insert("type", QJsonValue::fromVariant("progress_bar"));
    Echo.insert("id", QJsonValue::fromVariant(id));
    if((progress == 0) && (count == true)) {
        Echo.insert("status", QJsonValue::fromVariant("run"));
        count = false;
        //Echo.insert("content", QJsonValue::fromVariant(progress));
    }
    else if (progress == 100){
        Echo.insert("status", QJsonValue::fromVariant("done"));
        //Echo.insert("content", QJsonValue::fromVariant(progress));
    }
    else {
        Echo.insert("status", QJsonValue::fromVariant("work"));
        //Echo.insert("content", QJsonValue::fromVariant(progress));
    }
    Echo.insert("content", QJsonValue::fromVariant(progress));
    //Echo.insert("msg", QJsonValue::fromVariant(progress));
    QJsonDocument doc(Echo);
    emit send_progress_bar(doc.toJson(), m_Client);
}

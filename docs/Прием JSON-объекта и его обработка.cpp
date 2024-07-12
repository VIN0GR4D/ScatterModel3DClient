Прием JSON-объекта и его обработка
1. Получение сообщения:
Сообщение от клиента принимается в WebServer::processMessage().
Сообщение должно быть в формате JSON.

void WebServer::processMessage(const QString &message) {
    QWebSocket *pSender = qobject_cast<QWebSocket *>(sender());
    clientAI *clientInfo = (clientAI *)pSender->property("client_info").toULongLong();
    clogs("прием сообщения [" + clientInfo->id + "]", "", "");
    clogs("декодирование сообщения [" + clientInfo->id + "]", "", "");

    QJsonDocument jsonResponse = QJsonDocument::fromJson(message.toUtf8());
    qDebug() << "Received message:" << jsonResponse.toJson(); // Вывод полученных данных

    if (!jsonResponse.isEmpty()) {
        QJsonObject jsonObject = jsonResponse.object();

        if (clientInfo->authStatus) {
            if (jsonObject.value("type").toString() == "triangles") { // radardata!!!
                clogs("исходные данные приняты", "", "");
                webServerAnswer("исходные данные приняты", pSender);
                this->loadRadarData(jsonResponse, jsonObject, pSender);
            } else if (jsonObject.value("type").toString() == "cmd") {
                QJsonArray cmdParam = jsonObject.value("param").toArray();
                webServerAnswer("принята команда " + jsonObject.value("cmd").toString(), pSender);
                this->handlerCmd(jsonObject.value("cmd").toString(), &cmdParam, pSender); // command handler
            } else {
                webServerAnswer("принятое сообщение имеет неизвестный тип", pSender);
                return;
            }
        } else if (jsonObject.value("type").toString() == "auth") {
            QJsonArray cmdParam = jsonObject.value("param").toArray();
            webServerAnswer("производится аутентификация клиента", pSender);
            this->userVerification(jsonObject.value("login").toString(),
                                   jsonObject.value("password").toString(), pSender);
        } else {
            webServerAnswer("принятое сообщение имеет неизвестный тип", pSender);
            return;
        }
    } else {
        webServerAnswer("принятое сообщение не содержит данных", pSender);
        return;
    }
}

Передача данных в ядро обработки
2. Загрузка данных в ядро:
Данные передаются в функцию WebServer::loadRadarData().

void WebServer::loadRadarData(QJsonDocument &doc, QJsonObject &jsonObject, QWebSocket *pSender) {
    clientAI *clientInfo = (clientAI *)pSender->property("client_info").toULongLong();
    int size = task_list.size();
    uint id = GetCoreID(doc);
    for (int i = 0; i < size; i++) {
        if (task_list.at(i)->getClientRadar()->id == clientInfo->id) {
            if (task_list.at(i)->getModelId() == id) {
                clogs("ресурс [" + clientInfo->id + "] не выделен", "", "");
                QString QAnswer;
                QAnswer = "задача для oбъекта " + QString::number(jsonObject.value("id").toInt());
                QAnswer = QAnswer + " [ " + clientInfo->id + "] уже запущена";
                webServerAnswer(QAnswer, pSender);
                return;
            } else {
                task_kill(pSender); // другие входные данные, удаление старой задачи
            }
        }
    }
    QTest::qSleep(1000);
    // создание потока с вычислительным ядром
    QThread *pthread_radar = new QThread(this);
    if (pthread_radar != 0x0) {
        radarCore *pCore = new radarCore;
        pCore->moveToThread(pthread_radar);
        connect(pthread_radar, &QThread::started, pCore, &radarCore::run);
        connect(pCore, &radarCore::finished, pthread_radar, &QThread::quit);
        connect(pCore, &radarCore::finished, pCore, &radarCore::deleteLater);
        connect(pthread_radar, &QThread::finished, pthread_radar, &QThread::deleteLater);

        connect(pCore, &radarCore::task_kill, this, &WebServer::kill_task);
        connect(pCore, &radarCore::send_text, this, &WebServer::message_calc_radar);
        connect(pCore, &radarCore::send_progress_bar, this, &WebServer::message_calc_radar);
        connect(pCore, &radarCore::send_result, this, &WebServer::send_calc_radar_result);
        connect(this, &WebServer::pause, pCore, &radarCore::pause_core);

        // добавление новой задачи в список задач
        pCore->setRadarParam(doc, clientInfo, pSender);
        pCore->id = jsonObject.value("id").toInt();
        pCore->setModelId(id);
        pthread_radar->start();

        task_list.push_back(pCore);
        clogs("ресурс [" + clientInfo->id + "] выделен", "", "");
        qDebug() << "Radar calculation started for client:" << clientInfo->id;
    } else {
        clogs("память для [" + clientInfo->id + "] не выделена", "", "");
    }
}

Интерпретация и обработка JSON-объекта
3. Разбор JSON и построение модели:
В radarCore::run(), вызывается parseJSONtoRadar(), которая интерпретирует JSON-объект и строит модель.

void radarCore::run() {
    QHash<uint, node> Node;
    QHash<uint, edge> Edge;

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

            parseJSONtoRadar(Node, Edge);
            calcRadar();
            calcRadarResult();
            m_running = false;
        }
        QTest::qSleep(1000);
        emit finished();
        emit task_kill(m_Client);
    } catch (int a) {
        if (a == -1) {
            Txt = "ядро остановлено";
            sendText();
            QTest::qSleep(1000);
            emit finished();
            emit task_kill(m_Client);
        }
    } catch (std::bad_alloc &ba) {
        Txt = "нехватка памяти";
        sendText();
        QTest::qSleep(1000);
        emit finished();
        emit task_kill(m_Client);
    }
}

void radarCore::parseJSONtoRadar(QHash<uint, node> &Node, QHash<uint, edge> &Edge) {
    QJsonObject jsonObject = m_doc.object();
    Txt = "преобразование входных данных...";
    sendText();
    int err = build_Model(jsonObject, Node, Edge);
    sendText();
    if (err == 0) {
        Txt = "входные данные преобразованы успешно";
        sendText();
    } else if (err == 1) {
        Txt = "в переданном сообщении отсутствуют данные по координатам";
        sendText();
        throw -1;
    } else if (err == 2) {
        Txt = "в переданном сообщении отсутствуют данные по освещенности";
        sendText();
        throw -1;
    } else if (err == 3) {
        Txt = "в переданном сообщении отсутствуют данные по направлению падения волны";
        sendText();
        throw -1;
    } else if (err == 4) {
        Txt = "проверьте поляризацию";
        sendText();
        throw -1;
    } else if (err == 5) {
        Txt = "тип радиопортрета не задан";
        sendText();
        throw -1;
    }
    return;
}

Построение модели в CulcRadar
4. Построение модели на основе данных:
В CulcRadar.cpp функция build_Model() создает модель на основе входных данных.

int culcradar::build_Model(QJsonObject &jsonObject, QHash<uint, node> &Node, QHash<uint, edge> &Edge) {
    // Чтение данных координат
    QJsonObject coord;
    std::vector<double> n_coord;
    int n = 0;
    if (jsonObject.contains("data")) {
        coord = jsonObject.value("data").toObject();
        n = coord.count();
        assert(n > 0);
        for (int i = 0; i < n; i++) {
            QString ii = QString::number(i);
            if (coord.contains(ii)) {
                n_coord.push_back(coord.value(ii).toDouble());
            }
        }
    } else {
        return 1; // Ошибка: данные по координатам отсутствуют
    }

    // Обработка координат треугольников
    std::vector<std::vector<std::vector<double>>> tri;
    if (n_coord.size() % 9 == 0) tri.resize(n_coord.size() / 9);
    assert(n_coord.size() % 9 == 0);
    int jj = 0;
    for (int i = 0; i < (int)tri.size(); i++) {
        tri[i].resize(3);
        for (int j = 0; j < 3; j++) {
            tri[i][j].resize(3);
            for (int k = 0; k < 3; k++) {
                tri[i][j][k] = n_coord[jj];
                jj++;
            }
        }
    }

    // Создание узлов и ребер
    for (int i = 0; i < (int)tri.size(); i++) {
        for (int j = 0; j < 3; j++) {
            int k = j + 1;
            if (k == 3) k = 0;

            double x1 = tri[i][j][0];
            double y1 = tri[i][j][1];
            double z1 = tri[i][j][2];
            double x2 = tri[i][k][0];
            double y2 = tri[i][k][1];
            double z2 = tri[i][k][2];

            QJsonObject obj1, obj2;
            obj1.insert("x1", x1); obj1.insert("y1", y1); obj1.insert("z1", z1);
            uint hash1 = qHash(obj1);
            obj2.insert("x1", x2); obj2.insert("y1", y2); obj2.insert("z1", z2);
            uint hash2 = qHash(obj2);

            node tempNode1(x1, y1, z1, true);
            Node.insert(hash1, tempNode1);
            node tempNode2(x2, y2, z2, true);
            Node.insert(hash2, tempNode2);

            edge tempEdge1, tempEdge2;
            tempEdge1.setV1(&Node[hash1]);
            tempEdge1.setV2(&Node[hash2]);
            tempEdge2.setV1(&Node[hash2]);
            tempEdge2.setV2(&Node[hash1]);

            obj1.insert("x2", x2); obj1.insert("y2", y2); obj1.insert("z2", z2);
            obj2.insert("x2", x1); obj2.insert("y2", y1); obj2.insert("z2", z1);
            hash1 = qHash(obj1);
            hash2 = qHash(obj2);

            Edge.insert(hash1, tempEdge1);
            Edge.insert(hash2, tempEdge2);
        }
    }

    // Удаление дубликатов ребер
    QHash<uint, edge>::const_iterator it = Edge.constBegin();
    for (int i = 0; i < (int)Edge.size(); i++) {
        edge tempEdge = it.value();
        double x1 = tempEdge.getV1()->getX();
        double y1 = tempEdge.getV1()->getY();
        double z1 = tempEdge.getV1()->getZ();
        double x2 = tempEdge.getV2()->getX();
        double y2 = tempEdge.getV2()->getY();
        double z2 = tempEdge.getV2()->getZ();
        QJsonObject obj;
        obj.insert("x1", x2); obj.insert("y1", y2); obj.insert("z1", z2);
        obj.insert("x2", x1); obj.insert("y2", y1); obj.insert("z2", z1);
        uint hash = qHash(obj);
        Edge.remove(hash);
        ++it;
    }

    // Добавление узлов и ребер в векторы
    QHash<uint, node>::const_iterator in = Node.constBegin();
    int size = (int)Node.size();
    for (int i = 0; i < size; i++) {
        uint hash = in.key();
        nodes.push_back(&Node[hash]);
        ++in;
    }

    it = Edge.constBegin();
    size = (int)Edge.size();
    for (int i = 0; i < size; i++) {
        uint hash = it.key();
        edges.push_back(&Edge[hash]);
        ++it;
    }

    // Проверка и установка видимости треугольников
    QJsonArray qvisible;
    std::vector<bool> n_visible;
    int n_vis = 0;
    if (jsonObject.contains("visbleTriangles")) {
        qvisible = jsonObject.value("visbleTriangles").toArray();
        n_vis = qvisible.count();
        for (int i = 0; i < n_vis; i++) {
            n_visible.push_back(qvisible.at(i).toBool());
        }
    } else {
        return 2; // Ошибка: данные по освещенности отсутствуют
    }

    // Создание треугольников
    assert(tri.size() == n_visible.size());
    size = (int)tri.size();
    triangles.resize(size);
    for (int i = 0; i < size; i++) {
        triangles.at(i).setVisible(n_visible.at(i));
        for (int j = 0; j < 3; j++) {
            double x = tri[i][j][0];
            double y = tri[i][j][1];
            double z = tri[i][j][2];
            QJsonObject obj;
            obj.insert("x1", x); obj.insert("y1", y); obj.insert("z1", z);
            uint hash = qHash(obj);
            if (Node.contains(hash)) {
                node *tempNode = &Node[hash];
                tempNode->setVisible(triangles.at(i).getVisible());
                if (j == 0) {
                    triangles.at(i).setV1(tempNode);
                } else if (j == 1) {
                    triangles.at(i).setV2(tempNode);
                } else if (j == 2) {
                    triangles.at(i).setV3(tempNode);
                }
            }
        }
    }

    // Установка параметров радарной волны
    int freqband = -1;
    if (jsonObject.contains("freqBand")) {
        freqband = jsonObject.value("freqBand").toInt();
    }
    radar_wave wave1(freqband);

    int inc_polariz, ref_polariz;
    if (jsonObject.contains("polarRadiation")) {
        if (jsonObject.contains("polarRecive")) {
            inc_polariz = jsonObject.value("polarRadiation").toInt();
            ref_polariz = jsonObject.value("polarRecive").toInt();
        }
        int err = wave1.setPolariz(inc_polariz, ref_polariz, Nin, Ein);
        if (err > 0) {
            return 4; // Ошибка: поляризация некорректна
        }
    } else {
        return 4; // Ошибка: данные по поляризации отсутствуют
    }

    set_wave(2 * Pi / wave1.getLambda());

    // Установка типов углов для радиопортрета
    bool azimuth_radar_image, elevation_radar_image, range_radar_image;
    if (jsonObject.contains("typeAngle")) {
        elevation_radar_image = jsonObject.value("typeAngle").toBool();
    }
    if (jsonObject.contains("typeAzimut")) {
        azimuth_radar_image = jsonObject.value("typeAzimut").toBool();
    }
    if (jsonObject.contains("typeLength")) {
        range_radar_image = jsonObject.value("typeLength").toBool();
    }
    if (!(elevation_radar_image + azimuth_radar_image + range_radar_image)) {
        return 5; // Ошибка: тип радиопортрета не задан
    } else {
        set_boolXYZ(azimuth_radar_image, range_radar_image, elevation_radar_image);
        set_stepXYZ(wave1.getStepX(), wave1.getStepY(), wave1.getStepZ());
    }

    // Вычисление максимальной длины
    double Lmax = 0;
    rVect rmin = (rVect)get_Node(0);
    rVect rmax = (rVect)get_Node(0);
    node Node_;
    size_t nCount = getNodeSize();
    for (int iPoint = 1; iPoint < (int)nCount; iPoint++) {
        Node_ = get_Node(iPoint);
        if (rmin.getX() > Node_.getX()) rmin.setX(Node_.getX());
        if (rmin.getY() > Node_.getY()) rmin.setY(Node_.getY());
        if (rmin.getZ() > Node_.getZ()) rmin.setZ(Node_.getZ());
        if (rmax.getX() < Node_.getX()) rmax.setX(Node_.getX());
        if (rmax.getY() < Node_.getY()) rmax.setY(Node_.getY());
        if (rmax.getZ() < Node_.getZ()) rmax.setZ(Node_.getZ());
    }
    Lmax = (rmax - rmin).length();
    set_Lmax(Lmax);

    // Установка параметров отраженной волны
    if (jsonObject.contains("pplane")) {
        set_ref(jsonObject.value("pplane").toBool());
    }

    // Установка направления вектора
    QJsonObject qdirectv;
    rVect aEin;
    if (jsonObject.contains("directVector")) {
        qdirectv = jsonObject.value("directVector").toObject();
        double x = qdirectv.value("x").toDouble();
        double y = qdirectv.value("y").toDouble();
        double z = qdirectv.value("z").toDouble();
        rVect In(x, y, z);
        Nin = In;
        rVect Ref(Nin.getX(), Nin.getY(), -Nin.getZ());
        NinRef = Ref;
        rVect Out(-Nin.getX(), -Nin.getY(), -Nin.getZ());
        rVect OutRef(-Nin.getX(), -Nin.getY(), Nin.getZ());
        Nout = Out; NoutRef = OutRef;
    } else {
        return 3; // Ошибка: данные по направлению отсутствуют
    }

    RWave = wave1;

    // Сохранение модели в файл
    if (SAVE_MODEL_TO_FILE) {
        QFile file("model.json");
        QJsonObject model_obj;
        QJsonObject tr_obj;
        QJsonObject edge_obj;
        QJsonObject node_obj;
        QJsonObject dir_obj;
        QJsonObject pol_obj;
        size_t tr_size = triangles.size();
        for (size_t i = 0; i < tr_size; i++) {
            std::stringstream tr_str;
            tr_str << get_Triangle(i);
            QString tr_qstr;
            tr_qstr = QString::fromStdString(tr_str.str());
            tr_obj.insert(QString::number(i), tr_qstr);
        }
        size_t n_size = nodes.size();
        for (size_t i = 0; i < n_size; i++) {
            std::stringstream n_str;
            n_str << get_Node(i);
            QString n_qstr;
            n_qstr = QString::fromStdString(n_str.str());
            node_obj.insert(QString::number(i), n_qstr);
        }
        size_t edge_size = edges.size();
        for (size_t i = 0; i < edge_size; i++) {
            std::stringstream edge_str;
            edge_str << get_Edge(i);
            QString edge_qstr;
            edge_qstr = QString::fromStdString(edge_str.str());
            edge_obj.insert(QString::number(i), edge_qstr);
        }
        std::stringstream dir;
        dir << Nin;
        dir_obj.insert("direct vector", QString::fromStdString(dir.str()));
        QString pol;
        pol = QString::number(RWave.getIncPolariz());
        pol_obj.insert("wave incident polarization", pol);
        pol = QString::number(RWave.getRefPolariz());
        pol_obj.insert("wave receive polarization", pol);

        model_obj.insert("wave direct", dir_obj);
        model_obj.insert("wave polarization", pol_obj);
        model_obj.insert("triangles", tr_obj);
        model_obj.insert("nodes", node_obj);
        model_obj.insert("edges", edge_obj);

        QJsonDocument Model(model_obj);

        if (!file.open(QIODevice::WriteOnly)) {
            clogs("file model.json error to open", "", "");
        } else {
            clogs("save file model.json", "", "");
            file.resize(0);
            file.write(Model.toJson());
        }
        file.close();
        SAVE_MODEL_TO_FILE = false;
    }

    return 0;
}
Выполнение вычислений и отправка результатов
5. Выполнение расчетов:
В radarCore::calcRadar() вызывается culc_Eout() для выполнения основных вычислений.

void radarCore::calcRadar() {
    connect(this, &culcradar::signal_send_progress_bar_culcradar, this, &radarCore::sendProgressBar);

    Txt = "расчет радиопортрета...";
    sendText();
    int err;
    err = culc_Eout();
    if (err < 0) {
        Txt = "расчет радиопортрета завершен с ошибкой";
        sendText();
        throw -1;
    } else {
        Txt = "расчет радиопортрета завершен успешно";
        sendText();
    }

    disconnect(this, &culcradar::signal_send_progress_bar_culcradar, this, &radarCore::sendProgressBar);
    return;
}

6. Формирование результатов:
В radarCore::calcRadarResult() формируются результаты и отправляются клиенту.

void radarCore::calcRadarResult() {
    Txt = "формирование результата для передачи...";
    sendText();

    // Инициализация передаваемого json-документа
    QJsonObject Echo;
    QJsonDocument doc;
    Echo.insert("type", QJsonValue::fromVariant("result"));
    Echo.insert("id", QJsonValue::fromVariant(id));
    Echo.insert("content", QJsonValue::fromVariant("radioportrait"));

    // Размеры массивов
    uint size1 = getSizeEoutX();
    uint size2 = getSizeEoutY();
    uint size3 = getSizeEoutZ();

    // Инициализация json-массивов для заполнения полем рассеяния
    QJsonArray m_fft_absEout = {};
    QJsonArray m_fft_normEout = {};
    QJsonArray m_fft_Eout_base = {};
    QJsonArray m_fft_Eout_cross = {};
    cVect Eout_base, Eout_cross;
    rVect rEin = this->getEin();
    std::complex<double> dotE;

    // Вычисление границ объекта
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

    // Вычисление отступа от края портрета
    uint n1 = round((size1 - n) / 2);
    uint m1 = round((size2 - m) / 2);
    uint q1 = round((size3 - q) / 2);

    // Отмена ограничений разрешения картинки (для мелких объектов)
    if (size1 < 64) {
        n = size1; n1 = 0;
    }
    if (size2 < 64) {
        m = size2; m1 = 0;
    }
    if (size3 < 64) {
        q = size3; q1 = 0;
    }

    // Заполнение json-массивов полем рассеяния с учетом "обрезки"
    for (uint i = 0; i < size3; i++) {
        QJsonArray m_fft_absY = {};
        QJsonArray m_fft_normY = {};
        for (uint j = 0; j < size2; j++) {
            QJsonArray m_fft_absX = {};
            QJsonArray m_fft_normX = {};
            for (uint k = 0; k < size1; k++) {
                Eout_base = getEout(k + n1, j + m1, i + q1);
                dotE = dot(Eout_base, rEin);
                Eout_base = dotE * rEin;
                radar_wave RW = getRWave();
                if (RW.getIncPolariz() != RW.getRefPolariz()) {
                    Eout_cross = Eout_base - getEout(k + n1, j + m1, i + q1);
                    Eout_base = Eout_cross;
                }
                m_fft_absX.push_back(Eout_base.length());
                m_fft_normX.push_back(Eout_base.norm());
                if (k == n) break;
            }
            m_fft_absY.push_back(m_fft_absX);
            m_fft_normY.push_back(m_fft_normX);
            if (j == m) break;
        }
        m_fft_absEout.push_back(m_fft_absY);
        m_fft_normEout.push_back(m_fft_normY);
        if (i == q) break;
    }

    // Вставка в json-документ информации о размерах json-массивов
    if ((size3 == 1) && (size2 == 1) && (size1 > 1)) {
        Echo.insert("dimension_type", 0);
    } else if ((size3 == 1) && (size2 > 1) && (size1 == 1)) {
        Echo.insert("dimension_type", 0);
    } else if ((size3 > 1) && (size2 == 1) && (size1 == 1)) {
        Echo.insert("dimension_type", 0);
    } else if ((size3 > 1) && (size2 > 1) && (size1 == 1)) {
        Echo.insert("dimension_type", 1);
    } else if ((size3 > 1) && (size2 == 1) && (size1 > 1)) {
        Echo.insert("dimension_type", 1);
    } else if ((size3 == 1) && (size2 > 1) && (size1 > 1)) {
        Echo.insert("dimension_type", 1);
    } else if ((size3 > 1) && (size2 > 1) && (size1 > 1)) {
        Echo.insert("dimension_type", 2);
    } else {
        Echo.insert("dimension_type", -1);
    }

    Echo.insert("absEout", m_fft_absEout);
    Echo.insert("info_absEout", QString("fft result, absolute value"));
    Echo.insert("normEout", m_fft_normEout);
    Echo.insert("info_normEout", QString("fft result, norm value"));

    Txt = "передача результата клиенту";
    sendText();

    QJsonDocument doc1(Echo);
    emit send_result(doc1.toJson(), m_Client);

    // Сохранение сообщения клиенту в json и бинарный файл
    if (SAVE_MESSAGE_TO_FILE) {
        QFile file1("client_message.json");
        QFile file2("client_message.bin");
        if (!file1.open(QIODevice::WriteOnly)) {
            clogs("файл client_message.json не может быть открыт", "", "");
        } else {
            clogs("сохранение файла client_message.json", "", "");
            file1.resize(0);
            file1.write(doc1.toJson());
        }

        file1.close();

        if (!file2.open(QIODevice::WriteOnly)) {
            clogs("файл client_message.bin не может быть открыт", "", "");
        } else {
            clogs("сохранение файла client_message.bin", "", "");
            file2.resize(0);
            QDataStream stream(&file2);
            stream.setVersion(QDataStream::Qt_4_2);
            stream << doc1;
        }
        file2.close();
    }

    return;
}

Отправка результатов клиенту
7. Отправка результатов:
Результаты отправляются клиенту через сигнал send_result, который обрабатывается в WebServer::send_calc_radar_result().

void WebServer::send_calc_radar_result(QString doc, QWebSocket *Client) {
    clientAI *clientInfo = (clientAI *)Client->property("client_info").toULongLong();
    clogs("передача результата [" + clientInfo->id + "]", "", "");

    QJsonObject resultObj;
    resultObj.insert("type", QJsonValue::fromVariant("result"));
    resultObj.insert("content", QJsonDocument::fromJson(doc.toUtf8()).object());
    QJsonDocument resultDoc(resultObj);

    qDebug() << "Sending result to client:" << resultDoc.toJson(QJsonDocument::Compact);

    Client->sendTextMessage(resultDoc.toJson(QJsonDocument::Compact));
    webServerAnswer("вычисления окончены", Client);
}


Этап 1: Прием и Декодирование Сообщения
1. Получение Сообщения:
Сервер получает сообщение от клиента через WebSocket.
Сообщение должно быть в формате JSON.

2. Декодирование JSON-Объекта:
Сервер декодирует JSON-сообщение с помощью QJsonDocument::fromJson().
Если JSON-объект корректен, сервер извлекает его содержимое.

Этап 2: Обработка Входных Данных
3. Проверка Типа Сообщения:
Сервер проверяет поле type в JSON-объекте.
Если type равно "triangles", сервер понимает, что это данные для вычислений.

4. Проверка Аутентификации:
Если клиент не прошел аутентификацию, сервер отклоняет запрос.
Если клиент аутентифицирован, сервер продолжает обработку.

Этап 3: Загрузка и Интерпретация Данных
5. Загрузка Данных:
Сервер передает JSON-объект и информацию о клиенте в функцию, которая отвечает за загрузку данных в вычислительное ядро.

6.Создание Вычислительного Ядра:
Сервер создает объект вычислительного ядра (radarCore) и перемещает его в отдельный поток для асинхронной обработки.

7. Интерпретация JSON-Объекта:
Вычислительное ядро интерпретирует JSON-объект, извлекая из него необходимые данные:
Координаты узлов и ребер треугольников.
Видимость треугольников.
Частотные и поляризационные параметры.
Направление падения волны.

8. Построение Модели:
На основе извлеченных данных ядро строит внутренние структуры данных (узлы, ребра, треугольники).
Устанавливаются параметры волны, такие как длина волны и шаг по координатам.

Этап 4: Выполнение Вычислений
8. Выполнение Основных Вычислений:
Ядро запускает основные вычисления, используя методы, такие как FFT (быстрое преобразование Фурье).
В процессе вычислений ядро учитывает видимость треугольников и параметры волны.

9. Формирование Полей Рассеяния:
Ядро рассчитывает рассеянное поле и формирует трехмерные массивы данных.

Этап 5: Формирование и Отправка Результатов
10. Формирование Результатов:
Ядро обрабатывает результаты вычислений и формирует выходной JSON-объект.
В JSON-объект включаются:
Абсолютные значения рассеянного поля.
Нормы векторов рассеянного поля.
Дополнительная информация, такая как размеры массивов.

11. Отправка Результатов Клиенту:
Ядро отправляет сформированный JSON-объект обратно на сервер.
Сервер пересылает результаты клиенту через WebSocket.
Клиент получает результаты в формате JSON и может их использовать для дальнейшего анализа или визуализации.

Этап 6: Дополнительные Действия (при необходимости)
12. Сохранение Данных:
Сервер может сохранить входные и выходные данные в файлы (JSON или бинарные) для последующего анализа или отладки.

13. Обработка Команд:
Если клиент отправляет команды управления (например, пауза или продолжение), сервер обрабатывает их, изменяя состояние вычислительного ядра.

Детали Обработки JSON-Объекта с type: "triangles"
Ожидаемые Параметры JSON-Объекта:
data: Координаты узлов треугольников.
visbleTriangles: Массив, указывающий видимость каждого треугольника.
freqBand: Частотный диапазон.
polarRadiation и polarRecive: Параметры поляризации.
typeAngle, typeAzimut, typeLength: Типы углов для радиопортрета.
directVector: Направление падения волны.
Интерпретация и Обработка JSON-Объекта

Чтение и Валидация Данных:
Сервер читает координаты узлов и проверяет их корректность.
Проверяет наличие данных по видимости треугольников, частоте, поляризации и направлению волны.

Построение Модели:
На основе координат создаются узлы и ребра.
Узлы и ребра добавляются в хэш-таблицы для быстрой обработки.
Удаляются дублирующие ребра.

Создание Треугольников:
На основе узлов создаются треугольники.
Устанавливается видимость треугольников.

Установка Параметров Волны:
Устанавливаются параметры частотного диапазона и поляризации.
Устанавливается длина волны и шаг по координатам.

Выполнение Вычислений:
Производятся вычисления рассеянного поля.
Результаты вычислений сохраняются в трехмерные массивы.

Формирование и Отправка Результатов:
Формируются JSON-объекты с результатами вычислений.
Результаты отправляются клиенту через WebSocket.

Пример JSON-Объекта, Ожидаемого Сервером:
{
    "type": "triangles",
    "data": {
        "0": 0.1, "1": 0.2, "2": 0.3, "3": 0.4, "4": 0.5, "5": 0.6, "6": 0.7, "7": 0.8, "8": 0.9
    },
    "visbleTriangles": [true, false, true],
    "freqBand": 1,
    "polarRadiation": 0,
    "polarRecive": 1,
    "typeAngle": true,
    "typeAzimut": false,
    "typeLength": true,
    "directVector": {"x": 1.0, "y": 0.0, "z": 0.0}
}
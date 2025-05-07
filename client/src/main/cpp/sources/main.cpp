#include "mainwindow.h"
#include <QApplication>
#include <QIcon>

int main(int argc, char *argv[])
{

    QApplication a(argc, argv);

    // Иконка для приложения
    a.setWindowIcon(QIcon(":/icon.png"));

    // Создание и инициализация ConnectionManager
    ConnectionManager* connectionManager = new ConnectionManager(&a);

    // Создание MainWindow с передачей ConnectionManager
    MainWindow w(connectionManager);
    w.show();

    return a.exec();
}

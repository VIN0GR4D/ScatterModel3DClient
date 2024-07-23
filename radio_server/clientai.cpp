#include "clientai.h"
#include "calctools.h"

clientAI::clientAI(QObject *parent): QObject(parent)
{

}
void clientAI::auth() {
    if (!authStatus) {

        pClient->close();
        clogs("клиент "+id+" авторизацию на сервере не прошел","wrn","");
        delete this;
    }
}

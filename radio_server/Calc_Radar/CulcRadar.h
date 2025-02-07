#ifndef CULCRADAR_H
#define CULCRADAR_H

#include "Calc_Radar/Node.h"
#include "Calc_Radar/Edge.h"
#include "Calc_Radar/Triangle.h"
#include "Calc_Radar/Radar_Wave.h"
#include "rVect.h"
#include "cVect.h"
#include <vector>
#include <QJsonObject>
#include <QJsonArray>
#include "calctools.h"
#include "timer.h"

/*
Система координат выбрана таким образом, что плоскость XOY расположена горизонтально параллельно земной поверхности.
ось Z направлена вверх и оси XYZ образуют правую тройку.
Подстилающая поверхность (если она есть) лежит в плоскости Z = 0.
Все углы передаются в радианах.
*/

class culcradar : public QObject
{
    Q_OBJECT

public:
    culcradar(QObject *parent = 0); //конструктор

protected:
    Timer m_timer;         //таймер прогресс-бара
    int progress;          //значение прогресс-бара
    bool count = false;    //признак запуска прогресс-бара

public:
    bool SAVE_MODEL_TO_FILE;
    bool SCAT_FIELD_TO_FILE;
    bool FFT_FIELD_TO_FILE;
    bool RESULT_FROM_FILE;
signals:
    void signal_send_progress_bar_culcradar();

    //public slots:
    //    void slot_send_progress_bar_culcradar() {
    //        qDebug() << "I'm here!";
    //        emit(signal_send_progress_bar_culcradar());
    //    }

private:
    bool RUN_C; //признак работы
    bool ref;  //признак подстилающий поверхности
    //	double phi=0., theta=0.;// ракурс. Углы направления на объект. theta УГОЛ МЕСТА
    rVect Nin, Nout, NinRef, NoutRef;
    bool boolX, boolY, boolZ; //по каким осям строится РЛП
    //при изменении разрешения или максимального разменр вычисляем и размерности массива
    double Lmax; //максимальный размер объекта
    double stepX, stepY, stepZ; //разрешение по осям координат
    int countX, countY, countZ; //размерность массива РЛП
    double wave; //волновое число
    double stepW; //шаг по волновым числам
    //геометрическая модель
    vector<edge*> edges;
    vector<triangle> triangles;
    vector<node*> nodes;

    //падающее поле
    rVect Ein;

    //рассеянное поле
    vector<vector<vector<cVect>>> vEout;

    //параметры радара
    radar_wave RWave;


public:
    void Exit() {RUN_C = false;}
    void set_ref(bool Ref) { ref=Ref; } //подстилающая поверхность
    //	void set_phi(double Phi) { phi = Phi; }
    //	void set_theta(double Theta) { theta = Theta;}
    void set_boolXYZ(bool X, bool Y, bool Z);
    void set_boolX(bool X);
    void set_boolY(bool Y);
    void set_boolZ(bool Z);
    //при изменении разрешения или максимального разменр вычисляем и размерности массива
    void set_Lmax(double L); // { Lmax = L; }
    void set_stepXYZ(double x, double y, double z); // { stepX = x; stepY = y; stepZ = z; }
    void set_stepX(double x) {
        if (!boolX)
            stepX = 0;
        else
            stepX = x;
    }
    void set_stepY(double y) {
        if (!boolY)
            stepY = 0;
        else
            stepY = y;
    }
    void set_stepZ(double z) {
        if (!boolZ)
            stepZ = 0;
        else
            stepZ = z;
    }
    void set_wave(double W) { wave = W;}
    bool get_boolX() { return boolX; }
    bool get_boolY() { return boolY; }
    bool get_boolZ() { return boolZ; }
    double get_Lmax() { return Lmax; }
    double get_stepX() { return stepX; }
    double get_stepY() { return stepY; }
    double get_stepZ() { return stepZ; }
    double get_wave() { return wave; }

private:
    //выбор размерности массива и шага по волновым числам это отдельная песня
    void culc_count();// countX, countY, countZ; stepW;

public:
    //загрузку геометрической модели пока производим из файла obj потом из JSON
    int build_Model(QJsonObject &jsonObject, QHash<uint, node> &Node, QHash<uint,edge> &Edge);
    triangle get_Triangle(size_t iTriangle);
    edge get_Edge(size_t iEdge);
    node get_Node(size_t iNode);

    size_t getNodeSize() { return nodes.size(); }
    size_t getEdgeSize() { return edges.size(); }
    size_t getTriangleSize() { return triangles.size(); }

    bool get_ref() { return ref; }
    void built_Ns_in(double phi, double theta);
    void built_Ns_out(double phi, double theta);
    rVect get_Nin() { return Nin; }
    rVect get_Nout() { return Nout; }
    rVect get_NinRef() { return NinRef; }
    rVect get_NoutRef() { return NoutRef; }

    complex<double> dot(cVect& p1, rVect& p2) { //скалярное произведение комплексного на координатный вектор
        return ((p1.getX() * p2.getX())+(p1.getY() * p2.getY())+(p1.getZ() * p2.getZ()));
    }

    //падающее поле
    rVect getEin() { return Ein; }
    void setEin(rVect val) { Ein = val; }

    //рассеянное поле
    cVect getEout(size_t iX, size_t iY, size_t iZ);
    void setEout(size_t iX, size_t iY, size_t iZ, cVect Eout);
private:
    void setSizeEout(size_t iX, size_t iY, size_t iZ);
public:
    int getSizeEoutX() { return vEout[0][0].size(); }
    int getSizeEoutY() { return vEout[0].size(); }
    int getSizeEoutZ() { return vEout.size(); }

    radar_wave getRWave() {return RWave;}
    //запуск задачи вычисления поля по ФО
    //углы задаются в радианах. От клиента приходит угол места, polar это полярный угол
    int culc_Eout(/*bool Aref, double Aphi, double Atheta,
        bool AboolX, bool AboolY, bool AboolZ, double aLmax,
        double AstepX, double AstepY, double AstepZ, double Awave,
        rVect aEin*/);
};

#endif // CULCRADAR_H

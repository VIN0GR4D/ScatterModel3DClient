#ifndef RADAR_WAVE_H
#define RADAR_WAVE_H
#include "Calc_Radar/rVect.h"

class radar_wave {
public:
    //constructor
    radar_wave(): m_freqband(0),lambda(0),m_stepX(0),m_stepY(0),m_stepZ(0),
    m_inc_polariz(0),m_ref_polariz(0){}
    radar_wave(int freqband);
private:
    //Параметры падающей волны:
    int m_freqband;
    double lambda;
    //Параметры рассеянного поля:
    double m_stepX;      //шаг по осям координат
    double m_stepY;
    double m_stepZ;
    int m_inc_polariz;    //0-вертик., 1 - гориз.
    int m_ref_polariz;    //0-вертик., 1 - гориз.
public:
    int setPolariz(int &inc_polar, int &ref_polar, rVect &Nin, rVect &Ein);
    int getIncPolariz() {return m_inc_polariz;}
    int getRefPolariz() {return m_ref_polariz;}
    double getLambda(){return lambda;}
    double getStepX() {return m_stepX;}
    double getStepY() {return m_stepY;}
    double getStepZ() {return m_stepZ;}
};

#endif // RADAR_WAVE_H

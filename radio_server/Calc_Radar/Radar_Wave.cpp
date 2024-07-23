#include "Calc_Radar/Radar_Wave.h"
#include "Calc_Radar/ConstAndVar.h"

radar_wave::radar_wave(int freqband) {
    m_freqband = freqband;
    double c = 2.99792458e8;
    double f0, fmin, fmax;

    if (freqband == 0) {    //P
        fmin = 400e6;
        fmax = 450e6;
        m_stepX = 1.5;
        m_stepY = 3.0;
    }
    else if (freqband == 1) {//L
        fmin = 1e9;
        fmax = 1.5e9;
        m_stepX = 0.5;
        m_stepY = 1.0;
    }
    else if (freqband == 2) {//S
        fmin = 2.75e9;
        fmax = 3.15e9;
        m_stepX = 0.5;
        m_stepY = 1.0;
    }
    else if (freqband == 3) {//C
        fmin = 5e9;
        fmax = 5.5e9;
        m_stepX = 0.5;
        m_stepY = 1.0;
    }
    else if (freqband == 4) {//X
        fmin = 9e9;
        fmax = 10e9;
        m_stepX = 0.25;
        m_stepY = 0.5;
    }
    else if (freqband == 5) {//Ka
        fmin = 36.5e9;
        fmax = 38.5e9;
        m_stepX = 0.125;
        m_stepY = 0.25;
    }

    m_stepZ = m_stepX; //азимут. и угломестн. разрешение одинаковы
    f0 = (fmin + fmax)/2;
    lambda = c/f0;     //m
}

int radar_wave::setPolariz(int &inc_polar, int &ref_polar, rVect &Nin, rVect &Ein) {
    //inc_polar = 0 - вертикальная поляризация
    //inc_polar = 1 - горизонтальная поляризация
    //inc_polar = 2 - круговая поляризация

    m_inc_polariz = inc_polar;
    m_ref_polariz = ref_polar;
    double x = Nin.getX();
    double y = Nin.getY();
    double z = Nin.getZ();

    double delta;
    double phi, theta;
    if (y > 0) delta = 0;
    if (y < 0) delta = 1;
    if ((x!=0) && (y!=0)) {
       phi = atan2(y,x) + Pi*delta;
    }
    else {
        phi = 0;
    }
    theta = Pi - (acos(z/(Nin.length())));
//    double theta_gr = theta*180/Pi;
//    double phi_gr = phi*180/Pi;

    rVect Einv, Eing;

    double theta1 = theta + Pi/2;

    double cosphi(cos(phi));
    double sinphi(sin(phi));
    double costheta(cos(theta1));
    double sintheta(sin(theta1));

    Einv.setPoint(-cosphi*sintheta, -sinphi * sintheta, -costheta);
    Eing.setPoint(sinphi * sintheta, -cosphi*sintheta, 0);

//    double dot1 = Eing*Einv;
//    double dot2 = Nin*Eing;
//    double dot3 = Nin*Einv;

    if (inc_polar == 0) {
        Ein = Einv;
    }
    else if (inc_polar == 1) {
        Ein = Eing;
    }
    else return 1;

    //нормировка вектора
    double length = Ein.length();
    Ein.setX(Ein.getX()/length);
    Ein.setY(Ein.getY()/length);
    Ein.setZ(Ein.getZ()/length);

    return 0;
}

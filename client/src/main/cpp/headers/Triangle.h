#pragma once

#include <QSharedPointer>
#include <complex>
#include "rVect.h"
#include "Node.h"
#include "ConstAndVar.h"
#include "cVect.h"

class triangle {
private:
    QSharedPointer<node> m_V1;
    QSharedPointer<node> m_V2;
    QSharedPointer<node> m_V3;
    bool m_visible;

public:
    triangle(bool visible = true,
             QSharedPointer<node> V1 = nullptr,
             QSharedPointer<node> V2 = nullptr,
             QSharedPointer<node> V3 = nullptr)
        : m_V1(V1), m_V2(V2),  m_V3(V3), m_visible(visible) {}

    friend std::ostream& operator<<(std::ostream& out, const triangle& Tr) {
        out << Tr.m_visible << ";" << Tr.m_V1.get() << ";" << Tr.m_V2.get() << ";" << Tr.m_V3.get();
        return out;
    }

    void setV1(QSharedPointer<node> V1) { m_V1 = V1; }
    void setV2(QSharedPointer<node> V2) { m_V2 = V2; }
    void setV3(QSharedPointer<node> V3) { m_V3 = V3; }
    void setVisible(bool V) { m_visible = V; }

    QSharedPointer<node> getV1() const { return m_V1; }
    QSharedPointer<node> getV2() const { return m_V2; }
    QSharedPointer<node> getV3() const { return m_V3; }
    bool getVisible() const { return m_visible; }

    rVect culcNormal() {
        rVect Normal = (*m_V2 - *m_V1) ^ (*m_V3 - *m_V1);
        return 1.0 / Normal.length() * Normal;
    }

//????????????? ????????
    //??????????? ????????????? ???????? (?????????????? ?????) ?? ????????????
    complex<double> Difraction(rVect Nin, rVect Nout, double wave)
    {
        const double delta=1.e-3;  //???????? ?????????? ???????????? ?????
        rVect v2=*m_V2-*m_V1;
        rVect v3=*m_V3-*m_V1;
        double a = wave*((Nin-Nout)*v3);
        double b = wave*((Nin-Nout)*v2);
        double c = (v2^v3).length();
        complex<double> phase = wave / (2 * Pi) * exp(OneI * wave * ((Nin - Nout) * (*m_V1)));
//		complex<double> phase=exp(OneI*Wave*((Nin-Nout)*(*m_V1)));
        complex<double> res;

        if ((abs(a) <= delta) && (abs(b) <= delta) && (abs(a - b) <= delta))
            res = 0.5 * c * (1. + 0.5 * (a + b));
        else  if ((abs(a) > delta) && (abs(b) > delta) && (abs(a - b) <= delta))
            res = c / (a * b) * (exp(OneI * b) * (1. - OneI * b) - 1.);
        //			res = c / (a * b) * (exp(OneI * b) - 1.);
        //res = c / (a * b) * exp(OneI * b) * OneI * (a - b);
        else if ((abs(a) > delta) && (abs(b) <= delta))
            res = c / ((a - b) * a) * (1. - exp(OneI * a) + OneI * a);
        //	res = c / ((a - b) * a) * (-exp(OneI * a) - OneI * a);
        else if ((abs(a) <= delta) && (abs(b) > delta))
            res = c / ((a - b) * b) * (exp(OneI * b) - OneI * b - 1.);
            //res = c / ((a - b) * b) * (exp(OneI * b) - OneI * b);
        else
            res = c / (a * b) * ((a * exp(OneI * b) - b * exp(OneI * a)) / (a - b) - 1.);
/*		if (abs(res) > 0.5 * c)
        {
            complex<double> arg = res / abs(res);
            res = 0.5 * c;
        }*/
        return phase * res;
    }//Difraction1


//????????? ???????????
    //????????? ??????????? ??????????? ????
    cVect CulcPolarization(rVect Nin, rVect Nout, //??????????? ??????? ? ?????????
                         rVect p0)                //??????????? ??????. ???.
    {
        double len=p0.length();
        rVect s0 = 1./len*p0;
        rVect ey = culcNormal();
        rVect de = (Nout*p0)*s0;
        rVect dh = Nout^s0;
        cVect res;
        rVect temp = (len * ((ey ^ dh) - ((ey * de) * Nout)));

        return cVect(temp.getX(), temp.getY(), temp.getZ());
//		return ((Nin*ey)*p0)-((p0*ey)*Nin);
    }//CulcPolarization

//????????? ? ?????? ???????????
    cVect PolarDifraction(rVect Nin, rVect Nout, //??????????? ??????? ? ?????????
                         rVect p0,                //??????????? ??????. ???.)
                         double wave)				//???????? ?????
    {
        return Difraction(Nin, Nout, wave) * p0;// CulcPolarization(Nin, Nout, p0);
    }
};

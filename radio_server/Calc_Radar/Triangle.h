#pragma once
using namespace std;
#include <complex>
#include <vector>
#include "rVect.h"
#include "Node.h"
//#include "variables.h"
#include "Calc_Radar/ConstAndVar.h"
#include "cVect.h"
//#include "CPUFFT.h"

/**/
//const double Pi = 3.141592653589793;
//const complex<double> OneI(0., 1.);

class triangle
{
private:
	node* m_V1;
	node* m_V2;
	node* m_V3;
	bool m_visible;
public:
	triangle(bool visible=true, 
		node* V1 = nullptr, node* V2 = nullptr, node* V3 = nullptr):
    m_V1(V1),  m_V2(V2) ,m_V3(V3), m_visible(visible) {}
//ѕерегрузка оператора вывода
	friend std::ostream& operator<<(std::ostream& out, const triangle& Tr)
	{
        node n1,n2,n3;
        n1 = *Tr.m_V1; n2 = *Tr.m_V2; n3 = *Tr.m_V3;
        out << Tr.m_visible << ";" << n1 << ";" << n2 <<";" << n3;
		return out;
	}
	void setV1(node* V1) {m_V1=V1;}
	void setV2(node* V2) {m_V2=V2;}
	void setV3(node* V3) {m_V3=V3;}
	node* getV1() {return m_V1;}
	node* getV2() {return m_V2;}
	node* getV3() {return m_V3;}
    bool getVisible() {return m_visible;}
    void setVisible(bool visible){ m_visible = visible;}

	rVect culcNormal() 
	{	rVect Normal=(*m_V2-*m_V1)^(*m_V3-*m_V1);
		return 1./Normal.length()*Normal;
	}

//дифракционный интеграл
	//вычисл€етс€ дифракционный интеграл (преобразование ‘урье) от треугольника
	complex<double> Difraction(rVect Nin, rVect Nout, double wave)
	{
		const double delta=5.e-3;  //точность вычислени€ вещественных чисел
		rVect v2=*m_V2-*m_V1; 
		rVect v3=*m_V3-*m_V1;
		double a = wave*((Nin-Nout)*v3);
		double b = wave*((Nin-Nout)*v2);
		double c = (v2^v3).length();
		complex<double> phase=wave/(2*Pi)*exp(OneI*wave*((Nin-Nout)*(*m_V1)));
//		complex<double> phase=exp(OneI*Wave*((Nin-Nout)*(*m_V1)));
		complex<double> res;

		if ((abs(a)<=delta)&&(abs(b)<=delta)&&(abs(a-b)<=delta))
			return phase*0.5*c*(1.+0.5*(a+b));
		else  if ((abs(a)>delta)&&(abs(b)>delta)&&(abs(a-b)<=delta))
			return phase*c/(a*b)*(exp(OneI*b)*(1.-OneI*b)-1.);
//		return phase*c/(a*b)*exp(OneI*b)*OneI*(a-b);
		else if ((abs(a)>delta)&&(abs(b)<=delta))
			return phase*c/((a-b)*a)*(1.-exp(OneI*a)-OneI*a);
		else if ((abs(a)<=delta)&&(abs(b)>delta))
			return phase*c/((a-b)*b)*(exp(OneI*b)-OneI*b-1.);
		else 
			return phase*c/(a*b)*((a*exp(OneI*b)-b*exp(OneI*a))/(a-b)-1.);
	
	}//Difraction

//выисление пол€ризации
	//вычисл€ет пол€ризацию рассе€нного пол€
	rVect CulcPolarization(rVect Nin, rVect Nout, //направление падени€ и рассе€ни€
						 rVect p0)                //пол€ризаци€ падающ. изл.
	{
		double len=p0.length();
		rVect s0 = 1./len*p0;
		rVect ey = culcNormal();
		rVect de = (Nout*p0)*s0;
		rVect dh = Nout^s0;
		return len*((ey^dh) - ((ey*de)*Nout));
//		return ((Nin*ey)*p0)-((p0*ey)*Nin);
	}//CulcPolarization

//рассе€ние с учетом пол€ризации
	cVect PolarDifraction(rVect Nin, rVect Nout, //направление падени€ и рассе€ни€
						 rVect p0,                //пол€ризаци€ падающ. изл.)
						 double wave)				//волновое число
	{
		return Difraction(Nin, Nout, wave)*CulcPolarization(Nin, Nout, p0);
	}
};

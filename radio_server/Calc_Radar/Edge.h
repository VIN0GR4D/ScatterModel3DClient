#pragma once
//#include "stdafx.h"
#include <complex>
#include <vector>
#include "rVect.h"
//#include "variables.h"
#include "Triangle.h"
#include "cVect.h"

using namespace std;

class edge
{
	private:
	node* m_V1;
	node* m_V2;
    std::vector<triangle*> m_triangles;
	bool m_visible;
public:
	edge(bool visible=true, 
		node* V1 = nullptr, node* V2 = nullptr):
	m_visible(visible), m_V1(V1),  m_V2(V2){}

	//Перегрузка оператора вывода
	friend std::ostream& operator<<(std::ostream& out, const edge& Ed)
	{
        node n1,n2;
        n1 = *Ed.m_V1; n2 = *Ed.m_V2;
        out << Ed.m_visible << ";" << n1 << ";" << n2;
		return out;
	}
    void push_triangle(triangle * tri) {m_triangles.push_back(tri);}
	void setV1(node* V1) {m_V1=V1;}
	void setV2(node* V2) {m_V2=V2;}
	node* getV1() {return m_V1;}
	node* getV2() {return m_V2;}
    void setVisible(bool v) {m_visible = m_visible + v;}

	cVect culcDifraction(triangle Tr1, triangle Tr2,
		rVect Nin, rVect Nout, //направление падения и рассеяния
		rVect p0);                //поляризация падающ. изл.
};

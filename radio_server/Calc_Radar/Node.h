#pragma once
//#include <iostream>
#include "rVect.h"


class node : public rVect
{
private:
	bool m_visible;
public:
	//Конструктор по умолчанию
	node() :rVect(), m_visible(true) {}
	//Специфический конструктор
	node(double x, double y, double z, bool visible=true)
		: rVect(x, y, z), m_visible(true) {}
	//Перегрузка оператора вывода
//    friend std::ostream& operator<<(std::ostream& out, const node& point)
//	{
//		out << "(" << point <<";" << point.m_visible << ")";
//		return out;
//    }
	void setVisible(bool visible) {m_visible = visible;}
	bool getVisible() {return m_visible;}
}; //class node

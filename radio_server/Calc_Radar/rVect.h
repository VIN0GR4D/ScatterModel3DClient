#pragma once
#include <iostream>
#include <math.h>
#include "cVect.h"

//#include "variables.h"
//#include "stdafx.h"
using namespace std;

class rVect
{
private:
	double m_x;
	double m_y;
	double m_z;

public:
	//Конструктор по умолчанию
	rVect() : m_x(0), m_y(0), m_z(0) {}
	//Специфический конструктор
	rVect(double x, double y, double z)
		: m_x(x), m_y(y), m_z(z) {}
	//Перегрузка оператора вывода
	friend std::ostream& operator<<(std::ostream& out, const rVect& point)
	{
		out << "(" << point.m_x << "," << point.m_y << "," << point.m_z << ")";
		return out;
	}
	friend bool operator == (const rVect& p1, const rVect& p2) { //перегрузка оператора сравнения
		return ((p1.m_x == p2.m_x) && (p1.m_y == p2.m_y) && (p1.m_z == p2.m_z));
	}
	friend bool operator != (const rVect& p1, const rVect& p2) { //перегрузка оператора сравнения
		return !((p1.m_x == p2.m_x) && (p1.m_y == p2.m_y) && (p1.m_z == p2.m_z));
	}
	friend bool operator < (const rVect& p1, const rVect& p2) { //перегрузка оператора сложения
		return ((p1.m_x < p2.m_x) || (p1.m_y < p2.m_y) || (p1.m_z < p2.m_z));
	}
	friend rVect operator + (const rVect& p1, const rVect& p2) { //перегрузка оператора сравнения
		return rVect((p1.m_x + p2.m_x) , (p1.m_y + p2.m_y) , (p1.m_z + p2.m_z));
	}
	friend rVect operator - (const rVect& p1, const rVect& p2) { //перегрузка оператора сравнения
		return rVect((p1.m_x - p2.m_x), (p1.m_y - p2.m_y), (p1.m_z - p2.m_z));
	}
//скалярное произведение
	friend double operator * (const rVect& p1, const rVect& p2) { //перегрузка оператора сравнения
		return ((p1.m_x * p2.m_x)+(p1.m_y * p2.m_y)+(p1.m_z * p2.m_z));
	}
//векторное произведение
	friend rVect operator ^ (const rVect& p1, const rVect& p2) { //перегрузка оператора сравнения
		return rVect((p1.m_y * p2.m_z - p1.m_z * p2.m_y), 
			           (p1.m_z * p2.m_x - p1.m_x * p2.m_z), 
					   (p1.m_x * p2.m_y - p1.m_y * p2.m_x));
	}
//умножение на скаляр
	friend rVect operator * (const double p1, const rVect& p2) { //перегрузка оператора сравнения
		return rVect((p1 * p2.m_x), (p1 * p2.m_y), (p1 * p2.m_z));}

//умножение вещественного вектора на комплексное число
	friend cVect operator * (const complex<double> p1, const rVect p2) { //перегрузка оператора сравнения
		return cVect((p1 * p2.m_x), (p1 * p2.m_y), (p1 * p2.m_z));}

	bool IsZero(double eps=1e-3) {return m_x*m_x+m_y*m_y+m_z*m_z<eps*eps;}
	bool IsEqual(rVect p, double eps=1e-3) {
		return (pow(p.m_x-m_x,2)+pow(p.m_y-m_y,2)+pow(p.m_z-m_z,2)<eps*eps);
	}
    double getX() { return m_x; }
    double getY() { return m_y; }
    double getZ() { return m_z; }
	//Функция доступа
	void setPoint(double x, double y, double z)
	{
		m_x = x;
		m_y = y;
		m_z = z;
	}
	void setX(double x) {m_x = x;}
	void setY(double y) {m_y = y;}
	void setZ(double z) {m_z = z;}
	double norm() {return m_x * m_x + m_y * m_y + m_z * m_z;}
	double length() {return sqrt(norm());}
	void fromSphera(double aModulus, double aPhi, double aTheta)
	{
		double tempsinT = sin(aTheta);
		m_x = aModulus * cos(aPhi) * tempsinT;
		m_y = aModulus * sin(aPhi) * tempsinT;
		m_z = aModulus * cos(aTheta);
	}
};

const rVect rVectZero(0., 0., 0.);
const rVect rVectX(1., 0., 0.);
const rVect rVectY(0., 1., 0.);
const rVect rVectZ(0., 0., 1.);
const rVect rVectXY(1., 1., 0.);
const rVect rVectXZ(1., 0., 1.);
const rVect rVectYZ(0., 1., 1.);
const rVect rVectOne(1., 1., 1.);




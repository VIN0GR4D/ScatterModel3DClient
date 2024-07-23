#pragma once
#include <complex>
//#include "rVect.h"
using namespace std;

class cVect
{
private:
	complex<double> m_x;
	complex<double> m_y;
	complex<double> m_z;
public:
	//Конструктор по умолчанию
	cVect() : m_x(0), m_y(0), m_z(0) {}
	//Специфический конструктор
	cVect(complex<double> x, complex<double> y, complex<double> z)
		: m_x(x), m_y(y), m_z(z) {}
//Перегрузка оператора вывода
	friend std::ostream& operator<<(std::ostream& out, const cVect& point)
	{
		out << "(" << point.m_x << "," << point.m_y << "," << point.m_z << ")";
		return out;
	}
//сложение и вычитание
	friend cVect operator + (const cVect& p1, const cVect& p2) { //перегрузка оператора сравнения
		return cVect((p1.m_x + p2.m_x) , (p1.m_y + p2.m_y) , (p1.m_z + p2.m_z));
	}
	friend cVect operator - (const cVect& p1, const cVect& p2) { //перегрузка оператора сравнения
		return cVect((p1.m_x - p2.m_x), (p1.m_y - p2.m_y), (p1.m_z - p2.m_z));
	}

//скалярное произведение
	friend complex<double> operator * (const cVect& p1, const cVect& p2) { //перегрузка оператора сравнения
		return ((p1.m_x * p2.m_x)+(p1.m_y * p2.m_y)+(p1.m_z * p2.m_z));
	}
//векторное произведение
	friend cVect operator ^ (const cVect& p1, const cVect& p2) { //перегрузка оператора сравнения
		return cVect((p1.m_y * p2.m_z - p1.m_z * p2.m_y), 
			         (p1.m_z * p2.m_x - p1.m_x * p2.m_z), 
					 (p1.m_x * p2.m_y - p1.m_y * p2.m_x));
	}
//умножение на скаляр
	friend cVect operator * (const complex<double> p1, const cVect& p2) { //перегрузка оператора сравнения
		return cVect((p1 * p2.m_x), (p1 * p2.m_y), (p1 * p2.m_z));}


	
	const complex<double> getX() { return m_x; }
	const complex<double> getY() { return m_y; }
	const complex<double> getZ() { return m_z; }

//Функция доступа
	void setPoint(complex<double> x, complex<double> y, complex<double> z)
	{
		m_x = x;
		m_y = y;
		m_z = z;
	}
	void setX(complex<double> x) {m_x = x;}
	void setY(complex<double> y) {m_y = y;}
	void setZ(complex<double> z) {m_z = z;}

	cVect conj() {
		return cVect(std::conj(m_x), std::conj(m_y), std::conj(m_z));
	}

	double norm() {complex<double> cres = m_x * std::conj(m_x) + m_y * std::conj(m_y) + m_z * std::conj(m_z);
	return std::real(cres);}
	double length() {return sqrt(norm());}
};

const cVect cVectZero(0., 0., 0.);
const cVect cVectX(1., 0., 0.);
const cVect cVectY(0., 1., 0.);
const cVect cVectZ(0., 0., 1.);
const cVect cVectXY(1., 1., 0.);
const cVect cVectXZ(1., 0., 1.);
const cVect cVectYZ(0., 1., 1.);
const cVect cVectOne(1., 1., 1.);
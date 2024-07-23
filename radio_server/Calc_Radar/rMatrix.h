#pragma once

#include "rVect.h"
#include "cVect.h"

using namespace std;

class rMatrix
{
private:
	double m[3][3];

public:
	rMatrix()
	{
		m[0][0] = 1.; m[0][1] = 0.; m[0][2] = 0.;
		m[1][0] = 0.; m[1][1] = 1.; m[1][2] = 0.;
		m[2][0] = 0.; m[2][1] = 0.; m[2][2] = 1.;
	}

	double get_m(int col, int row) { return m[col][row]; }

	void set_m(double val, int col, int row) { m[col][row] = val; }

	//умножение матрицы на вектор
	friend rVect operator * (const rMatrix& p1, rVect& p2)
	{
		rVect res;
		res.setX(p1.m[0][0] * p2.getX() + p1.m[0][1] * p2.getY() + p1.m[0][2] * p2.getZ());
		res.setY(p1.m[1][0] * p2.getX() + p1.m[1][1] * p2.getY() + p1.m[1][2] * p2.getZ());
		res.setZ(p1.m[2][0] * p2.getX() + p1.m[2][1] * p2.getY() + p1.m[2][2] * p2.getZ());
		return res;
	}


	void CreateRotationMatrix(rVect anAxis, double angle)
	{
		double sine = sin(angle);
		double cosine = cos(angle);
		double one_minus_cosine = 1 - cosine;
		double len = anAxis.length();
		if (len)
		{
			rVect axis = 1 / len * anAxis;
			m[0][0] = one_minus_cosine * axis.getX() * axis.getX() + cosine;
			m[1][0] = one_minus_cosine * axis.getX() * axis.getY() - axis.getZ() * sine;
			m[2][0] = one_minus_cosine * axis.getX() * axis.getZ() + axis.getY() * sine;

			m[0][1] = one_minus_cosine * axis.getY() * axis.getX() + axis.getZ() * sine;
			m[1][1] = one_minus_cosine * axis.getY() * axis.getY() + cosine;
			m[2][1] = one_minus_cosine * axis.getY() * axis.getZ() - axis.getX() * sine;

			m[0][2] = one_minus_cosine * axis.getZ() * axis.getX() - axis.getY() * sine;
			m[1][2] = one_minus_cosine * axis.getZ() * axis.getY() + axis.getX() * sine;
			m[2][2] = one_minus_cosine * axis.getZ() * axis.getZ() + cosine;
		}
	}
}; //end class


rMatrix transpon(rMatrix matr)
{
	size_t col, row;
	rMatrix res;
	double temp;
	for (col = 0; col < 3; col++)
	{
		for (row = 0; row < 3; row++)
		{
			temp = matr.get_m(col, row);
			res.set_m(temp, row, col);
		}
	}
	return res;
}

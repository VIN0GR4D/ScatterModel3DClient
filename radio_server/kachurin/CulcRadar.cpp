#pragma once

#include "CulcRadar.h"
#include "CPUFFT.h"
#include "VectFFT.h"
#include "rVect.h"
#include "rMatrix.h"
#include <fstream>
#include <wtypes.h>
#include <string>

culcradar::culcradar()
{
	Nin = rVectY; 
	Nout = -1 * Nin;
	NinRef = rVectY;
	NoutRef=Nout;
	boolX = false;
	boolY = false;
	boolZ = false;
	Lmax=0.; //максимальный размер объекта
	stepX = 0.; stepY = 0.; stepZ = 0.; //разрешение по ос€м координат
	countX = 0; countY = 0; countZ = 0; //размерность массива –Ћѕ
	wave=0.; //волновое число
	stepW=0; //шаг по волновым числам
	edges.clear();
	triangles.clear();
	nodes.clear();
	Ein.setPoint(1., 0., 0.);
	vEout.clear();
}

//определ€ем размерности массива
void culcradar::culc_count()
{
	if (stepX)
		countX =2 * Lmax / stepX;
	else
		countX = 1;
	countX = (int)pad2((unsigned int)countX);
		
	if (stepY)
	{ 
		countY = 2 * Lmax / stepY;
		countY = (int)pad2((unsigned int)countY);
		stepW = 6. /(1. * countY*stepY);
	}
	else
		countY = 1;
		
	
	if (stepZ)
		countZ = 2 * Lmax / stepZ;
	else
		countZ = 1;
	countZ = (int)pad2((unsigned int)countZ);
	setSizeEout(countX, countY, countZ);
}

void culcradar::built_Ns_in(double phi, double theta)
{
	double cosphi(cos(phi));
	double sinphi(sin(phi));
	double costheta(cos(theta));
	double sintheta(sin(theta));
	Nin.setPoint(cosphi * sintheta, sinphi * sintheta, -costheta);
	NinRef.setPoint(cosphi * sintheta, sinphi * sintheta, costheta);
}

void culcradar::built_Ns_out(double phi, double theta)
{
	double cosphi(cos(phi));
	double sinphi(sin(phi));
	double costheta(cos(theta));
	double sintheta(sin(theta));
	Nout.setPoint(-cosphi * sintheta, -sinphi * sintheta, costheta);
	NoutRef.setPoint(-cosphi * sintheta, -sinphi * sintheta, -costheta);
}

void culcradar::set_Lmax(double L)
{
	Lmax = L;
	culc_count();
}

void culcradar::set_stepXYZ(double x, double y, double z)
{
	if (!boolX)
		stepX = 0;
	else
		stepX = x;

	if (!boolY)
		stepY = 0;
	else
		stepY = y;

	if (!boolZ)
		stepZ = 0;
	else
		stepZ = z;

	culc_count();
}

void culcradar::set_boolXYZ(bool X, bool Y, bool Z) 
{ 
	boolX = X;
	if (!boolX)
		stepX = 0;
	boolY = Y;
	if (!boolY)
		stepY = 0;
	boolZ = Z;
	if (!boolZ)
		stepZ = 0;
	culc_count();
}


void culcradar::set_boolX(bool X) 
{ 
	boolX = X;
	if (!boolX)
	{
		stepX = 0;
		culc_count();
	}
}

void culcradar::set_boolY(bool Y) 
{ 
	boolY = Y;
	if (!boolY)
	{
		stepY = 0;
		culc_count();
	}
}

void culcradar::set_boolZ(bool Z) 
{ 
	boolZ = Z;
	if (!boolZ)
	{
		stepZ = 0;
		culc_count();
	}
}

triangle culcradar::get_Triangle(size_t iTriangle)
{
//	if (iTriangle < triangles.size())
		return *(triangles[iTriangle]);
}

edge culcradar::get_Edge(size_t iEdge)
{
//	if (iEdge < edges.size())
		return *(edges[iEdge]);
}

node culcradar::get_Node(size_t iNode)
{
	return *(nodes[iNode]);
}

//загрузка геометрической модели
void culcradar::build_Model(string filename)
{
//загружаем вершины	
	string file;
//вершины
	double n1, n2, n3;
	file = filename +"_verte.txt";
	nodes.clear();

	ifstream model(file);
	{
		if (!model) 
			exit(EXIT_FAILURE);
//		getline(model,line);
		while (!model.eof())				//while ()		
		{
			model >> n1 >> n2 >> n3;
			node* n = new node();
			(*n).setPoint(n1, n2, n3);
			nodes.push_back(n);
		}
		model.close();
	} //node

//ребра
	file = filename + "_edge.txt";
	size_t i1, i2, i3;
	edges.clear();
	ifstream model1(file);
	{
		while (!model1.eof())				//while (getline(model, &line))		
		{
			model1 >> i1 >> i2;
			edge* eg = new edge();
			(*eg).setV1(nodes[i1]);
			(*eg).setV2(nodes[i2]);
			edges.push_back(eg);

		}
		model1.close();
	} //edge

//треугольники
	file = filename + "_trian.txt";
	triangles.clear();
	ifstream model2(file);
	{
		while (!model2.eof())				//while (getline(model, &line))		
		{
			model2 >> i1 >> i2 >> i3;
			triangle* tr = new triangle();
			(*tr).setV1(nodes[i1]);
			(*tr).setV2(nodes[i2]);
			(*tr).setV3(nodes[i3]);
			triangles.push_back(tr);

		}
		model2.close();
	}

}

//void culcradar::build_Model(string filename)
//{
////загружаем вершины	
//	string file;
////вершины
//	node n;
//	double n1, n2, n3;
//	file = filename +"_verte.txt";
//	nodes.clear();
//
//	ifstream model(file);
//	{
//		if (!model) 
//			exit(EXIT_FAILURE);
////		getline(model,line);
//		while (!model.eof())				//while ()		
//		{
//			model >> n1 >> n2 >> n3;
//			n.setPoint(n1, n2, n3);
//			nodes.push_back(n);
//		}
//		model.close();
//	}
//
////ребра
//	file = filename + "_edge.txt";
//	edge eg;
//	size_t i1, i2, i3;
//	edges.clear();
//	ifstream model1(file);
//	{
//		while (!model1.eof())				//while (getline(model, &line))		
//		{
//			model1 >> i1 >> i2;
//			eg.setV1(&nodes[i1]);
//			eg.setV2(&nodes[i2]);
//			edges.push_back(eg);
//
//		}
//		model1.close();
//	}
////треугольники
//	file = filename + "_trian.txt";
//	triangle tr;
//	triangles.clear();
//	ifstream model2(file);
//	{
//		while (!model2.eof())				//while (getline(model, &line))		
//		{
//			model2 >> i1 >> i2 >> i3;
//			tr.setV1(&nodes[i1]);
//			tr.setV2(&nodes[i2]);
//			tr.setV3(&nodes[i3]);
//			triangles.push_back(tr);
//
//		}
//		model2.close();
//	}
//}

cVect  culcradar::getEout(size_t iX, size_t iY, size_t iZ) 
{ 
//	if ((iZ<vEout.size()) && (iY<vEout[0].size()) && (iX<vEout[0][0].size())) 
	return vEout[iZ][iY][iX]; 
}

void culcradar::setEout(size_t iX, size_t iY, size_t iZ, cVect Eout) 
{
//	if ((iZ < vEout.size()) && (iY < vEout[0].size()) && (iX < vEout[0][0].size()))
		vEout[iZ][iY][iX] = Eout;
}

void culcradar::setSizeEout(size_t iX, size_t iY, size_t iZ)
{
	vEout.resize(iZ);	
		for (int iz = 0; iz < iZ; iz++)
		{
			vEout[iz].resize(iY);
			for (int iy = 0; iy < iY; iy++)
				vEout[iz][iy].resize(iX);
		}
		
}

//запуск задачи вычислени€ пол€ по ‘ќ
int culcradar::culc_Eout(bool Aref, 
	bool AboolX, bool AboolY, bool AboolZ, double aLmax,
	double AstepX, double AstepY, double AstepZ, double Awave,
	rVect aEin)
{
	int res = -1;
//инициализаци€ класса
	ref = Aref;
	set_boolXYZ(AboolX, AboolY, AboolZ);
	Lmax = aLmax;
	set_stepXYZ(AstepX, AstepY, AstepZ);
	wave = Awave;
	Ein = aEin;
	
//ћатрица перехода к системе локатора
	double cosangle = acos(Nin * rVectY);
	rVect aAxis = Nin ^ rVectY;
	rMatrix SO2, invSO2; 
	if (aAxis.norm() > 1e-6)
	{
		aAxis = 1. / aAxis.length()*aAxis;
		SO2.CreateRotationMatrix(aAxis, cosangle);
	}	
	invSO2 = transpon(SO2);
	rVect locNout = invSO2 * Nout;
	
	double dAngleX(0.);
	double dAngleZ(0.);
	if (stepX)
		dAngleX = 6. / (wave * stepX * countX);
	if (stepZ)
		dAngleZ = 6. / (wave * stepZ * countZ);

	triangle Triangle;

//цикл по углам и по частотам
	if (ref)
	{
		for (size_t iz = 0; iz < vEout.size(); iz++)
		{
			for (size_t iy = 0; iy < vEout[0].size(); iy++)
			{
				for (size_t ix = 0; ix < vEout[0][0].size(); ix++)
				{
					Nout.fromSphera(1.,
						0.5 * Pi + (1. * ix - 0.5 * (countX - 1)) * dAngleX,
						0.5 * Pi + (1. * iz - 0.5 * (countZ - 1)) * dAngleZ);
					Nout = -1.*(SO2 * Nout);
					NoutRef = Nout;  NoutRef.setZ(-Nout.getZ());
					for (int iTr = 0; iTr < triangles.size(); iTr++)
					{
						Triangle = *(triangles[iTr]);
						if (Triangle.getVisible())
						{ 
							vEout[iz][iy][ix] = vEout[iz][iy][ix] + Triangle.PolarDifraction(Nin,
								Nout, Ein, wave - (1. * iy - 0.5 * (countY - 1)) * stepW);
							vEout[iz][iy][ix] = vEout[iz][iy][ix] + Triangle.PolarDifraction(Nin,
								NoutRef, Ein, wave - (1. * iy - 0.5 * (countY - 1)) * stepW);
							vEout[iz][iy][ix] = vEout[iz][iy][ix] + Triangle.PolarDifraction(NinRef,
							Nout, Ein, wave - (1. * iy - 0.5 * (countY - 1)) * stepW);
							vEout[iz][iy][ix] = vEout[iz][iy][ix] + Triangle.PolarDifraction(NinRef,
								NoutRef, Ein, wave - (1. * iy - 0.5 * (countY - 1)) * stepW);
						}//if (triangles[iTr].getVisible())
					} //for iTr
				}//for ix
			}//for iy
		}//for iz
	}//if (ref)
	else
	{
		for (size_t iz = 0; iz < countZ; iz++)
		{
			for (size_t iy = 0; iy < countY; iy++)
			{
				for (size_t ix = 0; ix < countX; ix++)
				{
					/*built_Ns_out(0.5 * Pi + (1. * ix - 0.5 * (countX - 1)) * dAngleX,
						+0.5 * Pi + (1. * iz - 0.5 * (countZ - 1)) * dAngleZ);
					Nout = SO2 * Nout;
					NoutRef = SO2 * NoutRef;*/
					Nout.fromSphera(1.,
						0.5 * Pi + (1. * ix - 0.5 * (countX - 1)) * dAngleX,
						0.5 * Pi + (1. * iz - 0.5 * (countZ - 1)) * dAngleZ);
					Nout = -1.*(SO2 * Nout);
					for (int iTr = 0; iTr < triangles.size(); iTr++)
					{
						Triangle = *(triangles[iTr]);
						if (Triangle.getVisible())
							vEout[iz][iy][ix] = vEout[iz][iy][ix] + Triangle.PolarDifraction(Nin,
								Nout, Ein, wave - (1. * iy - 0.5 * (countY - 1)) * stepW);
					} //for iTr
				}//for ix
			}//for iy
		}//for iz
	}
	vEout = fft3(vEout, 1);
	vEout = reorder3(vEout);
	for (size_t iz = 0; iz < countZ; iz++)
		for (size_t iy = 0; iy < countY; iy++)
			for (size_t ix = 0; ix < countX; ix++)
				vEout[iz][iy][ix] = sqrt(4. * Pi / countY) * vEout[iz][iy][ix];
	res = 0;
	return res;
}
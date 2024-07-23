#pragma once

#include <complex>
#include <vector>
#include "cVect.h"
#include <iostream>
#include "ConstAndVar.h"
//#include "Math.h"

using namespace std;



template <class Value>
int Sign(Value Val) {
  if (Val == 0.)  return 0;
  if (Val > 0.)  return 1;
  else return -1;}

/*void sgnCheck(int &s)
{
	if (s == 0)
		cout<<"ERROR! sgn = 0"<<endl;
	if (s*s != 1)
	{
		cout<<"Warning! sgn = " << s <<endl;
		s = Sign(s);
		cout << "sgn put " << s << endl;
	}
}*/

//прецедура возвращает два в степени n ближайшее к размерности вычисленной из требований задачи
unsigned int pad2(unsigned int n)
{
	unsigned int res=1;
	while (res < n) 
		res = res << 1;
	return res;
}


//одномерное FFT от скал€ра
vector<complex<double>> fft(vector<complex<double>> cx, int sgn)
{
	Sign(sgn);
	unsigned int nx=cx.size();
	double scale, arg;
	complex<double> cw, cdel, ct;
    unsigned int i, j, k, m, istep;
	
	scale = 1. / sqrt((double)nx);
    for (i = 0; i<nx; i++)
		cx[i] = cx[i]* scale;

    j = 0;
    for (i = 0; i<nx; i++)
    {
		if (i <= j)
		{
			ct = cx[j];
			cx[j] = cx[i];
			cx[i] = ct;
		}
		m = nx >> 1;
		while ((j >= m) && (m > 1))
		{
			j = j - m;
			m = m >> 1;
		}
		j = j + m;
	} //for (i = 0; i<nx; i++)

	k = 1;
	do {
		istep = 2 * k;
		cw = 1.;
		arg = sgn * Pi / k;
		cdel = exp(OneI*arg);
//		cdel.real = cos(arg);
//		cdel.imag = sin(arg);

		for (m = 0; m < k; m++)
		{
			i = m;
			while (i < nx)
			{ // for i=m to nx step istep
				ct = cw * cx[i + k];
				cx[i + k] = cx[i] - ct;
				cx[i] = cx[i] + ct;
				i += istep;
			} //while i < nx
			cw = cw * cdel;
		} //m := 0 to k - 1

		k = istep;
	}while (k < nx);	
	return cx;
}


//мен€ет пор€док результата
vector<complex<double>> reorder(vector<complex<double>> inp)// 1D
{
//  vector<complex<double>> tmp;
  unsigned int i, h, w;
  w = inp.size();
  h = w >> 1;
  vector<complex<double>>tmp(h);
  for (i = 0; i<h; i++)
    tmp[i] = inp[i + h];
  for (i = 0; i < h; i++)
    inp[i + h] = inp[i];
  for (i = 0; i<h; i++)
    inp[i] = tmp[i];
  return inp;
}

//двумерное ‘урье
vector<vector<complex<double>>> fft2(vector<vector<complex<double>>> cx,int sgn) 
{
	Sign(sgn);
	unsigned int nx=cx[0].size();
	unsigned int ny=cx.size();
	unsigned int i, j;
	vector<complex<double>> tmp(ny);
	
	for (i = 0; i < ny; i++) // fft rows: up-down
	{
		cx[i] = fft(cx[i], sgn);
	}

	for (i = 0; i < nx; i++)
	{// for each col
      for (j = 0; j < ny; j++)// copy the i-th col of inp[rows,cols] to tmp[rows]
        tmp[j] = cx[j][i];
      tmp = fft(tmp, sgn); // fft on the i-th col
      for (j = 0; j<ny; j++) // copy result back from tmp to inp
        cx[j][i] = tmp[j];
	} //for i

	return cx;
}

//двумерное изменение пор€дка результата
vector<vector<complex<double>>> reorder2(vector<vector<complex<double>>> inp)
{
	unsigned int ix,iy;
	for (iy=0; iy<inp.size(); iy++)
		inp[iy]=reorder(inp[iy]);

	vector<complex<double>> tmp(inp.size());
	for (ix=0; ix<inp[0].size(); ix++)
	{
		for (iy= 0;iy<inp.size(); iy++)
			tmp[iy]=inp[iy][ix];
		tmp=reorder(tmp);
		for (iy= 0;iy<inp.size(); iy++)
			inp[iy][ix]=tmp[iy];
	}	
	return inp;	
}

//трехмерное ‘урье
vector<vector<vector<complex<double>>>> fft3(vector<vector<vector<complex<double>>>> cx,
											 int sgn)
{
	Sign(sgn);
	unsigned int nz=cx.size();
	unsigned int ny=cx[0].size();
	unsigned int nx=cx[0][0].size();
	unsigned ix, iy, iz;
	vector<complex<double>> tmp(nz);
	for (iz = 0; iz < nz; iz++) // fft rows: up-down
	{
		cx[iz] = fft2(cx[iz], sgn);
	}

	for (ix = 0; ix < nx; ix++)
	for (iy = 0; iy < ny; iy++)
	{
		for (iz = 0; iz < nz; iz++)
			tmp[iz] = cx[iz][iy][ix];
		tmp = fft(tmp, sgn);
		for (iz = 0; iz < nz; iz++)
			cx[iz][iy][ix] = tmp[iz];
	}
	
	return cx;
}

//трехмерное изменение пор€дка результата
vector<vector<vector<complex<double>>>> reorder3(vector<vector<vector<complex<double>>>> inp)
{
	unsigned int ix, iy, iz;
	vector<complex<double>> tmp(inp.size());
	for (iz = 0; iz < inp.size(); iz++)
		inp[iz]=reorder2(inp[iz]);

	for (ix = 0; ix < inp[0][0].size(); ix++)
	for (iy = 0; iy < inp[0].size(); iy++)
	{
		for (iz=0; iz<inp.size(); iz++)
			tmp[iz]=inp[iz][iy][ix];
		tmp=reorder(tmp);
		for (iz=0; iz<inp.size(); iz++)
			inp[iz][iy][ix]=tmp[iz];
	}

	return inp;
}
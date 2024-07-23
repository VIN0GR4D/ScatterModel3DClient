#pragma once

#include <complex>
#include <vector>
#include "cVect.h"
//#include <iostream>
//#include "CPUFFT.h"

using namespace std;



//одномерное FFT от комплексного вектора
vector<cVect> fft(vector<cVect> cx, int sgn)
{
	Sign(sgn);
	unsigned int nx = cx.size();
	if (nx <= 1)
		return cx;
	double scale, arg;
	complex<double> cdel, cw;
	cVect ct;
    unsigned int i, j, k, m, istep;
	scale = 1. / sqrt((double)nx);
    for (i = 0; i<nx; i++)
		cx[i] = scale*cx[i] ;

    j = 0;
    for (i = 0; i<nx; i++)
    {
		if (i <= j)
		{
			ct = cx[j];
			cx[j] = cx[i];
			cx[i] = ct;
		}//if
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
			cw = cdel * cw;
		} //m := 0 to k - 1

		k = istep;
	}while (k < nx);	
	return cx;
}



//двумерное Фурье вектора
vector<vector<cVect>> fft2(vector<vector<cVect>> cx,int sgn)
{
	Sign(sgn);
	unsigned int i, j;
	unsigned int nx=cx[0].size();
	unsigned int ny=cx.size();
	vector<cVect> tmp(ny);
	
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

//трехмерное Фурье
vector<vector<vector<cVect>>> fft3(vector<vector<vector<cVect>>> cx,
											 int sgn)
{
	Sign(sgn);
	unsigned int nz=cx.size();
	unsigned int ny=cx[0].size();
	unsigned int nx=cx[0][0].size();
	unsigned ix, iy, iz;
	vector<cVect> tmp(nz);
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

//меняет порядок результата
vector<cVect> reorder(vector<cVect> inp)// 1D
{
  unsigned int i, h, w;
  w = inp.size();
  h = w >> 1;
  vector<cVect> tmp(h);
  for (i = 0; i<h; i++)
    tmp[i] = inp[i + h];
  for (i = 0; i < h; i++)
    inp[i + h] = inp[i];
  for (i = 0; i<h; i++)
    inp[i] = tmp[i];
  return inp;
}

//двумерное изменение порядка результата
vector<vector<cVect>> reorder2(vector<vector<cVect>> inp)
{
	unsigned int ix,iy;
	for (iy=0; iy<inp.size(); iy++)
		inp[iy]=reorder(inp[iy]);

	vector<cVect> tmp(inp.size());
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

//трехмерное изменение порядка результата
vector<vector<vector<cVect>>> reorder3(vector<vector<vector<cVect>>> inp)
{
	unsigned int ix, iy, iz;
	vector<cVect> tmp(inp.size());
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
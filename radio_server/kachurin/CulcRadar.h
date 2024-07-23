#pragma once

#include "node.h"
#include "edge.h"
#include "triangle.h"
#include "rVect.h"
#include "cVect.h"
#include <vector>

/*
������� ��������� ������� ����� �������, ��� ��������� XOY ����������� ������������� ����������� ������ �����������.
��� Z ���������� ����� � ��� XYZ �������� ������ ������. 
������������ ����������� (���� ��� ����) ����� � ��������� Z = 0.
��� ���� ���������� � ��������.
*/

class culcradar
{
private:
	bool ref=false; //������� ������������ �����������
//	double phi=0., theta=0.;// ������. ���� ����������� �� ������. theta ���� �����
	rVect Nin, Nout, NinRef, NoutRef;
	bool boolX, boolY, boolZ; //�� ����� ���� �������� ���
//��� ��������� ���������� ��� ������������� ������� ��������� � ����������� �������
	double Lmax; //������������ ������ �������
	double stepX, stepY, stepZ; //���������� �� ���� ���������
	int countX, countY, countZ; //����������� ������� ���
	double wave; //�������� �����
	double stepW; //��� �� �������� ������
//�������������� ������
	vector<edge*> edges;
	vector<triangle*> triangles;
	vector<node*> nodes;

//�������� ����
	rVect Ein;

//���������� ����
	vector<vector<vector<cVect>>> vEout;

public:
	culcradar(); //�����������
	void set_ref(bool Ref) { ref=Ref; } //������������ �����������
//	void set_phi(double Phi) { phi = Phi; }
//	void set_theta(double Theta) { theta = Theta;}
	void set_boolXYZ(bool X, bool Y, bool Z);
	void set_boolX(bool X);
	void set_boolY(bool Y);
	void set_boolZ(bool Z);
//��� ��������� ���������� ��� ������������� ������� ��������� � ����������� �������
	void set_Lmax(double L); // { Lmax = L; }
	void set_stepXYZ(double x, double y, double z); // { stepX = x; stepY = y; stepZ = z; }
	void set_stepX(double x) {
		if (!boolX)
			stepX = 0;
		else
			stepX = x;
	}
	void set_stepY(double y) {
		if (!boolY)
			stepY = 0;
		else
			stepY = y;
	}
	void set_stepZ(double z) {
		if (!boolZ)
			stepZ = 0;
		else
			stepZ = z;
	}
	void set_wave(double W) { wave = W;}

	bool get_boolX() { return boolX; }
	bool get_boolY() { return boolY; }
	bool get_boolZ() { return boolZ; }
	double get_Lmax() { return Lmax; }
	double get_stepX() { return stepX; }
	double get_stepY() { return stepY; }
	double get_stepZ() { return stepZ; }
	double get_wave() { return wave; }

private:
//����� ����������� ������� � ���� �� �������� ������ ��� ��������� �����
	void culc_count();// countX, countY, countZ; stepW;

public:
//�������� �������������� ������ ���� ���������� �� ����� obj ����� �� JSON
	void build_Model(string filename);
	void addNode(node N) { nodes.push_back(&N); }
	triangle get_Triangle(size_t iTriangle);
	void addTriangle(triangle T) { triangles.push_back(&T); }
	void addEdge(edge E) { edges.push_back(&E); }
	edge get_Edge(size_t iEdge);
	node get_Node(size_t iNode);

	size_t getNodeSize() { return nodes.size(); }
	size_t getEdgeSize() { return edges.size(); }
	size_t getTriangleSize() { return triangles.size(); }


	bool get_ref() { return ref; }
	void built_Ns_in(double phi, double theta);
	void built_Ns_out(double phi, double theta);
	rVect get_Nin() { return Nin; }
	rVect get_Nout() { return Nout; }
	rVect get_NinRef() { return NinRef; }
	rVect get_NoutRef() { return NoutRef; }

//�������� ����
	rVect getEin() { return Ein; }
	void setEin(rVect val) { Ein = val; }

//���������� ����
	cVect getEout(size_t iX, size_t iY, size_t iZ);
	void setEout(size_t iX, size_t iY, size_t iZ, cVect Eout);
private:
	void setSizeEout(size_t iX, size_t iY, size_t iZ);
public:
	int getSizeEoutX() { return vEout[0][0].size(); }
	int getSizeEoutY() { return vEout[0].size(); }
	int getSizeEoutZ() { return vEout.size(); }

//������ ������ ���������� ���� �� ��
//���� �������� � ��������. �� ������� �������� ���� �����, polar ��� �������� ���� 
	int culc_Eout(bool Aref,
		bool AboolX, bool AboolY, bool AboolZ, double aLmax,
		double AstepX, double AstepY, double AstepZ, double Awave,
		rVect aEin);

};

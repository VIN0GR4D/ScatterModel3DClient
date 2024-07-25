// #define byte unsigned char
#include <wtypes.h>
// #undef byte

#include "CulcRadar.h"
#include "CPUFFT.h"
#include "VectFFT.h"
#include "rVect.h"
#include "rMatrix.h"
#include <fstream>
#include <string>
#include <QTextCodec>
#include <QFile>
#include <QJsonDocument>
#include <QDataStream>

culcradar::culcradar(QObject *parent) : QObject(parent)
{
    qDebug() <<"Culc_radar constructor";
    progress = 0;
    RUN_C = true;
    ref = false;
	Nin = rVectY; 
	Nout = -1 * Nin;
	NinRef = rVectY;
	NoutRef=Nout;
	boolX = false;
	boolY = false;
	boolZ = false;
	Lmax=0.; //������������ ������ �������
	stepX = 0.; stepY = 0.; stepZ = 0.; //���������� �� ���� ���������
	countX = 0; countY = 0; countZ = 0; //����������� ������� ���
	wave=0.; //�������� �����
	stepW=0; //��� �� �������� ������
    //edges.clear();
	triangles.clear();
	nodes.clear();
	Ein.setPoint(1., 0., 0.);
	vEout.clear();
    SAVE_MODEL_TO_FILE = false;
    SCAT_FIELD_TO_FILE = false;
    FFT_FIELD_TO_FILE = false;
    RESULT_FROM_FILE = false;   
}

//���������� ����������� �������
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
		return triangles[iTriangle];
}

edge culcradar::get_Edge(size_t iEdge)
{
//	if (iEdge < edges.size())
        return *edges[iEdge];
}

node culcradar::get_Node(size_t iNode)
{
//	if (iNode < nodes.size())
        return *nodes[iNode];
}

//�������� �������������� ������
int culcradar::build_Model(QJsonObject &jsonObject, QHash<uint, node> &Node, QHash<uint,edge> &Edge)
{
//��������� ������� �� jsonObject
 // QJsonObject coord;
 // std::vector<double> n_coord;
 // int n = 0;
 // if (jsonObject.contains("data")) {
 //     coord = jsonObject.value("data").toObject();
 //     qDebug() << "Received data:" << coord;
 //     qDebug() << coord;
 //     n = coord.count();
 //     assert(n > 0);
 //     for (int i = 0; i < n; i++) {
 //         QString ii = QString::number(i);
 //         if (coord.contains(ii)) {
 //             double value = coord.value(ii).toDouble();
 //             n_coord.push_back(value);
 //             qDebug() << "Coordinate" << i << ":" << value;
 //         }
 //     }
 // } else {
 //     qDebug() << "Error: 'data' not found in JSON";
 //     return 1; // ������
 // }
    QJsonArray coordArray;
    std::vector<double> n_coord;

    if (jsonObject.contains("data")) {
        coordArray = jsonObject.value("data").toArray();
        qDebug() << "Received data:" << coordArray;
        for (auto value : coordArray) {
            n_coord.push_back(value.toDouble());
        }
    } else {
        qDebug() << "Error: 'data' not found in JSON";
        return 1; // ������
    }

 // �������� ���������� ���������
 if (n_coord.size() % 9 != 0) {
     qDebug() << "Error: Number of coordinates is not a multiple of 9";
     return 1; // ������
 }


//�� ������ ��������� ������������� ������������
 std::vector<std::vector<std::vector<double>>> tri;
 if (n_coord.size() % 9 == 0) tri.resize(n_coord.size()/9); //�������� �� ������� �� �������
 assert(n_coord.size() % 9 == 0);
 int jj = 0;
 for (int i = 0; i < (int)tri.size(); i++) {
     tri[i].resize(3);
     for (int j = 0; j < 3; j++) {
         tri[i][j].resize(3);
         for (int k = 0; k < 3; k++) {
             tri[i][j][k] = n_coord[jj];
             qDebug() << "Triangle" << i << "Vertex" << j << "Coordinate" << k << ":" << n_coord[jj];
             jj++;
         }
     }
 }

 //��������� ���������� ���� � ����� ����� ������� � �������� ������ Node, Edge
 //����� ����� �������� �� ���� �������������� �� 180 ��������
 for (int i = 0; i < (int)tri.size(); i++) {
     for (int j = 0; j < 3; j++) {

         int k = j + 1;
         if (k == 3) k = 0;

         double x1 = tri[i][j][0];
         double y1 = tri[i][j][1];
         double z1 = tri[i][j][2];
         double x2 = tri[i][k][0];
         double y2 = tri[i][k][1];
         double z2 = tri[i][k][2];

         QJsonObject obj1, obj2;
         obj1.insert("x1",x1); obj1.insert("y1",y1); obj1.insert("z1",z1);
         uint hash1 = qHash(obj1);
         obj2.insert("x1",x2); obj2.insert("y1",y2); obj2.insert("z1",z2);
         uint hash2 = qHash(obj2);

         node tempNode1(x1,y1,z1,true);
         Node.insert(hash1,tempNode1);
         node tempNode2(x2,y2,z2,true);
         Node.insert(hash2,tempNode2);

         edge tempEdge1, tempEdge2;
         tempEdge1.setV1(&Node[hash1]);
         tempEdge1.setV2(&Node[hash2]);
         tempEdge2.setV1(&Node[hash2]);
         tempEdge2.setV2(&Node[hash1]);

         obj1.insert("x2",x2); obj1.insert("y2",y2); obj1.insert("z2",z2);
         obj2.insert("x2",x1); obj2.insert("y2",y1); obj2.insert("z2",z1);
         hash1 = qHash(obj1);
         hash2 = qHash(obj2);

         Edge.insert(hash1,tempEdge1);
         Edge.insert(hash2,tempEdge2);
     }
 }

 //������� ��������� ����� Edge, �������� ����������
 QHash<uint, edge>::const_iterator it = Edge.constBegin();
 for (int i = 0; i < (int)Edge.size(); i++) {
     edge tempEdge = it.value();
     double x1 = tempEdge.getV1()->getX();
     double y1 = tempEdge.getV1()->getY();
     double z1 = tempEdge.getV1()->getZ();
     double x2 = tempEdge.getV2()->getX();
     double y2 = tempEdge.getV2()->getY();
     double z2 = tempEdge.getV2()->getZ();
     QJsonObject obj;
     obj.insert("x1",x2); obj.insert("y1",y2); obj.insert("z1",z2);
     obj.insert("x2",x1); obj.insert("y2",y1); obj.insert("z2",z1);
     uint hash = qHash(obj);
     Edge.remove(hash);
     ++it;
 }

 //��������� ������� nodes, edges ����������� ������ � �������
 QHash<uint, node>::const_iterator in = Node.constBegin();
 int size = (int)Node.size();
 for (int i = 0; i < size; i++) {
    uint hash = in.key();
    nodes.push_back(&Node[hash]);
    ++in;
 }

 it = Edge.constBegin();
 size = (int)Edge.size();
 for (int i = 0; i < size; i++) {
     uint hash = it.key();
     edges.push_back(&Edge[hash]);
     ++it;
 }

 //��������� ������ �� ������������ �������������
 QJsonArray qvisible;
 std::vector<bool> n_visible;
 int n_vis = 0;

 if (jsonObject.contains("visibleTriangles")) {
     qvisible = jsonObject.value("visibleTriangles").toArray();
     n_vis = qvisible.count();
     for (int i = 0; i < n_vis; i++) {
         bool visible = qvisible.at(i).toBool();
         n_visible.push_back(visible);
         qDebug() << "Triangle" << i << "visible:" << visible;
     }
 } else {
     qDebug() << "Error: 'visibleTriangles' not found in JSON";
     return 2; // ������
 }

 //��������� ������ ������������� triangles
 assert(tri.size() == n_visible.size()); //�������� �� ���������� �������� ��������
 size = (int)tri.size();
 triangles.resize(size);
 for (int i = 0; i < size; i++) {
    triangles.at(i).setVisible(n_visible.at(i));
    for (int j = 0; j < 3; j++) {
        double x = tri[i][j][0];
        double y = tri[i][j][1];
        double z = tri[i][j][2];
        QJsonObject obj;
        obj.insert("x1",x); obj.insert("y1",y); obj.insert("z1",z);
        uint hash = qHash(obj);
        if(Node.contains(hash)) {
            node *tempNode = &Node[hash];
            tempNode->setVisible(triangles.at(i).getVisible());
            if (j == 0) {
                triangles.at(i).setV1(tempNode);//
            }
            else if (j == 1) {
                triangles.at(i).setV2(tempNode);//
            }
            else if (j == 2) {
                triangles.at(i).setV3(tempNode);//
            }
        }
    }
 }

 //��������� ������� ������� ������������� (��� ������ �����)
 for (int i = 0; i < size; i++) {
    for (int j = 0; j < 3; j++) {

        int k = j + 1;
        if (k == 3) k = 0;

        double x1 = tri[i][j][0];
        double y1 = tri[i][j][1];
        double z1 = tri[i][j][2];
        double x2 = tri[i][k][0];
        double y2 = tri[i][k][1];
        double z2 = tri[i][k][2];
        QJsonObject obj1, obj2;
        obj1.insert("x1",x1); obj1.insert("y1",y1); obj1.insert("z1",z1);
        obj2.insert("x1",x2); obj2.insert("y1",y2); obj2.insert("z1",z2);
        obj1.insert("x2",x2); obj1.insert("y2",y2); obj1.insert("z2",z2);
        obj2.insert("x2",x1); obj2.insert("y2",y1); obj2.insert("z2",z1);
        uint hash1,hash2;
        hash1 = qHash(obj1);
        hash2 = qHash(obj2);
        edge * t_e;
        if (Edge.contains(hash1)) {
           t_e = &Edge[hash1];
           t_e->push_triangle(&triangles[i]);
           t_e->setVisible(triangles[i].getVisible());
        }
        if (Edge.contains(hash2)) {
           t_e = &Edge[hash2];
           t_e->push_triangle(&triangles[i]);
           t_e->setVisible(triangles[i].getVisible());
        }
    }
  }

 //��������� ���������� ����� ������, ������ �� ������ ��������� ������
 int freqband = -1;
 if (jsonObject.contains("freqBand")) {
     freqband = jsonObject.value("freqBand").toInt();
     qDebug() << "freqBand:" << freqband;
 }
 radar_wave wave1(freqband);
 //��������� ����������� �����
 int inc_polariz, ref_polariz;
 if (jsonObject.contains("polarRadiation")) {
     if (jsonObject.contains("polarRecive")) {
         inc_polariz = jsonObject.value("polarRadiation").toInt();
         ref_polariz = jsonObject.value("polarRecive").toInt();
         qDebug() << "polarRadiation:" << inc_polariz;
         qDebug() << "polarRecive:" << ref_polariz;

         int err = wave1.setPolariz(inc_polariz, ref_polariz, Nin, Ein);
         if (err > 0) {
             qDebug() << "Error in setPolariz, code:" << err;
             return 4;
         }
     } else {
         qDebug() << "Error: 'polarRecive' not found in JSON";
         return 4;
     }
 } else {
     qDebug() << "Error: 'polarRadiation' not found in JSON";
     return 4;
 }


 set_wave(2*Pi/wave1.getLambda());

 //6. ��������� ���� �������������
 bool azimuth_radar_image, elevation_radar_image, range_radar_image;
 if (jsonObject.contains("typeAngle")) {
     elevation_radar_image = jsonObject.value("typeAngle").toBool();
     qDebug() << "typeAngle:" << elevation_radar_image;
 }
 if (jsonObject.contains("typeAzimut")) {
     azimuth_radar_image = jsonObject.value("typeAzimut").toBool();
     qDebug() << "typeAzimut:" << azimuth_radar_image;
 }
 if (jsonObject.contains("typeLength")) {
     range_radar_image = jsonObject.value("typeLength").toBool();
     qDebug() << "typeLength:" << range_radar_image;
 }
 if (!(elevation_radar_image + azimuth_radar_image + range_radar_image)) {
     qDebug() << "Error: No radar portrait type selected";
     return 5;
 } else {
     set_boolXYZ(azimuth_radar_image, range_radar_image, elevation_radar_image);
     set_stepXYZ(wave1.getStepX(), wave1.getStepY(), wave1.getStepZ());
 }

 //��������� ������������ ������ ������ Lmax
 double Lmax = 0;
 rVect rmin = (rVect)get_Node(0);
 rVect rmax = (rVect)get_Node(0);
 node Node_;
 size_t nCount = getNodeSize();
 for (int iPoint = 1; iPoint < (int)nCount; iPoint++){
     Node_ = get_Node(iPoint);
     if (rmin.getX() > Node_.getX())
         rmin.setX(Node_.getX());
     if (rmin.getY() > Node_.getY())
         rmin.setY(Node_.getY());
     if (rmin.getX() > Node_.getX())
         rmin.setX(Node_.getX());
     if (rmin.getZ() > Node_.getZ())
         rmin.setZ(Node_.getZ());
     if (rmax.getX() < Node_.getX())
         rmax.setX(Node_.getX());
     if (rmax.getY() < Node_.getY())
         rmax.setY(Node_.getY());
     if (rmax.getX() < Node_.getX())
         rmax.setX(Node_.getX());
     if (rmax.getZ() < Node_.getZ())
         rmax.setZ(Node_.getZ());
 }//for iPoint
 Lmax = (rmax - rmin).length();
 set_Lmax(Lmax); //��������� Lmax

 //��������� ������� ������������ �����������
 if (jsonObject.contains("pplane")) {
     bool pplane = jsonObject.value("pplane").toBool();
     set_ref(pplane);
     qDebug() << "pplane:" << pplane;
  }

 //��������� ����������� ������� �����
  QJsonObject qdirectv;
  rVect aEin;
  if (jsonObject.contains("directVector")) {
      QJsonObject qdirectv = jsonObject.value("directVector").toObject();
      double x = qdirectv.value("x").toDouble();
      double y = qdirectv.value("y").toDouble();
      double z = qdirectv.value("z").toDouble();
      rVect In(x, y, z);
      Nin = In;
      rVect Ref(Nin.getX(), Nin.getY(), -Nin.getZ());
      NinRef = Ref;
      rVect Out(-Nin.getX(), -Nin.getY(), -Nin.getZ());
      rVect OutRef(-Nin.getX(), -Nin.getY(), Nin.getZ());
      Nout = Out;
      NoutRef = OutRef;

      qDebug() << "directVector: (" << x << "," << y << "," << z << ")";
  } else {
      qDebug() << "Error: 'directVector' not found in JSON";
      return 3; // ������, ���� directVector �� �����
  }





  RWave = wave1;
//������ ������ � json-����
  if (SAVE_MODEL_TO_FILE) {
      QFile file("model.json");
      QJsonObject model_obj;
      QJsonObject tr_obj;
      QJsonObject edge_obj;
      QJsonObject node_obj;
      QJsonObject dir_obj;
      QJsonObject pol_obj;
      size_t tr_size = triangles.size();
      for (size_t i = 0; i < tr_size; i++) {
          std::stringstream tr_str;
          tr_str << get_Triangle(i);
          QString tr_qstr;
          tr_qstr = QString::fromStdString(tr_str.str());
          tr_obj.insert(QString::number(i),tr_qstr);
      }
      size_t n_size = nodes.size();
      for (size_t i = 0; i < n_size; i++) {
          std::stringstream n_str;
          n_str << get_Node(i);
          QString n_qstr;
          n_qstr = QString::fromStdString(n_str.str());
          node_obj.insert(QString::number(i),n_qstr);
      }
      size_t edge_size = edges.size();
      for (size_t i = 0; i < edge_size; i++) {
          std::stringstream edge_str;
          edge_str << get_Edge(i);
          QString edge_qstr;
          edge_qstr = QString::fromStdString(edge_str.str());
          edge_obj.insert(QString::number(i),edge_qstr);
      }
      std::stringstream dir;
      dir << Nin;
      dir_obj.insert("direct vector",QString::fromStdString(dir.str()));
      QString pol;
      pol = QString::number(RWave.getIncPolariz());
      pol_obj.insert("wave incident polarization",pol);
      pol = QString::number(RWave.getRefPolariz());
      pol_obj.insert("wave receive polarization",pol);

      model_obj.insert("wave direct",dir_obj);
      model_obj.insert("wave polarization",pol_obj);
      model_obj.insert("triangles",tr_obj);
      model_obj.insert("nodes",node_obj);
      model_obj.insert("edges",edge_obj);

      QJsonDocument Model(model_obj);

      if(!file.open(QIODevice::WriteOnly)) {
          clogs("file model.json error to open","","");
      }
      else {
          clogs("save file model.json","","");
          file.resize(0);
          file.write(Model.toJson());
      }
      file.close();
      SAVE_MODEL_TO_FILE = false;
  }

  return 0;
}

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
        for (int iz = 0; (size_t)iz < iZ; iz++)
		{
			vEout[iz].resize(iY);
            for (int iy = 0; (size_t)iy < iY; iy++)
				vEout[iz][iy].resize(iX);
		}
		
}

//������ ������ ���������� ���� �� ��
int culcradar::culc_Eout(/*bool Aref, double Aphi, double Atheta,
	bool AboolX, bool AboolY, bool AboolZ, double aLmax,
	double AstepX, double AstepY, double AstepZ, double Awave,
    rVect aEin*/)
{
	int res = -1;
//������������� ������
//	ref = Aref;
//	built_Ns_in(Aphi, Atheta);
//	built_Ns_out(Aphi, Atheta);
//	set_boolXYZ(AboolX, AboolY, AboolZ);
//	Lmax = aLmax;
//	set_stepXYZ(AstepX, AstepY, AstepZ);
//	wave = Awave;
//	Ein = aEin;
	
//������� �������� � ������� ��������
	double cosangle = acos(Nin * rVectY);
	rVect aAxis = Nin ^ rVectY;
	rMatrix SO2, invSO2; 
	if (aAxis.norm() > 1e-6)
	{
		aAxis = 1. / aAxis.length()*aAxis;
		SO2.CreateRotationMatrix(aAxis, cosangle);
	}	
	invSO2 = transpon(SO2);
    //rVect locNout = invSO2 * Nout;
	
	double dAngleX(0.);
	double dAngleZ(0.);
	if (stepX)
		dAngleX = 6. / (wave * stepX * countX);
	if (stepZ)
		dAngleZ = 6. / (wave * stepZ * countZ);

    //����� �������� ���������
    progress = 0;
    float p = 0;
    count = true; //��������-��� �������
    signal_send_progress_bar_culcradar();

    int m = 0;
    uint size1 = vEout[0][0].size();
    uint size2 = vEout[0].size();
    uint size3 = vEout.size();
    int num_angle = size1 * size2 * size3;

    //�������� � ������ ������� � �������������� 1000 ����
    bool send = false; //���� ���������� �������� �������� ��������-����


    m_timer.setFunc([&](){
        //qDebug() << progress;
        send = true; //��������� ��������
    })
            ->setInterval(1000) //��������� ���������
            ->start();          //������

    //������ ����������� ���� �� ��������� �����
    if (RESULT_FROM_FILE) {
        //������ � vEout...
       RESULT_FROM_FILE = false;
    }
    else {
        //���� �� ����� � �� ��������
        if (ref)
        {
            for (size_t iz = 0; iz < size3; iz++)
            {
                for (size_t iy = 0; iy < size2; iy++)
                {
                    for (size_t ix = 0; ix < size1; ix++)
                    {
                        Nout.fromSphera(1.,
                            0.5 * Pi + (1. * ix - 0.5 * (countX - 1)) * dAngleX,
                            0.5 * Pi + (1. * iz - 0.5 * (countZ - 1)) * dAngleZ);
                        Nout = -1.*(SO2 * Nout);
                        NoutRef = Nout;  NoutRef.setZ(-Nout.getZ());
                        for (int iTr = 0; iTr < (int)triangles.size(); iTr++)
                        {
                            if (triangles[iTr].getVisible())
                            {
                                vEout[iz][iy][ix] = vEout[iz][iy][ix] + triangles[iTr].PolarDifraction(Nin,
                                    Nout, Ein, wave - (1. * iy - 0.5 * (countY - 1)) * stepW);
                                vEout[iz][iy][ix] = vEout[iz][iy][ix] + triangles[iTr].PolarDifraction(Nin,
                                    NoutRef, Ein, wave - (1. * iy - 0.5 * (countY - 1)) * stepW);
                                vEout[iz][iy][ix] = vEout[iz][iy][ix] + triangles[iTr].PolarDifraction(NinRef,
                                Nout, Ein, wave - (1. * iy - 0.5 * (countY - 1)) * stepW);
                                vEout[iz][iy][ix] = vEout[iz][iy][ix] + triangles[iTr].PolarDifraction(NinRef,
                                    NoutRef, Ein, wave - (1. * iy - 0.5 * (countY - 1)) * stepW);
                            }
                        } //if (triangles[iTr].getVisible())
                        //Progress bar
                        auto it = m;
                        p = (float)(it) / (num_angle - 1);
                        p *= 100;
                        m++;
                        if ((m_timer.isRunning()) && (send)) { //���� ������ ������� � �������� ���������
                           progress = (int)p;
                           signal_send_progress_bar_culcradar();
                           send = false;  //��������� ������� ��������
                        }
                        if (!RUN_C) {
                            m_timer.stop();
                            return -1;
                        }
                    }//for ix
                }//for iy
            }//for iz
        }//if (ref)
        else
        {
            for (size_t iz = 0; iz < size3; iz++)
            {
                for (size_t iy = 0; iy < size2; iy++)
                {
                    for (size_t ix = 0; ix < size1; ix++)
                    {
                        /*built_Ns_out(0.5 * Pi + (1. * ix - 0.5 * (countX - 1)) * dAngleX,
                            +0.5 * Pi + (1. * iz - 0.5 * (countZ - 1)) * dAngleZ);
                        Nout = SO2 * Nout;
                        NoutRef = SO2 * NoutRef;*/
                        Nout.fromSphera(1.,
                            0.5 * Pi + (1. * ix - 0.5 * (countX - 1)) * dAngleX,
                            0.5 * Pi + (1. * iz - 0.5 * (countZ - 1)) * dAngleZ);
                        Nout = -1.*(SO2 * Nout);
    //					NoutRef = Nout;  NoutRef.setZ(-Nout.getZ());
                        for (int iTr = 0; iTr < (int)triangles.size(); iTr++)
                        {
                            if (!RUN_C) return -1;
                            if (triangles[iTr].getVisible())
                                vEout[iz][iy][ix] = vEout[iz][iy][ix] + triangles[iTr].PolarDifraction(Nin,
                                    Nout, Ein, wave - (1. * iy - 0.5 * (countY - 1)) * stepW);
                        } //if (triangles[iTr].getVisible())

                        //Progress bar
                        auto it = m;
                        p = (float)(it) / (num_angle - 1);
                        p *= 100;
                        m++;
                        if ((m_timer.isRunning()) && (send)) {//���� ������ ������� � �������� ���������
                           progress = (int)p;
                           signal_send_progress_bar_culcradar();
                           send = false; //��������� ������� ��������
                        }
                        if (!RUN_C) {
                            m_timer.stop();
                            return -1;
                        }
                    }//for ix
                }//for iy
            }//for iz
        }
    }

    m_timer.stop(); //��������� �������
    progress = 100;
    signal_send_progress_bar_culcradar();

    if (SCAT_FIELD_TO_FILE) {

//        QFile file2("scat_field.bin");
        QJsonArray scat_obj = {};
        for (size_t i = 0; i  < size3; i++){
            QJsonArray Y = {};
            for (size_t j = 0; j < size2; j++) {
                QJsonArray X = {};
                for (size_t k = 0; k < size1; k++) {
                    QString index, value;
                    stringstream v;
                    index = "[" +QString::number(k)+"]" +
                            "[" +QString::number(j)+"]" +
                            "[" +QString::number(i)+"]";
                    cVect Eout = vEout[i][j][k];
                    v << Eout;
                    value = QString::fromStdString(v.str());
                    //scat_obj.insert(index,value);
                    X.push_back(value);
                }
                Y.push_back(X);
            }
            scat_obj.push_back(Y);
        }
        QJsonDocument Scat(scat_obj);

        QFile file1("scat_field.json");
        if(!file1.open(QIODevice::WriteOnly)) {
            clogs("file scat_field.json error to open","","");
        }
        else {
            clogs("save file scat_field.json","","");
            file1.resize(0);
            file1.write(Scat.toJson());
        }

        file1.close();

//        if(!file2.open(QIODevice::WriteOnly)) {
//           clogs("file scat_field.bin error to open","","");
//        }
//        else {
//            clogs("save file scat_field.bin","","");
//            file2.resize(0);
//            QDataStream stream(&file2);
//            stream. setVersion(QDataStream::Qt_4_2);
//            stream << Scat;
//        }
//        file2.close();
        SCAT_FIELD_TO_FILE = false;
    }

	vEout = fft3(vEout, 1);
	vEout = reorder3(vEout);
	for (size_t iz = 0; iz < vEout.size(); iz++)
		for (size_t iy = 0; iy < vEout[0].size(); iy++)
			for (size_t ix = 0; ix < vEout[0][0].size(); ix++)
				vEout[iz][iy][ix] = sqrt(4. * Pi / countY) * vEout[iz][iy][ix];

    if (FFT_FIELD_TO_FILE) {

        QJsonArray fft_obj = {};
        for (size_t i = 0; i<size3; i++){
            QJsonArray Y = {};
            for (size_t j = 0; j < size2; j++) {
                QJsonArray X = {};
                for (size_t k = 0; k < size1; k++) {
                    QString index, value;
                    stringstream v;
                    index = "[" +QString::number(k)+"]" +
                            "[" +QString::number(j)+"]" +
                            "[" +QString::number(i)+"]";
                    cVect Eout = vEout[i][j][k];
                    v << Eout;
                    value = QString::fromStdString(v.str());
                    X.push_back(value);
                }
                Y.push_back(X);
            }
            fft_obj.push_back(Y);
        }
        QJsonDocument FFT(fft_obj);

        QFile file1("fft_field.json");
        QFile file2("fft_field.bin");
        if(!file1.open(QIODevice::WriteOnly)) {
            clogs("file fft_field.json error to open","","");
        }
        else {
            clogs("save file fft_field.json","","");
            file1.resize(0);
            file1.write(FFT.toJson());
        }

        file1.close();

        if(!file2.open(QIODevice::WriteOnly)) {
           clogs("file fft_field.bin error to open","","");
        }
        else {
            clogs("save file fft_field.bin","","");
            file2.resize(0);
            QDataStream stream(&file2);
            stream. setVersion(QDataStream::Qt_4_2);
            stream << FFT;
        }
        file2.close();
        FFT_FIELD_TO_FILE = false;
    }
	res = 0;
	return res;
}

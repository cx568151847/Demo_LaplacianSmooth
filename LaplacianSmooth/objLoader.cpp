#include "objLoader.h"
#include <fstream>
#include <iostream>
using namespace std;

objLoader::objLoader(string filename)
{
	drawmode = GL_LINE;//Ĭ���ǻ�������
	string line;//��ȡÿһ��
	fstream f;
	f.open(filename, ios::in);//���ļ�
	if (!f.is_open()) {
		cout << "file cannot open" << endl;
	}
	else {
		cout << "file open successful" << endl;
	}

	while (!f.eof()) {
		getline(f, line);
		char breakmarker = ' ';//obj��ÿ�и������Կո����
		string little_tail = " ";
		line = line.append(little_tail);//Ϊlineĩβ���һ���ָ���
		//����ǰ�и��ݿո�ֿ�
		string part = "";
		vector<string> partofline;//��ǰ�и��ݿո��ֳɵĸ���string����
		for (int i = 0; i < line.length(); i++) {
			char nowchar = line[i];
			if (nowchar == breakmarker) {
				//�����ǰcharΪ�ո񣬽�ǰ��Ĳ��ִ���partofline
				partofline.push_back(part);
				part = "";
			}
			else {
				part += nowchar;
			}
		}
		//��ǰ�в�����
		//���ݲ�ͬ�����ݽ�����Ӧ����
		if (partofline.size() == 4) {
			//���ǵ�����������������������ǲ�ֳ�4�����Ǿ�û��Ҫ���
			if (partofline[0] == "v") {
				//��ǰ���Ƕ��㣬�洢��������
				vector<GLdouble> v;
				for (int n = 1; n < 4; n++) {
					//GLdouble xyz = atof(partofline[n].c_str());//ת����double
					GLdouble xyz = atof(partofline[n].c_str());//ת����double
					v.push_back(xyz);
				}
				vset.push_back(v);
			}
			if (partofline[0] == "vn") {
				//��ǰ���Ƕ����������洢����
				vector<GLdouble> vn;
				for (int n = 1; n < 4; n++) {
					//GLdouble xyz = atof(partofline[n].c_str());//ת����double
					GLdouble xyz = atof(partofline[n].c_str());//ת����double
					vn.push_back(xyz);
				}
				vnset.push_back(vn);
			}
			
			if (partofline[0] == "f") {
				//��ǰ�����棬�����������Ͷ�Ӧ�Ķ��㷨�������洢����
				//��Ϊ��ִ�е����ʱ�����ж����Ѿ��洢��ɣ��������Ӧ����洢��������
				vector<GLint> f;
				//��Ϊ���ݸ�ʽ��  ��������//���㷨������
				//Ҫ���м�����б��ȥ��
				for (int n = 1; n < 4; n++) {//��������v//vn
					vector<string> vvn;//��Ŵ���õ�v��vn
					string v_and_vn = partofline[n];// v//vn
					v_and_vn = v_and_vn.append("/");
					string part = "";
					for (int c = 0; c < v_and_vn.length(); c++) {//��v//vn��
						char nowcharofpart = v_and_vn[c];
						//��Ϊ2��/��vvn[0]�洢v,vvn[1]�洢""��vvn[2]�洢vn
						if (nowcharofpart == '/') {
							vvn.push_back(part);
							part = "";
						}
						else {
							part += nowcharofpart;
						}
					}
					//v��vn�Ѿ���ֺò�������vvn�����ڴ����Ӧ��set
					GLint vindex = atof(vvn[0].c_str());
					GLint vnindex = atof(vvn[2].c_str());
					//f�ĽṹΪ { v0 , vn0 , v1 , vn1 , v2 , vn2 }
					f.push_back(vindex);
					f.push_back(vnindex);
					//cout << "v:" << vindex << endl;
					//cout << "vn:" << vnindex << endl;
					//��Ȼ�Ѿ�֪���˶��������������������뵽��Ӧ�Ķ�����ȥ
					//obj�ļ��������Ǵ�1��ʼ�Ķ�����0
					//��ʱ��f��û����ӵ�fset��ȥ��f�����(��0��ʼ)Ӧ��Ϊfset.size()
					vset[vindex - 1].push_back(fset.size());
					//cout << "face number:" << fset.size()<<endl;
				}

				fset.push_back(f);//�����������Ͷ���������������fset
			}
		}
		/*
		//�����ǰ�еĳ�����ֽ��
		cout<<"line��" << line << endl;
		cout << "part number��" << partofline.size()<<endl;
		for (int j = 0; j < partofline.size(); j++) {
			cout << partofline[j] << endl;
		}
		*/
	}

	vset_begin.assign(vset.begin(), vset.end());//��vset����
	f.close();
	/*
	//������ս��
	cout << "���vset" << endl;
	for (int m = 0; m < vset.size(); m++) {
		for (int n = 0; n < vset[m].size(); n++) {
			cout << vset[m][n] << "��";
		}
		cout << endl;
	}
	cout << "���vnset" << endl;
	for (int m = 0; m < vnset.size(); m++) {
		for (int n = 0; n < vnset[m].size(); n++) {
			cout << vnset[m][n] << "��";
		}
		cout << endl;
	}
	cout << "���fset" << endl;
	for (int m = 0; m < fset.size(); m++) {
		for (int n = 0; n < fset[m].size(); n++) {
			cout << fset[m][n] << "��";
		}
		cout << endl;
	}
	*/
}

void objLoader::drawobj() {
	int vindex1, vindex2, vindex3,
		vnindex1,vnindex2,vnindex3;
		glPolygonMode(GL_FRONT_AND_BACK, drawmode);
		for (int i = 0; i < fset.size(); i++) {
			//������1��ʼ��set��0��ʼ
			//��������
			vindex1 = fset[i][0] - 1;
			vindex2 = fset[i][2] - 1;
			vindex3 = fset[i][4] - 1;
			//���㷨������
			vnindex1 = fset[i][1] - 1;
			vnindex2 = fset[i][3] - 1;
			vnindex3 = fset[i][5] - 1;
			glBegin(GL_TRIANGLES);
			if (reverseVertexNormal) {
				glNormal3d(-vnset[vnindex1][0], -vnset[vnindex1][1], -vnset[vnindex1][2]);//���ö��㷨��
				glVertex3d(vset[vindex1][0], vset[vindex1][1], vset[vindex1][2]);
				glNormal3d(-vnset[vnindex2][0], -vnset[vnindex2][1], -vnset[vnindex2][2]);
				glVertex3d(vset[vindex2][0], vset[vindex2][1], vset[vindex2][2]);
				glNormal3d(-vnset[vnindex3][0], -vnset[vnindex3][1], -vnset[vnindex3][2]);
				glVertex3d(vset[vindex3][0], vset[vindex3][1], vset[vindex3][2]);
			}
			else {
				glNormal3d(vnset[vnindex1][0], vnset[vnindex1][1], vnset[vnindex1][2]);//���ö��㷨��
				glVertex3d(vset[vindex1][0], vset[vindex1][1], vset[vindex1][2]);
				glNormal3d(vnset[vnindex2][0], vnset[vnindex2][1], vnset[vnindex2][2]);
				glVertex3d(vset[vindex2][0], vset[vindex2][1], vset[vindex2][2]);
				glNormal3d(vnset[vnindex3][0], vnset[vnindex3][1], vnset[vnindex3][2]);
				glVertex3d(vset[vindex3][0], vset[vindex3][1], vset[vindex3][2]);
			}
			glEnd();
		}
	
}

//�л�����ģʽ
void objLoader::switchdrawmode() {
	if (drawmode == GL_LINE) {
		drawmode = GL_FILL;
	}else if (drawmode == GL_FILL) {
		drawmode = GL_LINE;
	}
}

//�л���ת���㷨��
void objLoader::ReverseVertexNormal() {
	if (reverseVertexNormal) {
		reverseVertexNormal = false;
	}
	else {
		reverseVertexNormal = true;
	}
}

//����������˹���ӽ��������˳��ĵ�
void objLoader::LaplacianCal() {
	for (int i = 0; i < vset.size(); i++) {
		//�ҵ�������Χ�����е�����
		vector<GLint> v_near;//�����Χ������
		
		for (int j = 3; j < vset[i].size(); j++) {
			//���������ö��������face
			//��Ϊvset�洢����double���õ������Ҫת����int
			//vset[012]��Ŷ��㣬֮���ŵ���face������(��0��ʼ��
			int faceindex = (int)vset[i][j];
			for (int k = 0; k < 5; k = k+2) {//����face����������������ע��fset  { v0 , vn0 , v1 , vn1 , v2 , vn2 }
				//����Ƿ��뵱ǰ������һ�����㣬face�д洢����Ŵ�1��ʼ
				if (fset[faceindex][k]-1!=i ) {
					//���ǵĻ�����v_near
					v_near.push_back(fset[faceindex][k]-1);
				}
			}
		}
		
		//����������˹�������Բ���
		double xsum = 0, 
				ysum = 0,
				zsum = 0;
		for (int m = 0; m < v_near.size(); m++) {
			int vindex = v_near[m];
			xsum += vset[vindex][0];
			ysum += vset[vindex][1];
			zsum += vset[vindex][2];
		}
		int numofv = v_near.size();
		double newx = xsum / numofv*para_ofLaplacian + (1-para_ofLaplacian)*vset[i][0],
			newy = ysum / numofv * para_ofLaplacian + (1 - para_ofLaplacian)*vset[i][1],
			newz = zsum / numofv * para_ofLaplacian + (1 - para_ofLaplacian)*vset[i][2];
		vector<GLdouble> newv;
		newv.push_back(newx);
		newv.push_back(newy);
		newv.push_back(newz);
		for (int n = 3; n < vset[i].size(); n++) newv.push_back(vset[i][n]);//��Ҫ���ǽ�face���Ҳ�洢��ȥ��׼����һ��������˹���ӵļ���
		//�洢�¶���
		vset_afterLaplacian.push_back(newv);
		
	}
	vset.clear();
	vset.assign(vset_afterLaplacian.begin(), vset_afterLaplacian.end());
	vset_afterLaplacian.clear();
}

//����vsetΪ�տ�ʼ���������
void objLoader::reset_vset() {
	vset.clear();
	vset.assign(vset_begin.begin(), vset_begin.end());
}

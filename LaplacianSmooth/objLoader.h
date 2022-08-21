#pragma once
#include <GL/freeglut.h>
#include <vector>
#include <string>
using namespace std;
class objLoader {
public:
	objLoader(string filename);//���캯��
	void drawobj();//���ƺ���
	void switchdrawmode();//�л�����ģʽ
	void LaplacianCal();//������˹���Ӵ���
	void reset_vset();
	void ReverseVertexNormal();
private:
	vector<vector<GLdouble>>vset;//��Ŷ���v��xyz����
	vector<vector<GLdouble>>vset_begin;//����Ķ������꣬����rset
	vector<vector<GLdouble>>vset_afterLaplacian;//Laplacian���Ӵ����Ķ���
	vector<vector<GLdouble>> vnset;//��Ŷ��㷨��vn
	vector<vector<GLint>>fset;//�����face�Ķ��������Լ����㷨��
	int drawmode;
	float para_ofLaplacian = 0.5;
	bool reverseVertexNormal = false;
};
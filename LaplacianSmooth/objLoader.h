#pragma once
#include <GL/freeglut.h>
#include <vector>
#include <string>
using namespace std;
class objLoader {
public:
	objLoader(string filename);//构造函数
	void drawobj();//绘制函数
	void switchdrawmode();//切换绘制模式
	void LaplacianCal();//拉普拉斯算子处理
	void reset_vset();
	void ReverseVertexNormal();
private:
	vector<vector<GLdouble>>vset;//存放顶点v的xyz坐标
	vector<vector<GLdouble>>vset_begin;//最初的顶点坐标，用于rset
	vector<vector<GLdouble>>vset_afterLaplacian;//Laplacian算子处理后的顶点
	vector<vector<GLdouble>> vnset;//存放顶点法向vn
	vector<vector<GLint>>fset;//存放面face的顶点索引以及顶点法向
	int drawmode;
	float para_ofLaplacian = 0.5;
	bool reverseVertexNormal = false;
};
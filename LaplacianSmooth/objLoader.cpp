#include "objLoader.h"
#include <fstream>
#include <iostream>
using namespace std;

objLoader::objLoader(string filename)
{
	drawmode = GL_LINE;//默认是绘制轮廓
	string line;//读取每一行
	fstream f;
	f.open(filename, ios::in);//读文件
	if (!f.is_open()) {
		cout << "file cannot open" << endl;
	}
	else {
		cout << "file open successful" << endl;
	}

	while (!f.eof()) {
		getline(f, line);
		char breakmarker = ' ';//obj中每行各部分以空格隔开
		string little_tail = " ";
		line = line.append(little_tail);//为line末尾添加一个分隔符
		//将当前行根据空格分开
		string part = "";
		vector<string> partofline;//当前行根据空格拆分成的各个string部分
		for (int i = 0; i < line.length(); i++) {
			char nowchar = line[i];
			if (nowchar == breakmarker) {
				//如果当前char为空格，将前面的部分存入partofline
				partofline.push_back(part);
				part = "";
			}
			else {
				part += nowchar;
			}
		}
		//当前行拆分完毕
		//根据不同的数据进行相应操作
		if (partofline.size() == 4) {
			//我们的网格是三角形网格，如果不是拆分成4部分那就没必要检查
			if (partofline[0] == "v") {
				//当前行是顶点，存储顶点坐标
				vector<GLdouble> v;
				for (int n = 1; n < 4; n++) {
					//GLdouble xyz = atof(partofline[n].c_str());//转换成double
					GLdouble xyz = atof(partofline[n].c_str());//转换成double
					v.push_back(xyz);
				}
				vset.push_back(v);
			}
			if (partofline[0] == "vn") {
				//当前行是顶点向量，存储向量
				vector<GLdouble> vn;
				for (int n = 1; n < 4; n++) {
					//GLdouble xyz = atof(partofline[n].c_str());//转换成double
					GLdouble xyz = atof(partofline[n].c_str());//转换成double
					vn.push_back(xyz);
				}
				vnset.push_back(vn);
			}
			
			if (partofline[0] == "f") {
				//当前行是面，将顶点索引和对应的顶点法向索引存储起来
				//因为在执行到面的时候所有顶点已经存储完成，将顶点对应的面存储到顶点中
				vector<GLint> f;
				//因为数据格式是  顶点索引//顶点法向索引
				//要将中间两道斜线去掉
				for (int n = 1; n < 4; n++) {//处理三个v//vn
					vector<string> vvn;//存放处理好的v和vn
					string v_and_vn = partofline[n];// v//vn
					v_and_vn = v_and_vn.append("/");
					string part = "";
					for (int c = 0; c < v_and_vn.length(); c++) {//将v//vn拆开
						char nowcharofpart = v_and_vn[c];
						//因为2个/，vvn[0]存储v,vvn[1]存储""，vvn[2]存储vn
						if (nowcharofpart == '/') {
							vvn.push_back(part);
							part = "";
						}
						else {
							part += nowcharofpart;
						}
					}
					//v和vn已经拆分好并存入了vvn，现在存入对应的set
					GLint vindex = atof(vvn[0].c_str());
					GLint vnindex = atof(vvn[2].c_str());
					//f的结构为 { v0 , vn0 , v1 , vn1 , v2 , vn2 }
					f.push_back(vindex);
					f.push_back(vnindex);
					//cout << "v:" << vindex << endl;
					//cout << "vn:" << vnindex << endl;
					//既然已经知道了顶点索引，将面索引存入到对应的顶点中去
					//obj文件的索引是从1开始的而不是0
					//这时候f还没有添加到fset中去，f的序号(从0开始)应该为fset.size()
					vset[vindex - 1].push_back(fset.size());
					//cout << "face number:" << fset.size()<<endl;
				}

				fset.push_back(f);//将顶点索引和顶点向量索引存入fset
			}
		}
		/*
		//输出当前行的初步拆分结果
		cout<<"line：" << line << endl;
		cout << "part number：" << partofline.size()<<endl;
		for (int j = 0; j < partofline.size(); j++) {
			cout << partofline[j] << endl;
		}
		*/
	}

	vset_begin.assign(vset.begin(), vset.end());//将vset备份
	f.close();
	/*
	//输出最终结果
	cout << "输出vset" << endl;
	for (int m = 0; m < vset.size(); m++) {
		for (int n = 0; n < vset[m].size(); n++) {
			cout << vset[m][n] << "，";
		}
		cout << endl;
	}
	cout << "输出vnset" << endl;
	for (int m = 0; m < vnset.size(); m++) {
		for (int n = 0; n < vnset[m].size(); n++) {
			cout << vnset[m][n] << "，";
		}
		cout << endl;
	}
	cout << "输出fset" << endl;
	for (int m = 0; m < fset.size(); m++) {
		for (int n = 0; n < fset[m].size(); n++) {
			cout << fset[m][n] << "，";
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
			//索引从1开始，set从0开始
			//顶点索引
			vindex1 = fset[i][0] - 1;
			vindex2 = fset[i][2] - 1;
			vindex3 = fset[i][4] - 1;
			//顶点法向索引
			vnindex1 = fset[i][1] - 1;
			vnindex2 = fset[i][3] - 1;
			vnindex3 = fset[i][5] - 1;
			glBegin(GL_TRIANGLES);
			if (reverseVertexNormal) {
				glNormal3d(-vnset[vnindex1][0], -vnset[vnindex1][1], -vnset[vnindex1][2]);//设置顶点法向
				glVertex3d(vset[vindex1][0], vset[vindex1][1], vset[vindex1][2]);
				glNormal3d(-vnset[vnindex2][0], -vnset[vnindex2][1], -vnset[vnindex2][2]);
				glVertex3d(vset[vindex2][0], vset[vindex2][1], vset[vindex2][2]);
				glNormal3d(-vnset[vnindex3][0], -vnset[vnindex3][1], -vnset[vnindex3][2]);
				glVertex3d(vset[vindex3][0], vset[vindex3][1], vset[vindex3][2]);
			}
			else {
				glNormal3d(vnset[vnindex1][0], vnset[vnindex1][1], vnset[vnindex1][2]);//设置顶点法向
				glVertex3d(vset[vindex1][0], vset[vindex1][1], vset[vindex1][2]);
				glNormal3d(vnset[vnindex2][0], vnset[vnindex2][1], vnset[vnindex2][2]);
				glVertex3d(vset[vindex2][0], vset[vindex2][1], vset[vindex2][2]);
				glNormal3d(vnset[vnindex3][0], vnset[vnindex3][1], vnset[vnindex3][2]);
				glVertex3d(vset[vindex3][0], vset[vindex3][1], vset[vindex3][2]);
			}
			glEnd();
		}
	
}

//切换绘制模式
void objLoader::switchdrawmode() {
	if (drawmode == GL_LINE) {
		drawmode = GL_FILL;
	}else if (drawmode == GL_FILL) {
		drawmode = GL_LINE;
	}
}

//切换翻转顶点法向
void objLoader::ReverseVertexNormal() {
	if (reverseVertexNormal) {
		reverseVertexNormal = false;
	}
	else {
		reverseVertexNormal = true;
	}
}

//计算拉普拉斯算子进行网格光顺后的点
void objLoader::LaplacianCal() {
	for (int i = 0; i < vset.size(); i++) {
		//找到顶点周围的所有点的序号
		vector<GLint> v_near;//存放周围点的序号
		
		for (int j = 3; j < vset[i].size(); j++) {
			//遍历包含该顶点的所有face
			//因为vset存储的是double，得到序号需要转换成int
			//vset[012]存放顶点，之后存放的是face的索引(从0开始）
			int faceindex = (int)vset[i][j];
			for (int k = 0; k < 5; k = k+2) {//遍历face的三个顶点索引，注意fset  { v0 , vn0 , v1 , vn1 , v2 , vn2 }
				//检查是否与当前顶点是一个顶点，face中存储的序号从1开始
				if (fset[faceindex][k]-1!=i ) {
					//不是的话加入v_near
					v_near.push_back(fset[faceindex][k]-1);
				}
			}
		}
		
		//计算拉普拉斯分量乘以参数
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
		for (int n = 3; n < vset[i].size(); n++) newv.push_back(vset[i][n]);//不要忘记将face序号也存储进去，准备下一次拉普拉斯算子的计算
		//存储新顶点
		vset_afterLaplacian.push_back(newv);
		
	}
	vset.clear();
	vset.assign(vset_afterLaplacian.begin(), vset_afterLaplacian.end());
	vset_afterLaplacian.clear();
}

//重置vset为刚开始读入的坐标
void objLoader::reset_vset() {
	vset.clear();
	vset.assign(vset_begin.begin(), vset_begin.end());
}

#include <cassert>
#include <fstream>
#include "ifs.h"
#include "argparser.h"

#include <vector>
using namespace std;
// some helper stuff for VBOs
#define NUM_CUBE_QUADS 6 
#define NUM_CUBE_VERTS NUM_CUBE_QUADS * 4
#define BUFFER_OFFSET(i) ((char *)NULL + (i))

// ====================================================================
// ====================================================================
IFS::IFS(ArgParser *a) {

  args = a;
  T_length = 0;

  //打印参数
  std::cout << "input file:" << args->input_file << std::endl;
  std::cout << "points:" << to_string(args->points) << std::endl;
  std::cout << "iters:" << to_string(args->iters) << std::endl;
  std::cout << "cube:" << to_string(args->cubes) << std::endl;

  // open the file
  std::ifstream input(args->input_file.c_str());
  if (!input) {
    std::cout << "ERROR:can't read file:"<< args->input_file.c_str() << std::endl;
	system("pause");
    exit(1);
  }
  

  // read the number of transforms
  //读取变换的数量
  int num_transforms; 
  input >> num_transforms;

  //cout << "num_transforms = "<< num_transforms << endl;
  
  double probability= 0.0;
  double temp;
  // read in the transforms
  //将变换及其对应的概率记录下来
  for (int i = 0; i < num_transforms; i++) {
    input >> temp;
	probability += temp;
	p.push_back(probability);
	//cout << "probability = " << probability << endl;

    Matrix m; 
    input >> m;
	T.push_back(m);
	T_length++;
	//cout << "m = " << m << endl;
  }  
  
}

// ====================================================================
// ====================================================================

void IFS::setupVBOs() {
  HandleGLError("enter setup vbos");
  if (args->cubes) {
    setupCube();
  } else {
    setupPoints();
  }
  HandleGLError("leaving setup vbos");
}
void IFS::iter_draw(int iter) {
	if (iter == 0) {
		drawCube();//绘制
	}else {
		for (int i = 0; i < T_length; i++) {
			glPushMatrix();//保存当前矩阵状态
			glTranslated(T[i].get(0, 3), T[i].get(1, 3), T[i].get(2, 3));
			glScaled(T[i].get(0,0), T[i].get(1,1), T[i].get(2,2));
			iter_draw(iter-1);//迭代
			glPopMatrix();//复位
		}
	}
}

void IFS::drawVBOs() {
  HandleGLError("enter draw vbos");
  if (args->cubes) {


    // ASSIGNMENT: don't just draw one cube...
	//递归cube
	  iter_draw(args->iters);


  } else {
    drawPoints();
  }
  HandleGLError("leaving draw vbos");
}

// unfortunately, this is never called
// (but this is not a problem, since this data is used for the life of the program)
void IFS::cleanupVBOs() {
  HandleGLError("enter cleanup vbos");
  if (args->cubes) {
    cleanupCube();
  } else {
    cleanupPoints();
  }
  HandleGLError("leaving cleanup vbos");
}


// ====================================================================
// ====================================================================

void IFS::setupPoints() {

  HandleGLError("in setup points");

  // allocate space for the data
  VBOPoint* points = new VBOPoint[args->points];

  // generate a block of random data
  for (int i = 0; i < args->points; i++) {
    double x = args->mtrand.rand();
    double y = args->mtrand.rand();
    double z = args->mtrand.rand();
    
	for(int k=0;k<args->iters;k++){
    // ASSIGNMENT: manipulate point
	//生成一个概率来选择使用哪个变换
	double probability = args->mtrand.rand();

	int j = 0;
	for (j; j < T_length; j++) {
		if (probability < p[j]) {
			break;
		}
	}
	Matrix m = T[j];
	Vec4f temp = Vec4f(x, y, z, 1.0);
	m.Transform(temp);
	x = temp.x();
	y = temp.y();
	z = temp.z();

    points[i] = VBOPoint(Vec3f(x,y,z));
  }
  }
  // create a pointer for the VBO
  glGenBuffers(1, &points_VBO);

  // copy the data to the VBO
  glBindBuffer(GL_ARRAY_BUFFER,points_VBO); 
  glBufferData(GL_ARRAY_BUFFER,
	       sizeof(VBOPoint) * args->points,
	       points,
	       GL_STATIC_DRAW); 

  delete [] points;

  HandleGLError("leaving setup points");
}


void IFS::drawPoints() const {
  glColor3f(0,0,0); 
  glPointSize(1);

  HandleGLError("in draw points");

  // select the point buffer
  glBindBuffer(GL_ARRAY_BUFFER, points_VBO);
  glEnableClientState(GL_VERTEX_ARRAY);
  // describe the data
  glVertexPointer(3, GL_FLOAT, 0, 0);
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
  // draw this data
  glDrawArrays(GL_POINTS, 0, args->points);
  
  glDisableClientState(GL_VERTEX_ARRAY);
  glDisableVertexAttribArray(0);

  HandleGLError("leaving draw points");
}


void IFS::cleanupPoints() {
  glDeleteBuffers(1, &points_VBO);
}

// ====================================================================
// ====================================================================
void IFS::changeCubeColor(int face, float c1, float c2, float c3) {
	if (face < 0 || face>23) {
		cout << "invalid param 1" << endl;
		return;
	}
	else {
		int index = face % 4;
		if (c1 < 0) c1 = cube_verts[index].cx;
		if (c2 < 0) c1 = cube_verts[index].cy;
		if (c3 < 0) c1 = cube_verts[index].cz;

		for (int i = 0; i < 4; i++) {
			index += i;
			
			cube_verts[index] = VBOVertex(
				Vec3f(cube_verts[index].x, cube_verts[index].y, cube_verts[index].z),
				Vec3f(cube_verts[index].nx, cube_verts[index].ny, cube_verts[index].nz),
				Vec3f(c1, c2, c3));
		}
	}
	glBufferData(GL_ARRAY_BUFFER,
		sizeof(VBOVertex) * NUM_CUBE_VERTS,
		cube_verts,
		GL_STATIC_DRAW);
	return;
}

void IFS::setupCube() {

	HandleGLError("in setup cube");
	// allocate space for the data
	cube_verts = new VBOVertex[NUM_CUBE_VERTS];
	VBOQuad* cube_face_indices = new VBOQuad[NUM_CUBE_QUADS];
	/*
	// initialize vertex data
  // back face, cyan
	cube_verts[0] = VBOVertex(Vec3f(0, 0, 0), Vec3f(0, 0, -1), Vec3f(0, 1, 1));
	cube_verts[1] = VBOVertex(Vec3f(0, 1, 0), Vec3f(0, 0, -1), Vec3f(0, 1, 1));
	cube_verts[2] = VBOVertex(Vec3f(1, 1, 0), Vec3f(0, 0, -1), Vec3f(0, 1, 1));
	cube_verts[3] = VBOVertex(Vec3f(1, 0, 0), Vec3f(0, 0, -1), Vec3f(0, 1, 1));
	// front face, red
	cube_verts[4] = VBOVertex(Vec3f(0, 0, 1), Vec3f(0, 0, 1), Vec3f(1, 0, 0));
	cube_verts[5] = VBOVertex(Vec3f(0, 1, 1), Vec3f(0, 0, 1), Vec3f(1, 0, 0));
	cube_verts[6] = VBOVertex(Vec3f(1, 1, 1), Vec3f(0, 0, 1), Vec3f(1, 0, 0));
	cube_verts[7] = VBOVertex(Vec3f(1, 0, 1), Vec3f(0, 0, 1), Vec3f(1, 0, 0));
	// bottom face, purple
	cube_verts[8] = VBOVertex(Vec3f(0, 0, 0), Vec3f(0, -1, 0), Vec3f(1, 0, 1));
	cube_verts[9] = VBOVertex(Vec3f(0, 0, 1), Vec3f(0, -1, 0), Vec3f(1, 0, 1));
	cube_verts[10] = VBOVertex(Vec3f(1, 0, 1), Vec3f(0, -1, 0), Vec3f(1, 0, 1));
	cube_verts[11] = VBOVertex(Vec3f(1, 0, 0), Vec3f(0, -1, 0), Vec3f(1, 0, 1));
	// top face, green
	cube_verts[12] = VBOVertex(Vec3f(0, 1, 0), Vec3f(0, 1, 0), Vec3f(0, 1, 0));
	cube_verts[13] = VBOVertex(Vec3f(0, 1, 1), Vec3f(0, 1, 0), Vec3f(0, 1, 0));
	cube_verts[14] = VBOVertex(Vec3f(1, 1, 1), Vec3f(0, 1, 0), Vec3f(0, 1, 0));
	cube_verts[15] = VBOVertex(Vec3f(1, 1, 0), Vec3f(0, 1, 0), Vec3f(0, 1, 0));
	// left face, yellow
	cube_verts[16] = VBOVertex(Vec3f(0, 0, 0), Vec3f(-1, 0, 0), Vec3f(1, 1, 0));
	cube_verts[17] = VBOVertex(Vec3f(0, 0, 1), Vec3f(-1, 0, 0), Vec3f(1, 1, 0));
	cube_verts[18] = VBOVertex(Vec3f(0, 1, 1), Vec3f(-1, 0, 0), Vec3f(1, 1, 0));
	cube_verts[19] = VBOVertex(Vec3f(0, 1, 0), Vec3f(-1, 0, 0), Vec3f(1, 1, 0));
	// right face, blue
	cube_verts[20] = VBOVertex(Vec3f(1, 0, 0), Vec3f(1, 0, 0), Vec3f(0, 0, 1));
	cube_verts[21] = VBOVertex(Vec3f(1, 0, 1), Vec3f(1, 0, 0), Vec3f(0, 0, 1));
	cube_verts[22] = VBOVertex(Vec3f(1, 1, 1), Vec3f(1, 0, 0), Vec3f(0, 0, 1));
	cube_verts[23] = VBOVertex(Vec3f(1, 1, 0), Vec3f(1, 0, 0), Vec3f(0, 0, 1));
	*/
	// initialize vertex data
   // back face, cyan
	cube_verts[0] = VBOVertex(Vec3f(0, 0, 0), Vec3f(0, 0, -1), Vec3f(0, 1, 1));
	cube_verts[1] = VBOVertex(Vec3f(0, 1, 0), Vec3f(0, 0, -1), Vec3f(0, 0.6, 1));
	cube_verts[2] = VBOVertex(Vec3f(1, 1, 0), Vec3f(0, 0, -1), Vec3f(0, 0.4, 1));
	cube_verts[3] = VBOVertex(Vec3f(1, 0, 0), Vec3f(0, 0, -1), Vec3f(0, 0.2, 1));
	// front face, red
	cube_verts[4] = VBOVertex(Vec3f(0, 0, 1), Vec3f(0, 0, 1), Vec3f(1, 0, 0));
	cube_verts[5] = VBOVertex(Vec3f(0, 1, 1), Vec3f(0, 0, 1), Vec3f(0.8, 0, 0));
	cube_verts[6] = VBOVertex(Vec3f(1, 1, 1), Vec3f(0, 0, 1), Vec3f(0.6, 0, 0));
	cube_verts[7] = VBOVertex(Vec3f(1, 0, 1), Vec3f(0, 0, 1), Vec3f(0.4, 0, 0));
	// bottom face, purple
	cube_verts[8] = VBOVertex(Vec3f(0, 0, 0), Vec3f(0, -1, 0), Vec3f(1, 0, 1));
	cube_verts[9] = VBOVertex(Vec3f(0, 0, 1), Vec3f(0, -1, 0), Vec3f(0.7, 0, 1));
	cube_verts[10] = VBOVertex(Vec3f(1, 0, 1), Vec3f(0, -1, 0), Vec3f(0.5, 0, 1));
	cube_verts[11] = VBOVertex(Vec3f(1, 0, 0), Vec3f(0, -1, 0), Vec3f(0.2, 0, 1));
	// top face, green
	cube_verts[12] = VBOVertex(Vec3f(0, 1, 0), Vec3f(0, 1, 0), Vec3f(0, 1, 0));
	cube_verts[13] = VBOVertex(Vec3f(0, 1, 1), Vec3f(0, 1, 0), Vec3f(0, 0.2, 0));
	cube_verts[14] = VBOVertex(Vec3f(1, 1, 1), Vec3f(0, 1, 0), Vec3f(0.3, 0.6, 0));
	cube_verts[15] = VBOVertex(Vec3f(1, 1, 0), Vec3f(0, 1, 0), Vec3f(0, 0.1, 0.9));
	// left face, yellow
	cube_verts[16] = VBOVertex(Vec3f(0, 0, 0), Vec3f(-1, 0, 0), Vec3f(1, 1, 0));
	cube_verts[17] = VBOVertex(Vec3f(0, 0, 1), Vec3f(-1, 0, 0), Vec3f(1, 0.1, 0));
	cube_verts[18] = VBOVertex(Vec3f(0, 1, 1), Vec3f(-1, 0, 0), Vec3f(1, 0.6, 0));
	cube_verts[19] = VBOVertex(Vec3f(0, 1, 0), Vec3f(-1, 0, 0), Vec3f(1, 0.4, 0));
	// right face, blue
	cube_verts[20] = VBOVertex(Vec3f(1, 0, 0), Vec3f(1, 0, 0), Vec3f(0, 0, 1));
	cube_verts[21] = VBOVertex(Vec3f(1, 0, 1), Vec3f(1, 0, 0), Vec3f(0, 0, 0.6));
	cube_verts[22] = VBOVertex(Vec3f(1, 1, 1), Vec3f(1, 0, 0), Vec3f(0, 0, 0.2));
	cube_verts[23] = VBOVertex(Vec3f(1, 1, 0), Vec3f(1, 0, 0), Vec3f(0, 0, 0.3));
	
	// initialize face data
	cube_face_indices[0] = VBOQuad(0, 1, 2, 3);
	cube_face_indices[1] = VBOQuad(4, 7, 6, 5);
	cube_face_indices[2] = VBOQuad(8, 11, 10, 9);
	cube_face_indices[3] = VBOQuad(12, 13, 14, 15);
	cube_face_indices[4] = VBOQuad(16, 17, 18, 19);
	cube_face_indices[5] = VBOQuad(20, 23, 22, 21);

	// create a pointer for the vertex & index VBOs
	glGenBuffers(1, &cube_verts_VBO);
	glGenBuffers(1, &cube_face_indices_VBO);

	// copy the data to each VBO
	glBindBuffer(GL_ARRAY_BUFFER, cube_verts_VBO);
	glBufferData(GL_ARRAY_BUFFER,
		sizeof(VBOVertex) * NUM_CUBE_VERTS,
		cube_verts,
		GL_STATIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, cube_face_indices_VBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER,
		sizeof(unsigned int) * NUM_CUBE_QUADS * 4,
		cube_face_indices, GL_STATIC_DRAW);

	HandleGLError("leaving setup cube");

	//delete[] cube_verts;
	delete[] cube_face_indices;
}

void IFS::drawCube() const {

  HandleGLError("in draw cube");
  // select the vertex buffer
  glBindBuffer(GL_ARRAY_BUFFER, cube_verts_VBO);
  // describe the layout of data in the vertex buffer
  glEnableClientState(GL_VERTEX_ARRAY);
  glVertexPointer(3, GL_FLOAT, sizeof(VBOVertex), BUFFER_OFFSET(0));
  glEnableClientState(GL_NORMAL_ARRAY);
  glNormalPointer(GL_FLOAT, sizeof(VBOVertex), BUFFER_OFFSET(12));
  glEnableClientState(GL_COLOR_ARRAY);
  glColorPointer(3, GL_FLOAT, sizeof(VBOVertex), BUFFER_OFFSET(24));

  // select the index buffer
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, cube_face_indices_VBO);
  // draw this data
  glDrawElements(GL_QUADS, 
		 NUM_CUBE_QUADS*4, 
		 GL_UNSIGNED_INT,
		 BUFFER_OFFSET(0));

  glDisableClientState(GL_NORMAL_ARRAY);
  glDisableClientState(GL_COLOR_ARRAY);
  glDisableClientState(GL_VERTEX_ARRAY);

  HandleGLError("leaving draw cube");
}


void IFS::cleanupCube() {
  glDeleteBuffers(1, &cube_verts_VBO);
  glDeleteBuffers(1, &cube_face_indices_VBO);
}

// ====================================================================
// ====================================================================


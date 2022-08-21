#include "glCanvas.h"
#include <fstream>
#include <sstream>

#include "mesh.h"
#include "edge.h"
#include "vertex.h"
#include "triangle.h"
using namespace std;

int Triangle::next_triangle_id = 0;//静态变量，来为每一个triangle分配一个id

// helper for VBOs
#define BUFFER_OFFSET(i) ((char *)NULL + (i))


// =======================================================================
// MESH DESTRUCTOR 
// =======================================================================

Mesh::~Mesh() {
  cleanupVBOs();

  // delete all the triangles
  std::vector<Triangle*> todo;
  for (triangleshashtype::iterator iter = triangles.begin();
       iter != triangles.end(); iter++) {
    Triangle *t = iter->second;
    todo.push_back(t);
  }
  int num_triangles = todo.size();
  for (int i = 0; i < num_triangles; i++) {
    removeTriangle(todo[i]);
  }
  // delete all the vertices
  int num_vertices = numVertices();
  for (int i = 0; i < num_vertices; i++) {
    delete vertices[i];
  }
}

// =======================================================================
// MODIFIERS:   ADD & REMOVE
// =======================================================================

Vertex* Mesh::addVertex(const Vec3f &position) {
	int index = numVertices();//序号为当前数量，例如一个数也没有，则index为0；有一个数，index为1
	Vertex *v = new Vertex(index, position);
	vertices.push_back(v);
	if (numVertices() == 1){//如果只有一个点，创建boundingbpx
		bbox = BoundingBox(position, position);
	}
	else {//如果已经有点，向boundingbox添加点
		bbox.Extend(position);
	}
    return v;
}

//三个顶点参数是有顺序的
//addTriangle(a,b,c)和addTriangle(c,b,a)生成的三角形是不一样的
void Mesh::addTriangle(Vertex *a, Vertex *b, Vertex *c) {
  // create the triangle
  Triangle *t = new Triangle();
  // create the edges
  Edge *ea = new Edge(a,b,t);
  Edge *eb = new Edge(b,c,t);
  Edge *ec = new Edge(c,a,t);
  // point the triangle to one of its edges
  t->setEdge(ea);
  // connect the edges to each other
  ea->setNext(eb);
  eb->setNext(ec);
  ec->setNext(ea);
  //cout << "vertex id : " << a->getIndex() << endl;
  //cout << "vertex id : " << b->getIndex() << endl;
  //cout << "vertex id : " << c->getIndex() << endl;
  // verify these edges aren't already in the mesh 
  // (which would be a bug, or a non-manifold mesh)
  assert (edges.find(std::make_pair(a,b)) == edges.end());
  assert (edges.find(std::make_pair(b,c)) == edges.end());
  assert (edges.find(std::make_pair(c,a)) == edges.end());
  // add the edges to the master list
  edges[std::make_pair(a,b)] = ea;
  edges[std::make_pair(b,c)] = eb;
  edges[std::make_pair(c,a)] = ec;
  // connect up with opposite edges (if they exist)
  //寻找opposite并链接起来
  edgeshashtype::iterator ea_op = edges.find(std::make_pair(b,a)); 
  edgeshashtype::iterator eb_op = edges.find(std::make_pair(c,b)); 
  edgeshashtype::iterator ec_op = edges.find(std::make_pair(a,c)); 
  
  if (ea_op != edges.end()) { ea_op->second->setOpposite(ea); }
  if (eb_op != edges.end()) { eb_op->second->setOpposite(eb); }
  if (ec_op != edges.end()) { ec_op->second->setOpposite(ec); }
  // add the triangle to the master list
  assert (triangles.find(t->getID()) == triangles.end());
  triangles[t->getID()] = t;
}


void Mesh::removeTriangle(Triangle *t) {
  Edge *ea = t->getEdge();
  Edge *eb = ea->getNext();
  Edge *ec = eb->getNext();
  Vertex *a = ea->getStartVertex();
  Vertex *b = eb->getStartVertex();
  Vertex *c = ec->getStartVertex();
  // remove these elements from master lists
  edges.erase(std::make_pair(a,b)); 
  edges.erase(std::make_pair(b,c)); 
  edges.erase(std::make_pair(c,a)); 
  triangles.erase(t->getID());
  // clean up memory
  delete ea;
  delete eb;
  delete ec;
  delete t;
}


// =======================================================================
// Helper functions for accessing data in the hash table
// =======================================================================

Edge* Mesh::getMeshEdge(Vertex *a, Vertex *b) const {
  edgeshashtype::const_iterator iter = edges.find(std::make_pair(a,b));
  if (iter == edges.end()) return NULL;
  return iter->second;
}

Vertex* Mesh::getChildVertex(Vertex *p1, Vertex *p2) const {
  vphashtype::const_iterator iter = vertex_parents.find(std::make_pair(p1,p2)); 
  if (iter == vertex_parents.end()) return NULL;
  return iter->second; 
}

void Mesh::setParentsChild(Vertex *p1, Vertex *p2, Vertex *child) {
  //assert (vertex_parents.find(std::make_pair(p1,p2)) == vertex_parents.end());//确保vertex_parents还未添加pair(p1,p2)
  vertex_parents[std::make_pair(p1,p2)] = child; //添加child
}


// =======================================================================
// the load function parses very simple .obj files
// the basic format has been extended to allow the specification 
// of crease weights on the edges.
// =======================================================================

#define MAX_CHAR_PER_LINE 200

void Mesh::Load(const std::string &input_file) {
  std::cout << "input:"<< input_file.c_str() << std::endl;
  std::ifstream istr(input_file.c_str());
  if (!istr) {
    std::cout << "ERROR! CANNOT OPEN: " << input_file << std::endl;
    return;
  }

  char line[MAX_CHAR_PER_LINE];
  std::string token, token2;
  float x,y,z;
  int a,b,c;
  int index = 0;
  int vert_count = 0;
  int vert_index = 1;

  // read in each line of the file
  while (istr.getline(line,MAX_CHAR_PER_LINE)) { 
    // put the line into a stringstream for parsing
    std::stringstream ss;
    ss << line;

    // check for blank line
    token = "";   
    ss >> token;
    if (token == "") continue;

    if (token == std::string("usemtl") ||token == std::string("g")) {
      vert_index = 1; 
      index++;
    } else if (token == std::string("v")) {//v 顶点
      vert_count++;//顶点数++
      ss >> x >> y >> z;
      addVertex(Vec3f(x,y,z));//添加顶点
    } else if (token == std::string("f")) {
	  //f 三角 vn才是法向
      //f f v/vt/vn v/vt/vn v/vt/vn（f 顶点索引 / 纹理坐标索引 / 顶点法向量索引）
	  //一般的 f v v v 是3个顶点索引
      a = b = c = -1;
      ss >> a >> b >> c;
      a -= vert_index;
      b -= vert_index;
      c -= vert_index;
      assert (a >= 0 && a < numVertices());
      assert (b >= 0 && b < numVertices());
      assert (c >= 0 && c < numVertices());
      addTriangle(getVertex(a),getVertex(b),getVertex(c));
    } else if (token == std::string("e")) {
      a = b = -1;
      ss >> a >> b >> token2;
	  //edge和vertex结构不一样，不需要减一
      // whoops: inconsistent file format, don't subtract 1
      assert (a >= 0 && a <= numVertices());
      assert (b >= 0 && b <= numVertices());
      if (token2 == std::string("inf")) x = 1000000; // this is close to infinity...
      x = atof(token2.c_str());//字符串转浮点数
      Vertex *va = getVertex(a);
      Vertex *vb = getVertex(b);
      Edge *ab = getMeshEdge(va,vb);
      Edge *ba = getMeshEdge(vb,va);
      assert (ab != NULL);
      assert (ba != NULL);
      ab->setCrease(x);
      ba->setCrease(x);
	  //为顶点s++
	  va->setS(va->getS() + 1);
	  vb->setS(vb->getS() + 1);

    } else if (token == std::string("vt")) {
    } else if (token == std::string("vn")) {
    } else if (token[0] == '#') {
    } else {
      printf ("LINE: '%s'",line);
    }
  }

  calculate_edge_cost();//计算所有边的QEM
}


// =======================================================================
// DRAWING
// =======================================================================

Vec3f ComputeNormal(const Vec3f &p1, const Vec3f &p2, const Vec3f &p3) {
  Vec3f v12 = p2;
  v12 -= p1;
  Vec3f v23 = p3;
  v23 -= p2;
  Vec3f normal;
  Vec3f::Cross3(normal,v12,v23);
  normal.Normalize();
  return normal;
}


void Mesh::initializeVBOs() {
  // create a pointer for the vertex & index VBOs
  glGenBuffers(1, &mesh_tri_verts_VBO);
  glGenBuffers(1, &mesh_tri_indices_VBO);
  glGenBuffers(1, &mesh_verts_VBO);
  glGenBuffers(1, &mesh_boundary_edge_indices_VBO);
  glGenBuffers(1, &mesh_crease_edge_indices_VBO);
  glGenBuffers(1, &mesh_other_edge_indices_VBO);
  setupVBOs();
}

void Mesh::setupVBOs() {
  HandleGLError("in setup mesh VBOs");
  setupTriVBOs();
  setupEdgeVBOs();
  HandleGLError("leaving setup mesh");
}


void Mesh::setupTriVBOs() {

  VBOTriVert* mesh_tri_verts;
  VBOTri* mesh_tri_indices;
  unsigned int num_tris = triangles.size();

  // allocate space for the data
  mesh_tri_verts = new VBOTriVert[num_tris*3];
  mesh_tri_indices = new VBOTri[num_tris];

  // write the vertex & triangle data
  unsigned int i = 0;
  triangleshashtype::iterator iter = triangles.begin();
  for (; iter != triangles.end(); iter++,i++) {
    Triangle *t = iter->second;
    Vec3f a = (*t)[0]->getPos();
    Vec3f b = (*t)[1]->getPos();
    Vec3f c = (*t)[2]->getPos();
    
    if (args->gouraud) {


      // =====================================
      // ASSIGNMENT: reimplement 
      Vec3f normal = ComputeNormal(a,b,c);
      mesh_tri_verts[i*3]   = VBOTriVert(a,normal);
      mesh_tri_verts[i*3+1] = VBOTriVert(b,normal);
      mesh_tri_verts[i*3+2] = VBOTriVert(c,normal);
      // =====================================


    } else {
      Vec3f normal = ComputeNormal(a,b,c);
      mesh_tri_verts[i*3]   = VBOTriVert(a,normal);
      mesh_tri_verts[i*3+1] = VBOTriVert(b,normal);
      mesh_tri_verts[i*3+2] = VBOTriVert(c,normal);
    }
    mesh_tri_indices[i] = VBOTri(i*3,i*3+1,i*3+2);
  }

  // cleanup old buffer data (if any)
  glDeleteBuffers(1, &mesh_tri_verts_VBO);
  glDeleteBuffers(1, &mesh_tri_indices_VBO);

  // copy the data to each VBO
  glBindBuffer(GL_ARRAY_BUFFER,mesh_tri_verts_VBO); 
  glBufferData(GL_ARRAY_BUFFER,
	       sizeof(VBOTriVert) * num_tris * 3,
	       mesh_tri_verts,
	       GL_STATIC_DRAW); 
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,mesh_tri_indices_VBO); 
  glBufferData(GL_ELEMENT_ARRAY_BUFFER,
	       sizeof(VBOTri) * num_tris,
	       mesh_tri_indices, GL_STATIC_DRAW);

  delete [] mesh_tri_verts;
  delete [] mesh_tri_indices;

}


void Mesh::setupEdgeVBOs() {

  VBOVert* mesh_verts;
  VBOEdge* mesh_boundary_edge_indices;
  VBOEdge* mesh_crease_edge_indices;
  VBOEdge* mesh_other_edge_indices;

  mesh_boundary_edge_indices = NULL;
  mesh_crease_edge_indices = NULL;
  mesh_other_edge_indices = NULL;

  unsigned int num_verts = vertices.size();

  // first count the edges of each type
  num_boundary_edges = 0;
  num_crease_edges = 0;
  num_other_edges = 0;
  for (edgeshashtype::iterator iter = edges.begin();iter != edges.end(); iter++) {
    Edge *e = iter->second;
    int a = e->getStartVertex()->getIndex();
    int b = e->getEndVertex()->getIndex();
    if (e->getOpposite() == NULL) {//如果没有opposite，则是边界边
      num_boundary_edges++;
    } else {//如果a<b说明一个三角形
      if (a < b) continue; // don't double count edges!
      if (e->getCrease() > 0) num_crease_edges++;
      else num_other_edges++;
    }
  }

  // allocate space for the data
  mesh_verts = new VBOVert[num_verts];
  if (num_boundary_edges > 0)
    mesh_boundary_edge_indices = new VBOEdge[num_boundary_edges];
  if (num_crease_edges > 0)
    mesh_crease_edge_indices = new VBOEdge[num_crease_edges];
  if (num_other_edges > 0)
    mesh_other_edge_indices = new VBOEdge[num_other_edges];

  // write the vertex data
  for (unsigned int i = 0; i < num_verts; i++) {
    mesh_verts[i] = VBOVert(vertices[i]->getPos());
  }

  // write the edge data
  int bi = 0;
  int ci = 0;
  int oi = 0; 
  for (edgeshashtype::iterator iter = edges.begin();
       iter != edges.end(); iter++) {
    Edge *e = iter->second;
    int a = e->getStartVertex()->getIndex();
    int b = e->getEndVertex()->getIndex();
    if (e->getOpposite() == NULL) {
      mesh_boundary_edge_indices[bi++] = VBOEdge(a,b);
    } else {
      if (a < b) continue; // don't double count edges!
      if (e->getCrease() > 0) 
	mesh_crease_edge_indices[ci++] = VBOEdge(a,b);
      else 
	mesh_other_edge_indices[oi++] = VBOEdge(a,b);
    }
  }
  assert (bi == num_boundary_edges);
  assert (ci == num_crease_edges);
  assert (oi == num_other_edges);

  // cleanup old buffer data (if any)
  glDeleteBuffers(1, &mesh_verts_VBO);
  glDeleteBuffers(1, &mesh_boundary_edge_indices_VBO);
  glDeleteBuffers(1, &mesh_crease_edge_indices_VBO);
  glDeleteBuffers(1, &mesh_other_edge_indices_VBO);

  // copy the data to each VBO
  glBindBuffer(GL_ARRAY_BUFFER,mesh_verts_VBO); 
  glBufferData(GL_ARRAY_BUFFER,
	       sizeof(VBOVert) * num_verts,
	       mesh_verts,
	       GL_STATIC_DRAW); 

  if (num_boundary_edges > 0) {
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,mesh_boundary_edge_indices_VBO); 
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
		 sizeof(VBOEdge) * num_boundary_edges,
		 mesh_boundary_edge_indices, GL_STATIC_DRAW);
  }
  if (num_crease_edges > 0) {
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,mesh_crease_edge_indices_VBO); 
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
		 sizeof(VBOEdge) * num_crease_edges,
		 mesh_crease_edge_indices, GL_STATIC_DRAW);
  }
  if (num_other_edges > 0) {
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,mesh_other_edge_indices_VBO); 
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
		 sizeof(VBOEdge) * num_other_edges,
		 mesh_other_edge_indices, GL_STATIC_DRAW);
  }

  delete [] mesh_verts;
  delete [] mesh_boundary_edge_indices;
  delete [] mesh_crease_edge_indices;
  delete [] mesh_other_edge_indices;
}


void Mesh::cleanupVBOs() {
  glDeleteBuffers(1, &mesh_tri_verts_VBO);
  glDeleteBuffers(1, &mesh_tri_indices_VBO);
  glDeleteBuffers(1, &mesh_verts_VBO);
  glDeleteBuffers(1, &mesh_boundary_edge_indices_VBO);
  glDeleteBuffers(1, &mesh_crease_edge_indices_VBO);
  glDeleteBuffers(1, &mesh_other_edge_indices_VBO);
}


void Mesh::drawVBOs() {

  HandleGLError("in draw mesh");

  // scale it so it fits in the window
  Vec3f center; bbox.getCenter(center);
  float s = 1/bbox.maxDim();
  glScalef(s,s,s);
  glTranslatef(-center.x(),-center.y(),-center.z());

  // this offset prevents "z-fighting" bewteen the edges and faces
  // so the edges will always win
  if (args->wireframe) {
    glEnable(GL_POLYGON_OFFSET_FILL);
    glPolygonOffset(1.1,4.0); 
  } 

  // ======================
  // draw all the triangles
  unsigned int num_tris = triangles.size();
  glColor3f(1,1,1);

  // select the vertex buffer
  glBindBuffer(GL_ARRAY_BUFFER, mesh_tri_verts_VBO);
  // describe the layout of data in the vertex buffer
  glEnableClientState(GL_VERTEX_ARRAY);
  glVertexPointer(3, GL_FLOAT, sizeof(VBOTriVert), BUFFER_OFFSET(0));
  glEnableClientState(GL_NORMAL_ARRAY);
  glNormalPointer(GL_FLOAT, sizeof(VBOTriVert), BUFFER_OFFSET(12));

  // select the index buffer
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh_tri_indices_VBO);
  // draw this data
  glDrawElements(GL_TRIANGLES, 
		 num_tris*3,
		 GL_UNSIGNED_INT,
		 BUFFER_OFFSET(0));

  glDisableClientState(GL_NORMAL_ARRAY);
  glDisableClientState(GL_VERTEX_ARRAY);

  if (args->wireframe) {
    glDisable(GL_POLYGON_OFFSET_FILL); 
  }

  // =================================
  // draw the different types of edges
  if (args->wireframe) {
    glDisable(GL_LIGHTING);

    // select the vertex buffer
    glBindBuffer(GL_ARRAY_BUFFER, mesh_verts_VBO);
    // describe the layout of data in the vertex buffer
    glEnableClientState(GL_VERTEX_ARRAY);
    glVertexPointer(3, GL_FLOAT, sizeof(VBOVert), BUFFER_OFFSET(0));

    // draw all the boundary edges
    glLineWidth(3);
    glColor3f(1,0,0);
    // select the index buffer
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh_boundary_edge_indices_VBO);
    // draw this data
    glDrawElements(GL_LINES, num_boundary_edges*2, GL_UNSIGNED_INT, BUFFER_OFFSET(0));

    // draw all the interior, crease edges
    glLineWidth(3);
    glColor3f(1,1,0);
    // select the index buffer
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh_crease_edge_indices_VBO);
    // draw this data
    glDrawElements(GL_LINES, num_crease_edges*2, GL_UNSIGNED_INT, BUFFER_OFFSET(0));

    // draw all the interior, non-crease edges
    glLineWidth(1);
    glColor3f(0,0,0);
    // select the index buffer
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh_other_edge_indices_VBO);
    // draw this data
    glDrawElements(GL_LINES, num_other_edges*2, GL_UNSIGNED_INT, BUFFER_OFFSET(0));

    glDisableClientState(GL_VERTEX_ARRAY);
  }

  HandleGLError("leaving draw VBOs");
}

// =================================================================
// SUBDIVISION
// =================================================================


//Implement the butterfly interpolating subdivision surface scheme.
void Mesh::ModifiedButterflySubdivision(){
	cout << "Modified Butterfly Subdivision" << endl;
	//为每一条边插入一个顶点
	edgeshashtype::iterator iter_edges = edges.begin();
	for (; iter_edges != edges.end(); ++iter_edges){
		Edge *e = iter_edges->second;
		Vertex *v1 = e->getStartVertex();
		Vertex *v2 = e->getEndVertex();

		//如果已经有child则不修改
		if (getChildVertex(v1, v2) != NULL) {
			continue;
		}

		//开始添加新顶点
		if (e->getOpposite() == NULL) {
			//e是边界边
			//找到两个点的另外两条边界边
			Edge *boundary_edge_1;
			Edge *boundary_edge_2;
			//找v1连接的另外一条边界边
			Edge *e_temp = e->getNext()->getNext();
			while (e_temp->getOpposite()!=NULL) {
				e_temp = e_temp->getOpposite()->getNext()->getNext();
			}
			boundary_edge_1 = e_temp;
			//找v2连接的另外一条边界边
			e_temp = e->getNext();
			while (e_temp->getOpposite() != NULL) {
				e_temp = e_temp->getOpposite()->getNext();
			}
			boundary_edge_2 = e_temp;
			//计算新顶点
			Vec3f new_vertex_pos = 0.5625*(v1->getPos() + v2->getPos()) - 0.0625*(boundary_edge_1->getStartVertex()->getPos() + boundary_edge_2->getEndVertex()->getPos());
			Vertex *new_vertex = addVertex(new_vertex_pos);
			setParentsChild(v1, v2, new_vertex);
		}
		else if (e->getCrease() > 0) {
			//e是crease边
			//找到v1和v2周围的crease边，各自平均-1/16
			vector<Edge *> v1_around_crease_edges;
			vector<Edge *> v2_around_crease_edges;
			//找v1连接的另外一条边界边
			Edge *e_temp = e->getNext()->getNext();
			//当遇到边界边或自身时终止
			while (e_temp->getOpposite() != NULL&& e_temp->getOpposite()!=e) {
				if (e_temp->getCrease() > 0) {
					v1_around_crease_edges.push_back(e_temp);
				}
				e_temp = e_temp->getOpposite()->getNext()->getNext();
			}
			if (e_temp->getOpposite() == e) {
				//继续执行
			}
			if (e_temp->getOpposite() == NULL) {
				//继续找另一边
				e_temp = e->getOpposite()->getNext();
				while (e_temp->getOpposite() != NULL) {
					if (e_temp->getCrease() > 0) {
						v1_around_crease_edges.push_back(e_temp);
					}
					e_temp = e_temp->getOpposite()->getNext();
				}
				if (e_temp->getCrease() > 0) {
					v1_around_crease_edges.push_back(e_temp);
				}
				//直到遇到边界边结束
			}
			//v1_around_crease_edges已经存储了v1周围所有的crease边

			//找v2连接的另外一条边界边
			e = e->getOpposite();
			e_temp = e->getNext()->getNext();
			//当遇到边界边或自身时终止
			while (e_temp->getOpposite() != NULL && e_temp->getOpposite() != e) {
				if (e_temp->getCrease() > 0) {
					v2_around_crease_edges.push_back(e_temp);
				}
				e_temp = e_temp->getOpposite()->getNext()->getNext();
			}
			if (e_temp->getOpposite() == e) {
				//继续执行

			}
			if (e_temp->getOpposite() == NULL) {
				//继续找另一边
				e_temp = e->getOpposite()->getNext();
				while (e_temp->getOpposite() != NULL) {
					if (e_temp->getCrease() > 0) {
						v2_around_crease_edges.push_back(e_temp);
					}
					e_temp = e_temp->getOpposite()->getNext();
				}
				if (e_temp->getCrease() > 0) {
					v2_around_crease_edges.push_back(e_temp);
				}
				//直到遇到边界边结束
			}
			//v2_around_crease_edges已经存储了v1周围所有的crease边
			Vec3f left;
			Vec3f right;
			if (v1_around_crease_edges.size() == 0) {
				left = v1->getPos()*0.5;
			}
			else {
				/*double left_weight = -0.0625 / v1_around_crease_edges.size();
				for (int i = 0; i < v1_around_crease_edges.size(); i++) {
					left += left_weight * v1_around_crease_edges[i]->getStartVertex()->getPos();
				}*/
				left += -0.0625 * v1_around_crease_edges[0]->getStartVertex()->getPos();
				left += v1->getPos()*0.5625;
			}
			if (v2_around_crease_edges.size() == 0) {
				right = v2->getPos()*0.5;
			}
			else {
				/*double right_weight = -0.0625 / v2_around_crease_edges.size();
				for (int i = 0; i < v2_around_crease_edges.size(); i++) {
					right += right_weight * v2_around_crease_edges[i]->getStartVertex()->getPos();
				}*/
				right += -0.0625 * v2_around_crease_edges[0]->getStartVertex()->getPos();
				right += v1->getPos()*0.5625;
			}
			Vec3f new_vertex_pos = left + right;
			Vertex *new_vertex = addVertex(new_vertex_pos);
			setParentsChild(v1, v2, new_vertex);
		}
		else {
			//e是普通边
			//根据左右顶点的度分为abc三种情况

			//找到v1和v2周围的边
			vector<Edge *> v1_around_edges;
			vector<Edge *> v2_around_edges;
			//找v1周围的边
			Edge *e_temp = e->getNext()->getNext();
			//当遇到边界边或自身时终止
			while (e_temp->getOpposite() != NULL && e_temp->getOpposite() != e) {
				v1_around_edges.push_back(e_temp);
				e_temp = e_temp->getOpposite()->getNext()->getNext();
			}
			if (e_temp->getOpposite() == e) {
				//继续执行
			}
			if (e_temp->getOpposite() == NULL) {
				v1_around_edges.push_back(e_temp);
				//继续找另一边
				e_temp = e->getOpposite()->getNext();
				vector<Edge *> temp_edges;
				while (e_temp->getOpposite() != NULL) {
					temp_edges.push_back(e_temp);
					e_temp = e_temp->getOpposite()->getNext();
				}
				temp_edges.push_back(e_temp);
				//倒序存入v1_around_vertex
				for (int i = temp_edges.size()-1; i>= 0; i--) {
					v1_around_edges.push_back(temp_edges[i]);
				}
				//直到遇到边界边结束
			}
			//v1_around_edges已经存储了v1周围所有的边

			//找v2连接的另外一条边界边
			e = e->getOpposite();
			e_temp = e->getNext()->getNext();
			//当遇到边界边或自身时终止
			while (e_temp->getOpposite() != NULL && e_temp->getOpposite() != e) {
				v2_around_edges.push_back(e_temp);
				e_temp = e_temp->getOpposite()->getNext()->getNext();
			}
			if (e_temp->getOpposite() == e) {
				//继续执行
			}
			if (e_temp->getOpposite() == NULL) {
				v2_around_edges.push_back(e_temp);
				//继续找另一边
				e_temp = e->getOpposite()->getNext();
				vector<Edge *> temp_edges;
				while (e_temp->getOpposite() != NULL) {
					temp_edges.push_back(e_temp);
					e_temp = e_temp->getOpposite()->getNext();
				}
				temp_edges.push_back(e_temp);
				//倒序存入v2_around_vertex
				for (int i = temp_edges.size() - 1; i >= 0; i--) {
					v2_around_edges.push_back(temp_edges[i]);
				}
				//直到遇到边界边结束
			}
			//v2_around_edges已经存储了v2周围所有的边

			Vec3f left;
			Vec3f right;
			Vec3f new_vertex_pos;
			//开始分类
			if (v1_around_edges.size() == 5 && v2_around_edges.size() == 5) {
				//a
				new_vertex_pos += (v1->getPos() + v2->getPos())*0.5;
				new_vertex_pos += (v1_around_edges[0]->getStartVertex()->getPos() + v1_around_edges[4]->getStartVertex()->getPos())*0.125;
				new_vertex_pos += (v1_around_edges[1]->getStartVertex()->getPos() + v1_around_edges[3]->getStartVertex()->getPos())*(-0.0625);
				new_vertex_pos += (v2_around_edges[1]->getStartVertex()->getPos() + v2_around_edges[3]->getStartVertex()->getPos())*(-0.0625);
			}
			else if (v1_around_edges.size() != 5 && v2_around_edges.size() == 5) {
				//b
				new_vertex_pos += v1->getPos()*0.75;
				if (v1_around_edges.size() == 2) {
					//k==3 因为没有算v2
					new_vertex_pos += v2->getPos()*(5.0/12);//5/12
					new_vertex_pos += (v1_around_edges[0]->getStartVertex()->getPos()+ v1_around_edges[1]->getStartVertex()->getPos())*(-1.0/12);
					//new_vertex_pos = 2 * (v1->getPos() + v2->getPos());
				}
				else if (v1_around_edges.size() == 3) {
					//k==4
					new_vertex_pos += v2->getPos()*0.375;//1/8
					new_vertex_pos +=  v1_around_edges[1]->getStartVertex()->getPos()*(-0.125);

				}
				else {
					double k = (v1_around_edges.size()+1);
					new_vertex_pos += v2->getPos()*(1.75 / k);
					for (int i = 0; i < v1_around_edges.size(); i++) {
						int index = i + 1;
						double weight = (0.25 + cos(2 * 3.1415926*index / k) + 0.5*cos(4 * index*3.1415926 / k))/k;
						new_vertex_pos += v1_around_edges[i]->getStartVertex()->getPos()*weight;
					}
				}
				//v1为k-vertices的情况结束
				
			}
			else if (v1_around_edges.size() == 5 && v2_around_edges.size() != 5) {
				//b
				new_vertex_pos += v2->getPos()*0.75;
				if (v2_around_edges.size() == 2) {
					//k==3 因为没有算v1
					new_vertex_pos += v1->getPos()*(5.0/12);//5/12
					new_vertex_pos += (v2_around_edges[0]->getStartVertex()->getPos() + v2_around_edges[1]->getStartVertex()->getPos())*(-1.0 / 12);
				}
				else if (v2_around_edges.size() == 3) {
					//k==4
					new_vertex_pos += v1->getPos()*0.375;//3/8
					new_vertex_pos += v2_around_edges[1]->getStartVertex()->getPos()*(-0.125);//1/8
				}
				else {
					double k = (v2_around_edges.size() + 1);
					new_vertex_pos += v1->getPos()*(1.75 / k);
					for (int i = 0; i < v2_around_edges.size(); i++) {
						int index = i+1;
						double weight = (0.25 + cos(2 * 3.1415926*index / k) + 0.5*cos(4 * index*3.1415926 / k))/k;
						new_vertex_pos += v2_around_edges[i]->getStartVertex()->getPos()*weight;
					}
				}
				
			}
			else{
				//c
				//计算v1的部分
				Vec3f new_vertex_pos_1;
				Vec3f new_vertex_pos_2;
				new_vertex_pos_1 += v1->getPos()*0.75;
				if (v1_around_edges.size() == 2) {
					//k==3 因为没有算v2
					new_vertex_pos_1 += v2->getPos()*(5/12);//5/12
					new_vertex_pos_1 += (v1_around_edges[0]->getStartVertex()->getPos() + v1_around_edges[1]->getStartVertex()->getPos())*(-1/12);
				}
				else if (v1_around_edges.size() == 3) {
					//k==4
					new_vertex_pos_1 += v2->getPos()*0.375;//
					new_vertex_pos_1 += v1_around_edges[1]->getStartVertex()->getPos()*(-0.125); //1 / 8
				}
				else {
					double k = (v1_around_edges.size() + 1);
					new_vertex_pos_1 += v2->getPos()*(1.75 / k);
					for (int i = 0; i < v1_around_edges.size(); i++) {
						int index = i + 1;
						double weight = (0.25 + cos(2 * 3.1415926*index / k) + 0.5*cos(4 * index*3.1415926 / k))/k;
						new_vertex_pos_1 += v1_around_edges[i]->getStartVertex()->getPos()*weight;
					}
				}
				//计算v2的部分
				new_vertex_pos_2 += v2->getPos()*0.75;
				if (v2_around_edges.size() == 2) {
					//k==3 因为没有算v1
					new_vertex_pos_2 += v1->getPos()*(5.0 / 12);//5/12
					new_vertex_pos_2 += (v2_around_edges[0]->getStartVertex()->getPos() + v2_around_edges[1]->getStartVertex()->getPos())*(-1.0 / 12);
				}
				else if (v2_around_edges.size() == 3) {
					//k==4
					new_vertex_pos_2 += v1->getPos()*0.375;//1/8
					new_vertex_pos_2 += v2_around_edges[1]->getStartVertex()->getPos()*(-0.125);
				}
				else {
					double k = (v2_around_edges.size() + 1);
					new_vertex_pos_2 += v1->getPos()*(1.75 / k);
					for (int i = 0; i < v2_around_edges.size(); i++) {
						int index = i + 1;
						double weight = (0.25 + cos(2 * 3.1415926*index / k) + 0.5*cos(4 * index*3.1415926 / k))/k;
						new_vertex_pos_2 += v2_around_edges[i]->getStartVertex()->getPos()*weight;
					}
				}
				//两边取平均
				new_vertex_pos = 0.5*(new_vertex_pos_1+ new_vertex_pos_2);
			}
			Vertex *new_vertex = addVertex(new_vertex_pos);
			setParentsChild(v1, v2, new_vertex);
		}
	}
	

	//添加新三角形，删除旧三角形
	//添加新的小三角形，因为添加的是小三角形，所以可以先添加再删除
	  //新的小三角形中，将原来的crease边给它的子边
	vector<Triangle *> triangle_wait_delete;
	vector<Vertex *> vector_1, vector_2, vector_3;
	int triangles_size_before_add = triangles.size();
	triangleshashtype::iterator iter_add_small_triangle = triangles.begin();
	int count_limit = 0;
	for (; iter_add_small_triangle != triangles.end();) {
		Triangle *now_triangle = iter_add_small_triangle->second;
		//防止将新添加的三角形也放进去
		if (count_limit == triangles_size_before_add) {
			break;
		}
		else {
			count_limit++;
		}

		if (now_triangle == NULL) {
			continue;
		}

		Vertex *va = (*now_triangle)[0];
		Vertex *vb = (*now_triangle)[1];
		Vertex *vc = (*now_triangle)[2];

		Vertex *vab = getChildVertex(va, vb);
		Vertex *vbc = getChildVertex(vb, vc);
		Vertex *vac = getChildVertex(va, vc);



		addTriangle(va, vab, vac);
		addTriangle(vb, vbc, vab);
		addTriangle(vc, vac, vbc);
		addTriangle(vab, vbc, vac);

		//父边如果是crease，子边也应该是crease
		if (getMeshEdge(va, vb)->getCrease() > 0) {
			getMeshEdge(va, vab)->setCrease(getMeshEdge(va, vb)->getCrease());
			getMeshEdge(vab, vb)->setCrease(getMeshEdge(va, vb)->getCrease());
			//应该不用考虑opposite边
			//getMeshEdge(vab, va)->setCrease(getMeshEdge(va, vb)->getCrease());
		}
		if (getMeshEdge(vb, vc)->getCrease() > 0) {
			getMeshEdge(vb, vbc)->setCrease(getMeshEdge(vb, vc)->getCrease());
			getMeshEdge(vbc, vc)->setCrease(getMeshEdge(vb, vc)->getCrease());
			//应该不用考虑opposite边
			//getMeshEdge(vab, va)->setCrease(getMeshEdge(va, vb)->getCrease());
		}
		if (getMeshEdge(vc, va)->getCrease() > 0) {
			getMeshEdge(vc, vac)->setCrease(getMeshEdge(vc, va)->getCrease());
			getMeshEdge(vac, va)->setCrease(getMeshEdge(vc, va)->getCrease());
			//应该不用考虑opposite边
			//getMeshEdge(vab, va)->setCrease(getMeshEdge(va, vb)->getCrease());
		}

		++iter_add_small_triangle;
		//removeTriangle(now_triangle);
		triangle_wait_delete.push_back(now_triangle);
	}

	cout << "add new triangle ok" << endl;
	//删除旧三角形
	vector<Triangle *>::iterator iter_delete_triangle = triangle_wait_delete.begin();
	while (triangle_wait_delete.size() != 0) {
		Triangle *now_delete_triangle = (*iter_delete_triangle);
		iter_delete_triangle = triangle_wait_delete.erase(iter_delete_triangle);
		removeTriangle(now_delete_triangle);
	}
	cout << "delete old triangle ok" << endl;
	cout << "Modified Butterfly Subdivision end" << endl;
}


void Mesh::LoopSubdivision() {
	printf("Subdivide the mesh!\n");

	//得到每个点的类型
	edgeshashtype::iterator iter_vertex_type = edges.begin();
	for (; iter_vertex_type != edges.end(); iter_vertex_type++) {
		Edge *e_vertex_type = iter_vertex_type->second;
		Vertex *v1 = e_vertex_type->getStartVertex();
		//Vertex *v2 = e_vertex_type->getEndVertex();
		if (v1->getS() == 0) {
			v1->setVertexType(smooth);
		}
		else if (v1->getS() == 1) {
			v1->setVertexType(dart);
		}
		else if (v1->getS() == 2) {
			//检查是regular还是non-regular
			//保存周围边
			vector<Edge *> edge_right;
			vector<Edge *> edge_left;
			Edge *e_temp = e_vertex_type->getNext()->getNext();
			edge_right.push_back(e_vertex_type);
			bool have_boundary = false;//找周围边时是否遇到边界
			while (1) {
				if (e_temp->getOpposite() == NULL) {
					have_boundary = true;
					//去找另一个方向
					e_temp = e_vertex_type;
					while (e_temp->getOpposite() != NULL) {
						e_temp = e_temp->getOpposite()->getNext();
						edge_left.push_back(e_temp);
					}
					break;
				}
				else if (e_temp->getOpposite() == e_vertex_type) {
					//找完一圈
					break;
				}
				else if (e_temp->getOpposite() != NULL) {
					edge_right.push_back(e_temp->getOpposite());//存储
					e_temp = e_temp->getOpposite()->getNext()->getNext();
				}
				else {
					cout << "???" << endl;
				}
			}
			//判断顶点类型
			if (have_boundary) {
				//找一圈边的过程中遇到了边界opposite==NULL
				if (edge_right.size() + edge_left.size() == 4) {
					v1->setVertexType(reg_crease);
				}
				else {
					v1->setVertexType(non_reg_crease);
				}
			}
			else {
				//找一圈边的过程中没有遇到边界
				if (edge_right.size() != 6) {
					v1->setVertexType(non_reg_crease);
				}
				else {
					if (edge_right[0]->getCrease() > 0 && edge_right[3]->getCrease() > 0 &&
						edge_right[1]->getCrease() < 0 && edge_right[2]->getCrease() < 0 && edge_right[4]->getCrease() < 0 && edge_right[5]->getCrease() < 0) {
						v1->setVertexType(reg_crease);
					}
					else {
						v1->setVertexType(non_reg_crease);
					}
				}
			}
		}
		else if (v1->getS() >= 3) {
			v1->setVertexType(corner);
		}
		else {
			assert(0);
		}
	}
	//每个顶点的类型都确定了

	//打印所有顶点及其类型
	/*cout << "vertex type" << endl;
	vector<Vertex *>::iterator iter_v = vertices.begin();
	for (; iter_v != vertices.end(); ++iter_v) {
		cout << (*iter_v)->getVertexType() << endl;
	}
	cout << "=========" << endl;*/

	//计算新顶点odd的坐标
	edgeshashtype::iterator iter_edges = edges.begin();
	for (; iter_edges != edges.end(); iter_edges++) {
		Edge *e = iter_edges->second;
		Vertex *v_start = e->getStartVertex();
		Vertex *v_end = e->getEndVertex();
		//计算新坐标
		Vec3f new_vertex_pos;
		if (e->getOpposite() == NULL) {
			//边界边
			new_vertex_pos = (v_start->getPos() + v_end->getPos())*0.5;
			//创建新顶点
			Vertex *new_vertex = addVertex(new_vertex_pos);

			//cout <<"odd 边界" << v_start->getIndex() << " 和 " << v_end->getIndex() << " 得到 " << new_vertex->getIndex() << endl;

			//如果
			if (e->getCrease() > 0) {
				new_vertex->setS(2);//crease边拆分后的点上相连的也只会有2条crease相连
			}
			//设置新顶点为child
			setParentsChild(v_start, v_end, new_vertex);
		}
		else {
			//一般边
			//找另2个顶点
			Triangle *triangle1 = e->getTriangle();
			Triangle *triangle2 = e->getOpposite()->getTriangle();
			Vertex *v3, *v4;
			//找出三角形1中不在边上的顶点
			if ((*triangle1)[0]->getIndex() != e->getStartVertex()->getIndex() && (*triangle1)[0]->getIndex() != e->getEndVertex()->getIndex()) {
				v3 = (*triangle1)[0];
			}
			else if ((*triangle1)[1]->getIndex() != e->getStartVertex()->getIndex() && (*triangle1)[1]->getIndex() != e->getEndVertex()->getIndex()) {
				v3 = (*triangle1)[1];
			}
			else if ((*triangle1)[2]->getIndex() != e->getStartVertex()->getIndex() && (*triangle1)[2]->getIndex() != e->getEndVertex()->getIndex()) {
				v3 = (*triangle1)[2];
			}
			else {
				assert(0);
				v3 = v_start;
			}
			//找出三角形2中不在边上的顶点
			if ((*triangle2)[0] != e->getOpposite()->getStartVertex() && (*triangle2)[0] != e->getOpposite()->getEndVertex()) {
				v4 = (*triangle2)[0];
			}
			else if ((*triangle2)[1] != e->getOpposite()->getStartVertex() && (*triangle2)[1] != e->getOpposite()->getEndVertex()) {
				v4 = (*triangle2)[1];
			}
			else if ((*triangle2)[2] != e->getOpposite()->getStartVertex() && (*triangle2)[2] != e->getOpposite()->getEndVertex()) {
				v4 = (*triangle2)[2];
			}
			else {
				assert(0);
				v4 = v_start;
			}
			//根据两点的类型来决定
			//操作 3
			if (e->getCrease() > 0) {
				if ((v_start->getVertexType() == reg_crease && v_end->getVertexType() == non_reg_crease)
					|| (v_start->getVertexType() == reg_crease && v_end->getVertexType() == corner)
					) {
					//v_start是regular
					new_vertex_pos = 0.625*v_start->getPos() + 0.375*v_end->getPos();

				}
				else if ((v_end->getVertexType() == reg_crease && v_start->getVertexType() == non_reg_crease)
					|| (v_end->getVertexType() == reg_crease && v_start->getVertexType() == corner)) {
					//v_end是regular
					new_vertex_pos = 0.625*v_end->getPos() + 0.375*v_start->getPos();
				}
				else if ((v_end->getVertexType() == reg_crease && v_start->getVertexType() == reg_crease)
					|| (v_end->getVertexType() == non_reg_crease && v_start->getVertexType() == non_reg_crease)
					|| (v_end->getVertexType() == corner && v_start->getVertexType() == corner)
					|| (v_end->getVertexType() == non_reg_crease && v_start->getVertexType() == corner)
					|| (v_end->getVertexType() == corner && v_start->getVertexType() == non_reg_crease)
					) {
					//操作2
					new_vertex_pos = 0.5*v_end->getPos() + 0.5*v_start->getPos();
				}
				else {
					//操作1
					//含dart边
					new_vertex_pos = 0.375*v_start->getPos() + 0.375*v_end->getPos() + 0.125*v3->getPos() + 0.125*v4->getPos();
				}
			}
			else {
				//普通边
				new_vertex_pos = 0.375*v_start->getPos() + 0.375*v_end->getPos() + 0.125*v3->getPos() + 0.125*v4->getPos();
			}

			//创建新顶点
			Vertex *new_vertex = addVertex(new_vertex_pos);
			//如果
			if (e->getCrease() > 0) {
				new_vertex->setS(2);//crease边拆分后的点上相连的也只会有2条crease相连
			}
			//设置新顶点为child
			setParentsChild(v_start, v_end, new_vertex);
			//cout << "parent :" << v_start->getIndex() << " and " << v_end->getIndex() <<" get "<<new_vertex->getIndex()<< endl;//test
		}
	}//计算新odd结束

	//更新原有点even的坐标
	//更新even的坐标
	//存入已经更新的点的指针，如果已经更新过，则不再更新（否则会重复更新多次）
	vector<Vertex *> vertex_updated;
	  edgeshashtype::iterator iter_update_even = edges.begin();
	  for (; iter_update_even != edges.end(); ++iter_update_even) {
		  //遍历edges
		  Edge *e_update_even = iter_update_even->second;
		  Vertex *v1 = e_update_even->getStartVertex();
		  Vertex *v2 = e_update_even->getEndVertex();

		  //检查是否已经更新过
		  bool updated = false;
		  for (int i = 0; i < vertex_updated.size(); i++) {
			  if (v1->getIndex() == vertex_updated[i]->getIndex()) {
				  updated = true;
				  break;
			  }
		  }
		  if (updated) { 
			  continue; 
		  }
		  //找到v1周围的顶点
		  vector<Vertex *> vertex_around;
		  Edge *e_temp = e_update_even->getNext()->getNext();
		  Vertex *boundary_vertex_1 = v1;
		  Vertex *boundary_vertex_2 = v1;
		  bool have_boundary = false;
		  while (1) {
			  //遇到了boundary
			  if (e_temp->getOpposite() == NULL) {
				  boundary_vertex_1 = e_temp->getStartVertex();
				  have_boundary = true;
				  //去找另一个方向
				  e_temp = e_update_even;
				  while (e_temp->getOpposite() != NULL) {
					  e_temp = e_temp->getOpposite()->getNext();
				  }
				  boundary_vertex_2 = e_temp->getEndVertex();
				  break;
			  }
			  else {
				  //正常地找到周围的边
				  if (e_temp->getOpposite() == e_update_even) {
					  //找完一圈
					  vertex_around.push_back(e_temp->getStartVertex());
					  break;
				  }
				  else {
					  vertex_around.push_back(e_temp->getStartVertex());
					  e_temp = e_temp->getOpposite()->getNext()->getNext();
				  }

			  }
		  }
		  //寻找周围顶点结束
		  //计算新坐标
		  if (have_boundary) {
			  //有边界，计算
			  //打印边界边的两个顶点
			  //cout << v1->getIndex()<<" 周围的两个边界点：" << boundary_vertex_1->getIndex() << " and " << boundary_vertex_2->getIndex() << endl;
			  Vec3f new_pos = 0.125*(boundary_vertex_1->getPos() + boundary_vertex_2->getPos()) + 0.75*v1->getPos();
			  //v1->setPos(new_pos);
			  v1->setNextPosition(new_pos);
			  vertex_updated.push_back(v1);//将已经update过的点添加到vertex_updated中
		  }
		  else {
			  //周围没有边界边，根据v1是什么类型的点，计算新位置
			  if (v1->getVertexType() == corner) {
				  //不改变
				  v1->setNextPosition(v1->getPos());
			  }
			  else if (v1->getVertexType() == reg_crease || v1->getVertexType() == non_reg_crease) {
				  //找到两条crease边
				  Vertex *crease_edge_vertex_1 = v1;
				  Vertex *crease_edge_vertex_2 = v1;
				  vector<Vertex *>::iterator iter = vertex_around.begin();
				  bool find_first = false;
				  for (; iter != vertex_around.end(); iter++) {
					  if (edges.find(make_pair(v1, (*iter)))->second->getCrease() > 0&&!find_first) {
						  crease_edge_vertex_1 = (*iter);
						  find_first = true;
					  }
					  else if (edges.find(make_pair(v1, (*iter)))->second->getCrease() > 0 && find_first) {
						  crease_edge_vertex_2 = (*iter);
					  }
				  }
				  //计算新坐标
				  Vec3f new_pos = 0.125*(crease_edge_vertex_1->getPos() + crease_edge_vertex_2->getPos()) + 0.75*v1->getPos();
				  //v1->setPos(new_pos);
				  v1->setNextPosition(new_pos);
			  }
			  else {
				  
				  //一般情况
				  int n = vertex_around.size();
				  
				  //cout << "周围点的数量：" << n << endl;
				  double beta = ( 0.625 - pow(( 0.375+0.25*cos(2*3.1415926/n) ), 2) )/ n;

				  Vec3f new_pos = v1->getPos()*(1-n* beta);
				  vector<Vertex *>::iterator iter_add_around = vertex_around.begin();
				  for (; iter_add_around != vertex_around.end(); ++iter_add_around) {
					  //权重是多少
					  //new_pos += getChildVertex(v1, (*iter_add_around))->getPos()*beta;
					  new_pos += (*iter_add_around)->getPos()*beta;
				  }
				  //v1->setPos(new_pos);
				  v1->setNextPosition(new_pos);
			  }
			  vertex_updated.push_back(v1);//将已经update过的点添加到vertex_updated中
		  }
		  
	  }
	  for (int i = 0; i < vertex_updated.size(); i++) {
		  vertex_updated[i]->UpdatePos();
	  }
	  cout << "update even ok" << endl;
	  //更新even结束

	  
	  //添加新的小三角形，因为添加的是小三角形，所以可以先添加再删除
	  //新的小三角形中，将原来的crease边给它的子边
	    vector<Triangle *> triangle_wait_delete;
		vector<Vertex *> vector_1, vector_2, vector_3;
		int triangles_size_before_add = triangles.size();
		triangleshashtype::iterator iter_add_small_triangle = triangles.begin();
		int count_limit = 0;
		for(; iter_add_small_triangle!=triangles.end();){
			Triangle *now_triangle = iter_add_small_triangle->second;
			//防止将新添加的三角形也放进去
			if (count_limit == triangles_size_before_add) {
				break;
			}
			else {
				count_limit++;
			}

			if (now_triangle == NULL) { 
				continue; 
			}

			Vertex *va = (*now_triangle)[0];
			Vertex *vb = (*now_triangle)[1];
			Vertex *vc = (*now_triangle)[2];

			Vertex *vab = getChildVertex(va, vb);
			Vertex *vbc = getChildVertex(vb, vc);
			Vertex *vac = getChildVertex(va, vc);

			
			
			addTriangle(va, vab , vac);
			addTriangle(vb, vbc, vab);
			addTriangle(vc, vac, vbc);
			addTriangle(vab, vbc, vac);

			//父边如果是crease，子边也应该是crease
			if (getMeshEdge(va, vb)->getCrease() > 0) {
				getMeshEdge(va, vab)->setCrease(getMeshEdge(va, vb)->getCrease());
				getMeshEdge(vab, vb)->setCrease(getMeshEdge(va, vb)->getCrease());
				//应该不用考虑opposite边
				//getMeshEdge(vab, va)->setCrease(getMeshEdge(va, vb)->getCrease());
			}
			if (getMeshEdge(vb, vc)->getCrease() > 0) {
				getMeshEdge(vb, vbc)->setCrease(getMeshEdge(vb, vc)->getCrease());
				getMeshEdge(vbc, vc)->setCrease(getMeshEdge(vb, vc)->getCrease());
				//应该不用考虑opposite边
				//getMeshEdge(vab, va)->setCrease(getMeshEdge(va, vb)->getCrease());
			}
			if (getMeshEdge(vc, va)->getCrease() > 0) {
				getMeshEdge(vc, vac)->setCrease(getMeshEdge(vc, va)->getCrease());
				getMeshEdge(vac, va)->setCrease(getMeshEdge(vc, va)->getCrease());
				//应该不用考虑opposite边
				//getMeshEdge(vab, va)->setCrease(getMeshEdge(va, vb)->getCrease());
			}

			++iter_add_small_triangle;
			//removeTriangle(now_triangle);
			triangle_wait_delete.push_back(now_triangle);
		}

		cout << "add new triangle ok" << endl;
		//删除旧三角形
		vector<Triangle *>::iterator iter_delete_triangle = triangle_wait_delete.begin();
		while(triangle_wait_delete.size()!=0){
			Triangle *now_delete_triangle = (*iter_delete_triangle);
			iter_delete_triangle = triangle_wait_delete.erase(iter_delete_triangle);
			removeTriangle(now_delete_triangle);
		}
		cout << "delete old triangle ok" << endl;
}
// =================================================================
// SIMPLIFICATION RANDOM
// =================================================================

void Mesh::Simplification(int target_tri_count) {
  // clear out any previous relationships between vertices
  vertex_parents.clear();

  printf ("Simplify the mesh! %d -> %d\n", numTriangles(), target_tri_count);

  // =====================================
  // ASSIGNMENT: complete this functionality
  // =====================================
  int rechoose = 0;//连续重新选择限制，如果连续20次都重新选择，那么退出简化过程，防止没有可以删除的边陷入死循环
  int rechoose_limit = 20;
  MTRand rand;
  Triangle *triangle1;
  Triangle *triangle2;

  while(numTriangles()>target_tri_count){//当达到目标后结束循环
	  if (rechoose == rechoose_limit) break;
	  //随机挑一个三角形，然后三角形里随机挑一条边[0,numTriangles() - 1]

	  int tri_id;
	  tri_id= rand.randInt(numTriangles() - 1);//随机挑选一个三角形
	  triangleshashtype::iterator iter = triangles.begin();
	  for (int i = 0; i < tri_id; i++) {
		  if (iter != triangles.end()) {
			  iter++;
		  }
		  else {
			  cout << "【Error】：triangle num to delete out of boundary" << endl;
		  }
	  }
	  //需删除的三角形
	  triangle1 = iter->second;
	  //要删除的边
	  Edge *e1;
	  //随机挑一条边[0,2]
	  int edge_index = rand.randInt(2);
	  e1 = triangle1->getEdge();//要删除的边
	  for (int i = 0; i < edge_index; i++) {
		  e1 = e1->getNext();
	  }

	  //存储所有需要修改的三角形
	  vector<Triangle *> triangles_need_change;

	  //判断是否有opposite
	  if (e1->getOpposite() != NULL) {
		  triangle1 = e1->getTriangle();
		  triangle2 = e1->getOpposite()->getTriangle();
		  //cout << "two triangle to delete " << endl;
		  //cout << triangle1->getID() << " and " << triangle2->getID() << endl;
		  //可以删除，使用half edge来寻找周围需要修改的三角形
		  Edge* temp_edge = e1->getNext()->getOpposite();
		  while (temp_edge != NULL && temp_edge !=e1) {
			  //当前边所在三角形存入vector
			  triangles_need_change.push_back(temp_edge->getTriangle());
			  temp_edge = temp_edge->getNext()->getOpposite();
		  }
		  if (temp_edge != NULL) {
			  if (temp_edge->getTriangle()->getID() == e1->getTriangle()->getID() || temp_edge->getTriangle()->getID() == e1->getOpposite()->getTriangle()->getID()) {
				  triangles_need_change.pop_back();//最后一个是Triangle2
			  }
		  }
		  //cout << "half circle end" << endl;
		  //找另一边的三角形
		  temp_edge = e1->getOpposite()->getNext()->getOpposite();
		  while (temp_edge != NULL && temp_edge != e1->getOpposite()) {
			  //当前边所在三角形存入vector
			  triangles_need_change.push_back(temp_edge->getTriangle());
			  temp_edge = temp_edge->getNext()->getOpposite();
		  }
		  if (temp_edge != NULL) {
			  if (temp_edge->getTriangle()->getID() == e1->getTriangle()->getID() || temp_edge->getTriangle()->getID() == e1->getOpposite()->getTriangle()->getID()) {
				  triangles_need_change.pop_back();//最后一个是Triangle1
			  }
		  }
		  //cout << "another half circle end" << endl;
		  //取v1和v2的平均值，计算新顶点new_vec
		  Vertex *v1 = e1->getStartVertex();
		  Vertex *v2 = e1->getEndVertex();
		  Vec3f new_vec = (v1->getPos() + v2->getPos()) * 0.5;
		  Vertex *v_new = addVertex(new_vec);//新顶点
		  
		  //检查环是否唯一
		  vector<Vertex *> vertex_on_hoop;
		  temp_edge = e1->getNext();
		  Vertex* temp_vertex = temp_edge->getEndVertex();
		  
		  while (1) {
			  if (temp_edge->getOpposite() == NULL) {
				  vertex_on_hoop.push_back(temp_vertex);
				  break;
			  }
			  else if (temp_vertex == e1->getOpposite()->getNext()->getEndVertex()) {
				  //终止，走完半圈
				  break;
			  }
			  else {
				  vertex_on_hoop.push_back(temp_vertex);
				  temp_edge = temp_edge->getOpposite()->getNext();
				  temp_vertex = temp_edge->getEndVertex();
			  }
		  }
		  //另一半
		  temp_edge = e1->getOpposite()->getNext();
		   temp_vertex = temp_edge->getEndVertex();
		  while (1) {
			  if (temp_edge->getOpposite() == NULL) {
				  vertex_on_hoop.push_back(temp_vertex);
				  break;
			  }
			  else if (temp_vertex == e1->getNext()->getEndVertex()) {
				  //终止，走完半圈
				  break;
			  }
			  else {
				  vertex_on_hoop.push_back(temp_vertex);
				  temp_edge = temp_edge->getOpposite()->getNext();
				  temp_vertex = temp_edge->getEndVertex();
			  }
		  }
		  //打印环上所有点的index
		  /*cout << "hoop:" << endl;
		  for (int i = 0; i < vertex_on_hoop.size(); i++) {
			  cout << vertex_on_hoop[i]->getIndex() << endl;
		  }
		  cout << "end" << endl;*/
		  //检查环
		  vector<Vertex *>::iterator iter_hoop1 = vertex_on_hoop.begin();
		  vector<Vertex *>::iterator iter_hoop2;
		  bool hoop_only = true;
		  for (; iter_hoop1 < vertex_on_hoop.end(); iter_hoop1++) {
			  int count = 0;
			  //cout << "iter_hoop1 = " << (*iter_hoop1)->getIndex() << endl;
			  for (iter_hoop2 = vertex_on_hoop.begin(); iter_hoop2 < vertex_on_hoop.end(); iter_hoop2++) {
				  if ((*iter_hoop1)->getIndex() == (*iter_hoop2)->getIndex()) {
					  //cout << "    iter_hoop2 = " << (*iter_hoop2)->getIndex() << endl;
					  count++;
				  }
			  }
			  if (count > 1) hoop_only = false;
		  }
		  if (!hoop_only) {
			  //cout << "环不唯一，重新选择" << endl;
			  rechoose++;
			  continue;
		  }
		  else {
			  //cout << "环唯一,继续" << endl;
		  }

		  //检查更新后的面与之前的夹角
		  bool face_angle_check_result = true;
		  vector<Triangle *>::iterator iter_check_angle = triangles_need_change.begin();
		  for (; iter_check_angle != triangles_need_change.end(); iter_check_angle++) {
			  Triangle* temp_triangle = (*iter_check_angle);
			  Vertex *va = (*temp_triangle)[0];
			  Vertex *vb = (*temp_triangle)[1];
			  Vertex *vc = (*temp_triangle)[2];
			  Vertex *old_vertex1;
			  Vertex *old_vertex2;
			  Vertex *change_vertex;
			  //寻找三角形中两个不改变的点
			  if (va->getIndex() == v1->getIndex() || va->getIndex() == v2->getIndex()) {
				  old_vertex1 = vb;
				  old_vertex2 = vc;
				  if (va->getIndex() == v1->getIndex()) {
					  change_vertex = v1;
				  }
				  else {
					  change_vertex = v2;
				  }
				  
			  }
			  else if (vb->getIndex() == v1->getIndex() || vb->getIndex() == v2->getIndex()) {
				  old_vertex1 = va;
				  old_vertex2 = vc;
				  if (vb->getIndex() == v1->getIndex()) {
					  change_vertex = v1;
				  }
				  else {
					  change_vertex = v2;
				  }
			  }
			  else if (vc->getIndex() == v1->getIndex() || vc->getIndex() == v2->getIndex()) {
				  old_vertex1 = va;
				  old_vertex2 = vb;
				  if (vc->getIndex() == v1->getIndex()) {
					  change_vertex = v1;
				  }
				  else {
					  change_vertex = v2;
				  }
			  }
			  else {
				  old_vertex1 = va;
				  old_vertex2 = vb;
				  change_vertex = v1;
				  cout << "error" << endl;
				  system("pause");
			  }
			  //计算旧平面法向
			  Vec3f vec1_before = old_vertex1->getPos() - change_vertex->getPos();
			  Vec3f vec2_before = old_vertex2->getPos() - change_vertex->getPos();
			  Vec3f vec1_after = old_vertex1->getPos() - v_new->getPos();
			  Vec3f vec2_after = old_vertex2->getPos() - v_new->getPos();
			  Vec3f norm_before;//旧平面的法向
			  Vec3f norm_after;//新平面的法向
			  Vec3f::Cross3(norm_before, vec1_before, vec2_before);
			  Vec3f::Cross3(norm_after, vec1_after, vec2_after);
			  norm_before.Normalize();//归一化
			  norm_after.Normalize();
			  float angle = acos(norm_before.Dot3(norm_after));
			  if (angle > 3.1415926 / 2) {
				  //夹角大于90度，取消删除该边
				  face_angle_check_result = false;
				  break;
			  }
		  }
		  //如果检查有夹角大于90度的，重新选择一条边删除
		  if (!face_angle_check_result) {
			  rechoose++;
			  continue;
		  }
		  
		  //开始删除添加三角形
		  //删除e1和e1->opposite()分别所在的两个三角形
		  removeTriangle(triangle1);
		  removeTriangle(triangle2);
		  //先删除再添加
		  vector<Triangle *>::iterator iter_tri = triangles_need_change.begin();
		  for (; iter_tri != triangles_need_change.end(); iter_tri++) {
			  Triangle* temp_triangle = (*iter_tri);
			  //cout << temp_triangle->getID() << endl;//打印被删除的三角形的id
			  Vertex *va = (*temp_triangle)[0];
			  Vertex *vb = (*temp_triangle)[1];
			  Vertex *vc = (*temp_triangle)[2];
			  //删除
			  removeTriangle(temp_triangle);
			  //添加
			  if (va->getIndex() == v1->getIndex() || va->getIndex() == v2->getIndex()) {
				  addTriangle(v_new, vb, vc);
			  }else if (vb->getIndex() == v1->getIndex() || vb->getIndex() == v2->getIndex()) {
				  addTriangle(va, v_new, vc);
			  }else if (vc->getIndex() == v1->getIndex() || vc->getIndex() == v2->getIndex()) { 
				  addTriangle(va, vb, v_new);
			  }
		  }
		  //cout << "successful delete one edge" << endl;
		  //成功删除一条边,连续重新选择置0
		  rechoose = 0;
		  
	  }
	  else {//没有opposite边
		  //该边不能删除，重新选择
		  rechoose++;
		  //cout << "cant delete edge choose another" << endl;
		  continue;
	  }
  }
  cout << "Simplification end" << endl;
}


// =================================================================


// =================================================================
// SIMPLIFICATION WITH QEM
// =================================================================
#define MaxCost 100000
//计算所有边的cost
void Mesh::calculate_edge_cost() {
	//遍历edges
	int num_null_opposite = 0;
	int num_boundary_nearby = 0;
	int num_normal = 0;
	int inverse_num = 0;
	int uninverse_num = 0;
	for (edgeshashtype::const_iterator iter = edges.begin(); iter != edges.end(); iter++) {
		Edge* now_edge = iter->second;
		if (now_edge->getOpposite() == NULL) {
			//如果一条边没有opposite，它不会被删除，也不需要计算cost
			now_edge->getEndVertex()->setCost(MaxCost + 1);
			now_edge->getStartVertex()->setCost(MaxCost + 1);
			num_null_opposite++;
			//cout << "Vertex " << now_edge->getStartVertex()->getIndex() << "  :  MaxCost+1" << endl;
			//cout << "Vertex " << now_edge->getEndVertex()->getIndex() << "  :  MaxCost+1" << endl;
			continue;
		}
		Vertex* v1 = now_edge->getEndVertex();
		Vertex* v2 = now_edge->getStartVertex();
		//计算v1到周围平面距离的平方和
		//找周围的三角形
		double cost_v1 = 0.0;
		double cost_v2 = 0.0;
		double edge_cost = 0.0;
		Matrix Q1, Q2, Q;
		vector<Triangle *> triangles_around_v1;
		triangles_around_v1.push_back(now_edge->getTriangle());//先添加now_edge所在的三角形
		Edge* temp_edge = now_edge->getNext()->getOpposite();
		//寻找v1周围平面并计算v1的cost
		while (1) {
			if (temp_edge == NULL) {
				cost_v1 = MaxCost + 1;
				break;
			}
			else if (temp_edge == now_edge) {
				//计算一圈已经能结束
				//计算到各个顶点的距离平方和
				vector<Triangle *>::iterator iter_triangle = triangles_around_v1.begin();
				for (; iter_triangle != triangles_around_v1.end(); iter_triangle++) {
					//得到三角形的三个点
					Vertex* va = (*iter_triangle)->operator[](0);
					Vertex* vb = (*iter_triangle)->operator[](1);
					Vertex* vc = (*iter_triangle)->operator[](2);
					//计算法向
					Vec3f vec1 = vb->getPos() - va->getPos();
					Vec3f vec2 = vc->getPos() - va->getPos();
					Vec3f norm;
					Vec3f::Cross3(norm,vec1,vec2);
					norm.Normalize();
					//计算d
					float d = -norm.Dot3(va->getPos());
					//得到p
					Vec4f p(norm.x(), norm.y(), norm.z(), d);
					//计算Q1
					Matrix matrix_p, matrix_pT;
					matrix_p.clear();
					matrix_pT.clear();
					matrix_p.set(0, 0, p.x());
					matrix_p.set(1, 0, p.y());
					matrix_p.set(2, 0, p.z());
					matrix_p.set(3, 0, p.w());
					matrix_p.Transpose(matrix_pT);
					Q1 += matrix_p * matrix_pT;

					//齐次坐标
					Vec4f v(v1->getPos().x(), v1->getPos().y(), v1->getPos().z(), 1);
					cost_v1 += pow(p.Dot4(v),2);//计算cost
					
				}
				break;
			}
			else {
				triangles_around_v1.push_back(temp_edge->getTriangle());
				temp_edge = temp_edge->getNext()->getOpposite();
			}
		}
		//寻找v2周围平面并计算v2的cost
		vector<Triangle *> triangles_around_v2;
		triangles_around_v2.push_back(now_edge->getOpposite()->getTriangle());//先添加edge->opposite()所在三角形
		 temp_edge = now_edge->getOpposite()->getNext()->getOpposite();
		while (1) {
			if (temp_edge == NULL) {
				cost_v2 = MaxCost + 1;
				break;
			}
			else if (temp_edge == now_edge->getOpposite()) {
				//一圈已经能结束
				//计算到各个顶点的距离平方和
				vector<Triangle *>::iterator iter_triangle = triangles_around_v2.begin();
				for (; iter_triangle != triangles_around_v2.end(); iter_triangle++) {
					//得到三角形的三个点
					Vertex* va = (*iter_triangle)->operator[](0);
					Vertex* vb = (*iter_triangle)->operator[](1);
					Vertex* vc = (*iter_triangle)->operator[](2);
					//计算法向
					Vec3f vec1 = vb->getPos() - va->getPos();
					Vec3f vec2 = vc->getPos() - va->getPos();
					Vec3f norm;
					Vec3f::Cross3(norm, vec1, vec2);
					norm.Normalize();
					//计算d
					float d = -norm.Dot3(v2->getPos());
					//得到p
					Vec4f p(norm.x(), norm.y(), norm.z(), d);
					//计算Q1
					Matrix matrix_p, matrix_pT;
					matrix_p.clear();
					matrix_pT.clear();
					matrix_p.set(0, 0, p.x());
					matrix_p.set(1, 0, p.y());
					matrix_p.set(2, 0, p.z());
					matrix_p.set(3, 0, p.w());
					matrix_p.Transpose(matrix_pT);
					Q2 += matrix_p * matrix_pT;
					//齐次坐标
					Vec4f v(v2->getPos().x(), v2->getPos().y(), v2->getPos().z(), 1);
					cost_v2 += pow(p.Dot4(v), 2);//计算cost
				}
				break;
			}
			else {
				triangles_around_v2.push_back(temp_edge->getTriangle());
				temp_edge = temp_edge->getNext()->getOpposite();
			}
		}
		if (cost_v1 > MaxCost || cost_v2 > MaxCost) {
			num_boundary_nearby++;
			v1->setCost(cost_v1);
			v2->setCost(cost_v2);
			cost_heap.push(now_edge, MaxCost+1);
			continue;
		}
		
		//计算边的Q矩阵
		Q = Q1 + Q2;
		//计算边的cost
		Matrix Q_inverse;
		Q.set(3, 0, 0);
		Q.set(3, 1, 0);
		Q.set(3, 2, 0);
		Q.set(3, 3, 1);
		int inverse_able = Q.Inverse(Q_inverse);
		if (inverse_able == 1) {
			//可逆
			Vec4f result(0, 0, 0, 1);
			Q_inverse.Transform(result);
			double cost = result.Dot4((Q1+Q2)*result);
			//添加到小顶堆中
			cost_heap.push(now_edge, cost);
			inverse_num++;
			
			
		}
		else {
			//不可逆，从v1v2上找一个cost最小的点
			int step = 10;
			double mincost = MaxCost+1;
			for (int i = 0; i <= 10; i++) {
				Vec3f new_vec = (i / step)*v1->getPos() + ((step - i) / step)*v2->getPos();
				Vec4f new_vec_homo(new_vec.x(), new_vec.y(), new_vec.z(), 1);
				double now_cost = new_vec_homo.Dot4((Q1 + Q2)*new_vec_homo);
				if (now_cost < mincost) mincost = now_cost;
			}
			cost_heap.push(now_edge, mincost);
			uninverse_num++;
		}
		//存储v1和v2的矩阵Q
		v1->setCost(cost_v1);
		v2->setCost(cost_v2);
		v1->setQ(Q1);
		v2->setQ(Q2);
		
		num_normal++;
	}
	//cout << "calculate edges cost" << endl;
	//结束，所有的edge已经有了cost
	/*cout << "边界边" << num_null_opposite << endl;
	cout << "一个顶点连接边界边" << num_boundary_nearby << endl;
	cout << "正常边" << num_normal << endl;
	cout << "cost_heap.size()" << cost_heap.getsize() << endl;
	cout << "可逆：" << inverse_num << endl;
	cout << "不可逆：" << uninverse_num << endl;*/
}

//计算一个顶点的Q，需要一条该点连接的边做辅助
void calculate_one_vertex_cost(Vertex *v, Edge *e) {
	
	//计算v1到周围平面距离的平方和
	//找周围的三角形
	double cost_v = 10;
	Matrix Q1, Q2, Q;
	Edge* now_edge = e;
	
	//如果是边界边
	if (e->getOpposite() == NULL) {
		cost_v = MaxCost + 1;
		v->setCost(cost_v);
		v->setQ(Q1);
		return;
	}
	//点v是边的endVertex时比较容易处理
	if (v->getIndex() == e->getEndVertex()->getIndex()) {
		//ok
	}
	else if(v->getIndex() == e->getStartVertex()->getIndex()){
		//换opposite
		now_edge = e->getOpposite();
	}
	else {
		cout << "Error: vertex is not on edge" << endl;
		system("pause");
	}

	vector<Triangle *> triangles_around_v;
	triangles_around_v.push_back(now_edge->getTriangle());//先添加now_edge所在的三角形
	Edge* temp_edge = now_edge->getNext()->getOpposite();
	//寻找v1周围平面并计算v1的cost
	while (1) {
		if (temp_edge == NULL) {
			cost_v = MaxCost + 1;
			break;
		}
		else if (temp_edge == now_edge) {
			//计算一圈已经能结束
			//计算到各个顶点的距离平方和
			vector<Triangle *>::iterator iter_triangle = triangles_around_v.begin();
			for (; iter_triangle != triangles_around_v.end(); iter_triangle++) {
				//得到三角形的三个点
				Vertex* va = (*iter_triangle)->operator[](0);
				Vertex* vb = (*iter_triangle)->operator[](1);
				Vertex* vc = (*iter_triangle)->operator[](2);
				//计算法向
				Vec3f vec1 = vb->getPos() - va->getPos();
				Vec3f vec2 = vc->getPos() - va->getPos();
				Vec3f norm;
				Vec3f::Cross3(norm, vec1, vec2);
				norm.Normalize();
				//计算d
				float d = -norm.Dot3(v->getPos());
				//得到p
				Vec4f p(norm.x(), norm.y(), norm.z(), d);
				//计算Q1
				Matrix matrix_p, matrix_pT;
				matrix_p.clear();
				matrix_pT.clear();
				matrix_p.set(0, 0, p.x());
				matrix_p.set(1, 0, p.y());
				matrix_p.set(2, 0, p.z());
				matrix_p.set(3, 0, p.w());
				matrix_p.Transpose(matrix_pT);
				Q1 += matrix_p * matrix_pT;
				//齐次坐标
				//Vec4f v(v->getPos().x(), v->getPos().y(), v->getPos().z(), 1);
				//cost_v += pow(p.Dot4(v), 2);//计算cost
			}
			break;
		}
		else {
			triangles_around_v.push_back(temp_edge->getTriangle());
			temp_edge = temp_edge->getNext()->getOpposite();
		}
	}

	v->setCost(cost_v);
	v->setQ(Q1);
	return;
}


//QEM里面没有检查夹角是否大于90度
void Mesh::Simplification_QEM(int target_tri_count) {
	// clear out any previous relationships between vertices
	vertex_parents.clear();
	printf("【QEM】Simplify the mesh! %d -> %d\n", numTriangles(), target_tri_count);

	Triangle *triangle1;
	Triangle *triangle2;
	int rechoose = 0;//连续重新选择限制，如果连续20次都重新选择，那么退出简化过程，防止没有可以删除的边陷入死循环
	int rechoose_limit = 20;
	while(numTriangles()>target_tri_count){
		//达到选择次数上限
		if (rechoose == rechoose_limit) break;
		//已经没有可以删除的边
		if (cost_heap.getsize() == 0) break;

		if (cost_heap.get_top()->getCost() > MaxCost) break;
		/*
		cout << "【edges  size】: " << edges.size() << endl;
		cout << "【cost_heap  size】: " << cost_heap.getsize() << endl;*/

		
		//从cost_heap中得到要删除的边
		assert(cost_heap.get_top()->getStartVertex() != NULL && cost_heap.get_top()->getEndVertex() != NULL);
		edgeshashtype::iterator iter_edges= edges.find(std::make_pair(cost_heap.get_top()->getStartVertex(), cost_heap.get_top()->getEndVertex()));
		assert(iter_edges != edges.end());
		Edge *e1 = iter_edges->second;

		//存储所有需要修改的三角形
		vector<Triangle *> triangles_need_change;

		//输出cost
		/*cout << "start cost : " << cost_heap.get_top()->getStartVertex()->getCost() << endl;
		cout << "end cost : " << cost_heap.get_top()->getEndVertex()->getCost() << endl;
		cout<<"edge cost : " << cost_heap.get_top()->getCost() << endl;
		cout << "next edge cost : " << cost_heap.get_top_next()->getCost() << endl;*/

		//判断是否有opposite
		if (e1->getOpposite() != NULL) {
			triangle1 = e1->getTriangle();
			triangle2 = e1->getOpposite()->getTriangle();
			//cout << "two triangle to delete " << endl;
			//cout << triangle1->getID() << " and " << triangle2->getID() << endl;
			//可以删除，使用half edge来寻找周围需要修改的三角形
			Edge* temp_edge = e1->getNext()->getOpposite();
			while (temp_edge != NULL && temp_edge != e1) {
				//当前边所在三角形存入vector
				triangles_need_change.push_back(temp_edge->getTriangle());
				temp_edge = temp_edge->getNext()->getOpposite();
			}
			if (temp_edge->getTriangle()->getID() == e1->getTriangle()->getID() || temp_edge->getTriangle()->getID() == e1->getOpposite()->getTriangle()->getID()) {
				triangles_need_change.pop_back();//最后一个是Triangle2
			}
			//cout << "half circle end" << endl;
			//找另一边的三角形
			temp_edge = e1->getOpposite()->getNext()->getOpposite();
			while (temp_edge != NULL && temp_edge != e1->getOpposite()) {
				//当前边所在三角形存入vector
				triangles_need_change.push_back(temp_edge->getTriangle());
				temp_edge = temp_edge->getNext()->getOpposite();
			}
			if (temp_edge->getTriangle()->getID() == e1->getTriangle()->getID() || temp_edge->getTriangle()->getID() == e1->getOpposite()->getTriangle()->getID()) {
				triangles_need_change.pop_back();//最后一个是Triangle1
			}
			//cout << "another half circle end" << endl;
			//计算新顶点new_vec
			Vertex *v1 = e1->getStartVertex();
			Vertex *v2 = e1->getEndVertex();
			//计算新顶点
			//Vec3f new_vec = (v1->getPos() + v2->getPos()) * 0.5;
			Vec3f new_vec;
			//获得当前边的Q
			Matrix Q = cost_heap.get_top()->getQ();
			Matrix Q_inverse;
			Q.set(3, 0, 0);
			Q.set(3, 1, 0);
			Q.set(3, 2, 0);
			Q.set(3, 3, 1);

			int inverse_able = Q.Inverse(Q_inverse);
			if (inverse_able == 1) {
				//可逆
				Vec4f result(0, 0, 0, 1);
				Q_inverse.Transform(result);
				new_vec.setx(result.x());
				new_vec.sety(result.y());
				new_vec.setz(result.z());
			}
			else {
				//不可逆，从v1v2上找一个cost最小的点
				int step = 10;
				double mincost = MaxCost + 1;
				for (int i = 0; i <= 10; i++) {
					Vec3f new_vec_temp = (i / step)*v1->getPos() + ((step - i) / step)*v2->getPos();
					Vec4f new_vec_homo(new_vec.x(), new_vec.y(), new_vec.z(), 1);
					double now_cost = new_vec_homo.Dot4(cost_heap.get_top()->getQ()*new_vec_homo);
					if (now_cost < mincost) { 
						mincost = now_cost; 
						new_vec = new_vec_temp;
					}
				}
			}


			Vertex *v_new = addVertex(new_vec);//新顶点

			//检查环是否唯一
			vector<Vertex *> vertex_on_hoop;
			temp_edge = e1->getNext();
			Vertex* temp_vertex = temp_edge->getEndVertex();

			while (1) {
				if (temp_edge->getOpposite() == NULL) {
					vertex_on_hoop.push_back(temp_vertex);
					break;
				}
				else if (temp_vertex == e1->getOpposite()->getNext()->getEndVertex()) {
					//终止，走完半圈
					break;
				}
				else {
					vertex_on_hoop.push_back(temp_vertex);
					temp_edge = temp_edge->getOpposite()->getNext();
					temp_vertex = temp_edge->getEndVertex();
				}
			}
			//另一半
			temp_edge = e1->getOpposite()->getNext();
			temp_vertex = temp_edge->getEndVertex();
			while (1) {
				if (temp_edge->getOpposite() == NULL) {
					vertex_on_hoop.push_back(temp_vertex);
					break;
				}
				else if (temp_vertex == e1->getNext()->getEndVertex()) {
					//终止，走完半圈
					break;
				}
				else {
					vertex_on_hoop.push_back(temp_vertex);
					temp_edge = temp_edge->getOpposite()->getNext();
					temp_vertex = temp_edge->getEndVertex();
				}
			}
			//检查环
			vector<Vertex *>::iterator iter_hoop1 = vertex_on_hoop.begin();
			vector<Vertex *>::iterator iter_hoop2;
			bool hoop_only = true;
			for (; iter_hoop1 < vertex_on_hoop.end(); iter_hoop1++) {
				int count = 0;
				for (iter_hoop2 = vertex_on_hoop.begin(); iter_hoop2 < vertex_on_hoop.end(); iter_hoop2++) {
					if ((*iter_hoop1)->getIndex() == (*iter_hoop2)->getIndex()) {
						count++;
					}
				}
				if (count > 1) hoop_only = false;
			}
			if (!hoop_only) {
				//cout << "环不唯一，重新选择" << endl;
				rechoose++;
				cost_heap.push(e1, MaxCost + 1);
				cost_heap.pop();
				continue;
			}
			else {
				//cout << "环唯一,继续" << endl;
			}

			//删除e1和e2分别所在的两个三角形
			removeTriangle(triangle1);
			removeTriangle(triangle2);
			//先删除再添加
			vector<Triangle *>::iterator iter_tri = triangles_need_change.begin();
			for (; iter_tri != triangles_need_change.end(); iter_tri++) {
				Triangle* temp_triangle = (*iter_tri);
				//cout << temp_triangle->getID() << endl;//打印被删除的三角形的id
				Vertex *va = (*temp_triangle)[0];
				Vertex *vb = (*temp_triangle)[1];
				Vertex *vc = (*temp_triangle)[2];
				//删除
				removeTriangle(temp_triangle);
				//添加
				//检查三角形中哪一个点消失了
				if (va->getIndex() == v1->getIndex() || va->getIndex() == v2->getIndex()) {
					va = v_new;
				}
				else if (vb->getIndex() == v1->getIndex() || vb->getIndex() == v2->getIndex()) {
					vb = v_new;
				}
				else if (vc->getIndex() == v1->getIndex() || vc->getIndex() == v2->getIndex()) {
					vc = v_new;
				}
				addTriangle(va, vb, vc);
				//在这里得到around_vertexs，周围一圈的点
			}
			rechoose = 0;
			cost_heap.pop();//成功删除后在小顶堆中也将其删除
			if(cost_heap.getsize()>0) cost_heap.pop();//将其opposite也删除

			

			//找到周围点与新顶点构成的新边，push到cost_heap中去
			//更新new顶点的cost和Q
			edgeshashtype::iterator any_edge_iter = edges.find(std::make_pair(v_new, vertex_on_hoop[0]));
			Edge* temp_e = any_edge_iter->second;
			calculate_one_vertex_cost(v_new, temp_e);//在函数中给点计算并设置了cost和Q
			//更新周围点的cost和Q
			for (vector<Vertex *>::iterator iter_updatev = vertex_on_hoop.begin(); iter_updatev < vertex_on_hoop.end(); iter_updatev++) {
				//更新周围点的cost
				Vertex* around_v = (*iter_updatev);
				any_edge_iter = edges.find(std::make_pair(v_new, around_v));
				temp_e = any_edge_iter->second;
				calculate_one_vertex_cost(around_v, temp_e);//更新cost和Q
				//并向cost_heap中添加新的边
				cost_heap.push(temp_e);

				//处理反向边
				temp_e = temp_e->getOpposite();
				//并向cost_heap中添加新的边
				cost_heap.push(temp_e);

			}
			//必须在更新完顶点的Q之后才能去更新边的cost
			cost_heap.update(this->edges, vertex_on_hoop);//更新cost_heap

		}else {//没有opposite边
			cout << "Opposite == NULL" << endl;
			//该边不能删除，重新选择
			rechoose++;
			cost_heap.push(e1, MaxCost + 1);
			cost_heap.pop();
			//cout << "cant delete edge choose another" << endl;
			continue;
		}
	}
	/*this->clearheap();
	this->calculate_edge_cost();
	this->heapsize();*/
	cout << "Simplification end" << endl;
}


// =================================================================

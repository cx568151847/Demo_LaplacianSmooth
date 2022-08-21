#ifndef _VERTEX_H
#define _VERTEX_H

#include "vectors.h"
#include "matrix.h"
// ==========================================================
// Stores th vertex position, used by the Mesh class

enum VertexType { smooth, dart, reg_crease, non_reg_crease, corner };

class Vertex {

public:

  // ========================
  // CONSTRUCTOR & DESTRUCTOR
  Vertex(int i, const Vec3f &pos) : position(pos) { 
	  index = i; 
	  vertex_type = smooth;
	  this->s = 0;
  }
  
  // =========
  // ACCESSORS
  int getIndex() const { return index; }
  double x() const { return position.x(); }
  double y() const { return position.y(); }
  double z() const { return position.z(); }
  const Vec3f& getPos() const { return position; }

  // =========
  // MODIFIERS
  void setPos(Vec3f v) { position = v; }
  double getCost() {
	  assert(cost != NULL);
	  return this->cost;
  }
  void setCost(double cost) {
	  this->cost = cost;
  }
  Matrix getQ() {
	  return this->Q;
  }
  void setQ(Matrix m) {
	  this->Q = m;
  }

  VertexType getVertexType() {
	  return this->vertex_type;
  }
  void setVertexType(VertexType t) {
	  this->vertex_type = t;
  }
  int getS() {
	  return this->s;
  }
  void setS(int s) {
	  this->s = s;
  }

  //暂存一个pos，用于统一更新
  void setNextPosition(Vec3f v) {
	  this->next_position = v;
  }
  void UpdatePos() {
	  this->setPos(this->next_position);
  }

private:

  // don't use these constructors
  Vertex() { assert(0); exit(0); }
  Vertex(const Vertex&) { assert(0); exit(0); }
  Vertex& operator=(const Vertex&) { assert(0); exit(0); }
  
  // ==============
  // REPRESENTATION
  Vec3f position;

  // this is the index from the original .obj file.
  // technically not part of the half-edge data structure, 
  // but we use it for hashing
  int index;  

  // NOTE: the vertices don't know anything about adjacency.  In some
  // versions of this data structure they have a pointer to one of
  // their incoming edges.  However, this data is very complicated to
  // maintain during mesh manipulation, so it has been omitted.
  double cost;
  Matrix Q;
	
  VertexType vertex_type;
  int s;//crease边的数量

  Vec3f next_position;
};

// ==========================================================

#endif


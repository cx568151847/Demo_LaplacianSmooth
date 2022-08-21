#pragma once
#include <vector>
#include "matrix.h"
#include "triangle.h"
#include "edge.h"
#include "vertex.h"
#include "hash.h"
#include <cmath>

#define MaxCost 100000

class edge_cost_node {
public:
	edge_cost_node(Edge *e);
	edge_cost_node(Edge *e, double cost);
	Vertex* getStartVertex();//��ȡ����
	Vertex* getEndVertex();
	void setEdge(Edge *e);
	double getCost();
	void setCost(double cost);
	Matrix getQ();
	void setQ(Matrix m);
private:
	//�洢һ���ߵ�������
	Vertex *StartVertex;
	Vertex *EndVertex;
	//��ǰ�ߵ�Q
	Matrix Q;
	double cost;//��ǰ�ߵ�cost
};

class minCostHeap {
public:
	minCostHeap();
	
	//��ӣ�����
	void push(Edge *e);
	
	//��ӣ�����
	void push(Edge *e, double cost);
	
	//ɾ���Ѷ�
	void pop();
	
	//ɾ��������ָ��Ľڵ�,������ָ����һ������ĵ�����
	//vector<edge_cost_node *>::iterator erase(std::vector<edge_cost_node *>::iterator iter);
	
	//����heap
	void update(edgeshashtype edges,std::vector<Vertex *> around_vertexs);
	
	//���ضѶ�
	edge_cost_node* get_top();
	
	//�����ã����صڶ�С����
	edge_cost_node* get_top_next();
	
	//��ն�
	void clear();
	
	//��ȡ�ѵĴ�С
	int getsize();
private:
	std::vector<edge_cost_node *> heap;
};
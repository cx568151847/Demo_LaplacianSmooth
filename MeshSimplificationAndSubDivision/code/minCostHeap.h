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
	Vertex* getStartVertex();//获取顶点
	Vertex* getEndVertex();
	void setEdge(Edge *e);
	double getCost();
	void setCost(double cost);
	Matrix getQ();
	void setQ(Matrix m);
private:
	//存储一条边的两顶点
	Vertex *StartVertex;
	Vertex *EndVertex;
	//当前边的Q
	Matrix Q;
	double cost;//当前边的cost
};

class minCostHeap {
public:
	minCostHeap();
	
	//添加，排序
	void push(Edge *e);
	
	//添加，排序
	void push(Edge *e, double cost);
	
	//删除堆顶
	void pop();
	
	//删除迭代器指向的节点,并返回指向下一个对象的迭代器
	//vector<edge_cost_node *>::iterator erase(std::vector<edge_cost_node *>::iterator iter);
	
	//更新heap
	void update(edgeshashtype edges,std::vector<Vertex *> around_vertexs);
	
	//返回堆顶
	edge_cost_node* get_top();
	
	//测试用，返回第二小的项
	edge_cost_node* get_top_next();
	
	//清空堆
	void clear();
	
	//获取堆的大小
	int getsize();
private:
	std::vector<edge_cost_node *> heap;
};
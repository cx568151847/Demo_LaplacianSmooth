#pragma once
#include "minCostHeap.h"
using namespace std;


edge_cost_node::edge_cost_node(Edge *e) {
		assert(e != NULL);
		assert(e->getStartVertex() != NULL && e->getEndVertex() != NULL);
		this->StartVertex = e->getStartVertex();
		this->EndVertex = e->getEndVertex();
		this->Q = e->getStartVertex()->getQ() + e->getEndVertex()->getQ();
		if (e->getStartVertex()->getCost() > MaxCost || e->getEndVertex()->getCost() > MaxCost) {
			this->cost = MaxCost + 1;
		}
		else {
			//使用Q计算cost
			//计算边的cost
			Matrix Q_temp = e->getStartVertex()->getQ() + e->getEndVertex()->getQ();
			Matrix Q_inverse;
			Q_temp.set(3, 0, 0);
			Q_temp.set(3, 1, 0);
			Q_temp.set(3, 2, 0);
			Q_temp.set(3, 3, 1);
			int inverse_able = Q_temp.Inverse(Q_inverse);
			if (inverse_able == 1) {
				//可逆
				Vec4f result(0, 0, 0, 1);
				Q_inverse.Transform(result);
				double now_cost = result.Dot4(this->Q*result);
				this->cost = now_cost;
			}
			else {
				//不可逆，从v1v2上找一个cost最小的点
				int step = 10;
				double mincost = MaxCost + 1;
				for (int i = 0; i <= 10; i++) {
					Vec3f new_vec = (i / step)*this->StartVertex->getPos() + ((step - i) / step)*this->EndVertex->getPos();
					Vec4f new_vec_homo(new_vec.x(), new_vec.y(), new_vec.z(), 1);
					double now_cost = new_vec_homo.Dot4(this->Q*new_vec_homo);
					if (now_cost < mincost) mincost = now_cost;
				}
				this->cost = mincost;
			}
		}
	}

//带cost的构造函数
edge_cost_node::edge_cost_node(Edge *e,double cost) {
	assert(e != NULL);
	assert(e->getStartVertex() != NULL && e->getEndVertex() != NULL);
	this->StartVertex = e->getStartVertex();
	this->EndVertex = e->getEndVertex();
	this->Q = e->getStartVertex()->getQ() + e->getEndVertex()->getQ();
	this->cost = cost;
}
	//更新
	//0 : pop掉该点
	//1 : cost改变
	//2 : cost不变
	//int update() {
	//	if (this->cost > MaxCost) return 2;//不处理这种边

	//	assert(this->e != NULL);
	//	assert(this->e->getStartVertex() != NULL && this->e->getEndVertex() != NULL);

	//	if (this->e == NULL || this->e->getStartVertex() == NULL || this->e->getEndVertex() == NULL) {
	//		//发生了改变
	//		return 0;
	//	}else {
	//		double new_cost = this->e->getStartVertex()->getCost() + this->e->getEndVertex()->getCost();
	//		if (fabs(new_cost-this->cost)<1e-24) {
	//			return 2;
	//		}
	//		this->cost = new_cost;
	//		this->Q = this->e->getStartVertex()->getQ() + this->e->getEndVertex()->getQ();
	//		return 1;
	//	}
	//}
	Vertex* edge_cost_node::getStartVertex() {
		return this->StartVertex;
	}
	Vertex* edge_cost_node::getEndVertex() {
		return this->EndVertex;
	}
	void edge_cost_node::setEdge(Edge *e) {
		assert(e != NULL);
		assert(e->getStartVertex() != NULL && e->getEndVertex() != NULL);
		this->StartVertex = e->getStartVertex();
		this->EndVertex = e->getEndVertex();
	}
	double edge_cost_node::getCost() {
		return this->cost;
	}
	void edge_cost_node::setCost(double cost) {
		this->cost = cost;
	}
	Matrix edge_cost_node::getQ() {
		return this->Q;
	}
	void edge_cost_node::setQ(Matrix m) {
		this->Q = m;
	}


	minCostHeap::minCostHeap() {
	}
	//添加，排序
	void minCostHeap::push(Edge *e) {
		edge_cost_node *node = new edge_cost_node(e);
		if (this->heap.size() == 0) {
			this->heap.push_back(node);
		}
		else {
			//遍历
			vector<edge_cost_node *>::iterator iter = heap.begin();
			for (; iter < heap.end(); iter++) {
				if (node->getCost() < (*iter)->getCost()) {
					heap.insert(iter, node);
					break;
				}
			}
		}
	}
	//【带cost】用于添加因为特殊情况设置一个很大的cost来让以后不再处理该边
	void minCostHeap::push(Edge *e,double cost) {
		edge_cost_node *node = new edge_cost_node(e, cost);
		if (this->heap.size() == 0) {
			this->heap.push_back(node);
		}
		else {
			//已经有其他节点，按从小到大顺序插入
			vector<edge_cost_node *>::iterator iter = heap.begin();
			//疑问，如果是最后一个会怎么样？
			bool inserted = false;
			for (; iter < heap.end(); iter++) {
				if (node->getCost() < (*iter)->getCost()) {
					heap.insert(iter, node);
					inserted = true;
					break;
				}
			}
			if (!inserted) { //如果和最后一个元素相等，或者大于最后一个元素的cost
				heap.push_back(node); 
			}
		}
	}
	//删除堆顶
	void minCostHeap::pop() {
		assert(this->heap.size() > 0);
		heap.erase(heap.begin());
	}

	//vector<Vertex *> around_vertexs,只对有周围点的边做修改
	//更新
	//void minCostHeap::update(edgeshashtype edges, vector<Vertex *> around_vertexs) {
	void minCostHeap::update(edgeshashtype edges, vector<Vertex *> around_vertexs){
		//使用
		//1.找到边消失的node，直接删除
		//2.边没有消失的，更新其cost和Q（erase再重新添加）
		//暂存边
		vector<Edge *> edges_wait_add;

		int change_num = 0;
		int jump_num = 0;


		//遍历heap，检查每一个node
		vector<edge_cost_node *>::iterator iter_heap = this->heap.begin();
		int edge_disapper = 0;
		int edge_cost_change = 0;
		for (; iter_heap != this->heap.end();) {
			//如果任一顶点在around_vertexs中则处理该node
			bool vertex_in_around = false;//该node的点是否在around_vertexs中
			vector<Vertex *>::iterator iter_around_vertexs = around_vertexs.begin();
			for (; iter_around_vertexs != around_vertexs.end(); iter_around_vertexs++) {
				if ((*iter_heap)->getStartVertex()->getIndex() == (*iter_around_vertexs)->getIndex() || (*iter_heap)->getEndVertex()->getIndex() == (*iter_around_vertexs)->getIndex()) {
					vertex_in_around = true;
					break;
				}
			}
			if (!vertex_in_around) {
				iter_heap++;
				jump_num++;
				continue;
			}
			else {
				change_num++;
			}

			edgeshashtype::iterator iter_edges = edges.find(std::make_pair((*iter_heap)->getStartVertex(), (*iter_heap)->getEndVertex()));
			if (iter_edges == edges.end()) {
				//边不存在,删除该node
				iter_heap = this->heap.erase(iter_heap);
				edge_disapper++;//test
			}
			else {
				//使用参数传入的around_vertexs来help update
				//如果node的任意一个vertex在around_vertexs中




				Vertex *v_start = (*iter_heap)->getStartVertex();
				Vertex *v_end = (*iter_heap)->getEndVertex();
				//边存在,重新计算cost，检查cost是否改变
				//重新计算cost
				double old_cost = (*iter_heap)->getCost();
				double new_cost;
				//计算边的Q矩阵
				double cost = 0;
				if (v_start->getCost() > MaxCost || v_end->getCost() > MaxCost|| iter_edges->second->getOpposite()==NULL) {
					new_cost = MaxCost + 1;
				}
				else {
					Matrix Q1 = v_start->getQ();
					Matrix Q2 = v_end->getQ();
					Matrix Q = Q1 + Q2;
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
						new_cost = result.Dot4((Q1 + Q2)*result);
					}
					else {
						//不可逆，从v1v2上找一个cost最小的点
						int step = 10;
						double mincost = MaxCost + 1;
						for (int i = 0; i <= 10; i++) {
							Vec3f new_vec = (i / step)*v_start->getPos() + ((step - i) / step)*v_end->getPos();
							Vec4f new_vec_homo(new_vec.x(), new_vec.y(), new_vec.z(), 1);
							double now_cost = new_vec_homo.Dot4((Q1 + Q2)*new_vec_homo);
							if (now_cost < mincost) mincost = now_cost;
						}
						new_cost = mincost;
					}
				}
				
				if (fabs(new_cost - old_cost) < 1e-6) {
					//没有改变
					iter_heap++;
				}
				else {
					edge_cost_change++;
					iter_heap = this->heap.erase(iter_heap);
					//改变了，删除当前节点，获取边，重新添加
					Edge *e = iter_edges->second;
					//this->push(e);
					edges_wait_add.push_back(e);
				}
			}
		}
		//将e添加
		int real_add = 0;
		vector<Edge *>::iterator iter2addedge = edges_wait_add.begin();
		for (; iter2addedge != edges_wait_add.end(); iter2addedge++) {
			this->push((*iter2addedge));
		}
		/*cout << "around vertex num :" << around_vertexs.size() << endl;
		cout << "change node :" << change_num << endl;
		cout << "jump node :" << jump_num << endl;*/
		/*cout << "update end" << endl;
		cout << "消失的边 : " << edge_disapper << endl;
		cout << "cost改变的边 : " << edge_cost_change << endl;*/
	}
	//返回堆顶
	edge_cost_node* minCostHeap::get_top() {
		assert(this->heap.size() > 0);
		return (*heap.begin());
	}
	//测试用，返回第二小的项
	edge_cost_node* minCostHeap::get_top_next() {
		assert(this->heap.size() > 1);
		return (*(++heap.begin()));
	}
	//清空堆
	void minCostHeap::clear() {
		this->heap.clear();
	}
	//获取堆的大小
	int minCostHeap::getsize() {
		return this->heap.size();
	}
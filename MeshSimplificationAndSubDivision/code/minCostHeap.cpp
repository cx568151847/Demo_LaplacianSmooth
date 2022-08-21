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
			//ʹ��Q����cost
			//����ߵ�cost
			Matrix Q_temp = e->getStartVertex()->getQ() + e->getEndVertex()->getQ();
			Matrix Q_inverse;
			Q_temp.set(3, 0, 0);
			Q_temp.set(3, 1, 0);
			Q_temp.set(3, 2, 0);
			Q_temp.set(3, 3, 1);
			int inverse_able = Q_temp.Inverse(Q_inverse);
			if (inverse_able == 1) {
				//����
				Vec4f result(0, 0, 0, 1);
				Q_inverse.Transform(result);
				double now_cost = result.Dot4(this->Q*result);
				this->cost = now_cost;
			}
			else {
				//�����棬��v1v2����һ��cost��С�ĵ�
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

//��cost�Ĺ��캯��
edge_cost_node::edge_cost_node(Edge *e,double cost) {
	assert(e != NULL);
	assert(e->getStartVertex() != NULL && e->getEndVertex() != NULL);
	this->StartVertex = e->getStartVertex();
	this->EndVertex = e->getEndVertex();
	this->Q = e->getStartVertex()->getQ() + e->getEndVertex()->getQ();
	this->cost = cost;
}
	//����
	//0 : pop���õ�
	//1 : cost�ı�
	//2 : cost����
	//int update() {
	//	if (this->cost > MaxCost) return 2;//���������ֱ�

	//	assert(this->e != NULL);
	//	assert(this->e->getStartVertex() != NULL && this->e->getEndVertex() != NULL);

	//	if (this->e == NULL || this->e->getStartVertex() == NULL || this->e->getEndVertex() == NULL) {
	//		//�����˸ı�
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
	//��ӣ�����
	void minCostHeap::push(Edge *e) {
		edge_cost_node *node = new edge_cost_node(e);
		if (this->heap.size() == 0) {
			this->heap.push_back(node);
		}
		else {
			//����
			vector<edge_cost_node *>::iterator iter = heap.begin();
			for (; iter < heap.end(); iter++) {
				if (node->getCost() < (*iter)->getCost()) {
					heap.insert(iter, node);
					break;
				}
			}
		}
	}
	//����cost�����������Ϊ�����������һ���ܴ��cost�����Ժ��ٴ���ñ�
	void minCostHeap::push(Edge *e,double cost) {
		edge_cost_node *node = new edge_cost_node(e, cost);
		if (this->heap.size() == 0) {
			this->heap.push_back(node);
		}
		else {
			//�Ѿ��������ڵ㣬����С����˳�����
			vector<edge_cost_node *>::iterator iter = heap.begin();
			//���ʣ���������һ������ô����
			bool inserted = false;
			for (; iter < heap.end(); iter++) {
				if (node->getCost() < (*iter)->getCost()) {
					heap.insert(iter, node);
					inserted = true;
					break;
				}
			}
			if (!inserted) { //��������һ��Ԫ����ȣ����ߴ������һ��Ԫ�ص�cost
				heap.push_back(node); 
			}
		}
	}
	//ɾ���Ѷ�
	void minCostHeap::pop() {
		assert(this->heap.size() > 0);
		heap.erase(heap.begin());
	}

	//vector<Vertex *> around_vertexs,ֻ������Χ��ı����޸�
	//����
	//void minCostHeap::update(edgeshashtype edges, vector<Vertex *> around_vertexs) {
	void minCostHeap::update(edgeshashtype edges, vector<Vertex *> around_vertexs){
		//ʹ��
		//1.�ҵ�����ʧ��node��ֱ��ɾ��
		//2.��û����ʧ�ģ�������cost��Q��erase��������ӣ�
		//�ݴ��
		vector<Edge *> edges_wait_add;

		int change_num = 0;
		int jump_num = 0;


		//����heap�����ÿһ��node
		vector<edge_cost_node *>::iterator iter_heap = this->heap.begin();
		int edge_disapper = 0;
		int edge_cost_change = 0;
		for (; iter_heap != this->heap.end();) {
			//�����һ������around_vertexs�������node
			bool vertex_in_around = false;//��node�ĵ��Ƿ���around_vertexs��
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
				//�߲�����,ɾ����node
				iter_heap = this->heap.erase(iter_heap);
				edge_disapper++;//test
			}
			else {
				//ʹ�ò��������around_vertexs��help update
				//���node������һ��vertex��around_vertexs��




				Vertex *v_start = (*iter_heap)->getStartVertex();
				Vertex *v_end = (*iter_heap)->getEndVertex();
				//�ߴ���,���¼���cost�����cost�Ƿ�ı�
				//���¼���cost
				double old_cost = (*iter_heap)->getCost();
				double new_cost;
				//����ߵ�Q����
				double cost = 0;
				if (v_start->getCost() > MaxCost || v_end->getCost() > MaxCost|| iter_edges->second->getOpposite()==NULL) {
					new_cost = MaxCost + 1;
				}
				else {
					Matrix Q1 = v_start->getQ();
					Matrix Q2 = v_end->getQ();
					Matrix Q = Q1 + Q2;
					//����ߵ�cost
					Matrix Q_inverse;
					Q.set(3, 0, 0);
					Q.set(3, 1, 0);
					Q.set(3, 2, 0);
					Q.set(3, 3, 1);
					int inverse_able = Q.Inverse(Q_inverse);
					if (inverse_able == 1) {
						//����
						Vec4f result(0, 0, 0, 1);
						Q_inverse.Transform(result);
						new_cost = result.Dot4((Q1 + Q2)*result);
					}
					else {
						//�����棬��v1v2����һ��cost��С�ĵ�
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
					//û�иı�
					iter_heap++;
				}
				else {
					edge_cost_change++;
					iter_heap = this->heap.erase(iter_heap);
					//�ı��ˣ�ɾ����ǰ�ڵ㣬��ȡ�ߣ��������
					Edge *e = iter_edges->second;
					//this->push(e);
					edges_wait_add.push_back(e);
				}
			}
		}
		//��e���
		int real_add = 0;
		vector<Edge *>::iterator iter2addedge = edges_wait_add.begin();
		for (; iter2addedge != edges_wait_add.end(); iter2addedge++) {
			this->push((*iter2addedge));
		}
		/*cout << "around vertex num :" << around_vertexs.size() << endl;
		cout << "change node :" << change_num << endl;
		cout << "jump node :" << jump_num << endl;*/
		/*cout << "update end" << endl;
		cout << "��ʧ�ı� : " << edge_disapper << endl;
		cout << "cost�ı�ı� : " << edge_cost_change << endl;*/
	}
	//���ضѶ�
	edge_cost_node* minCostHeap::get_top() {
		assert(this->heap.size() > 0);
		return (*heap.begin());
	}
	//�����ã����صڶ�С����
	edge_cost_node* minCostHeap::get_top_next() {
		assert(this->heap.size() > 1);
		return (*(++heap.begin()));
	}
	//��ն�
	void minCostHeap::clear() {
		this->heap.clear();
	}
	//��ȡ�ѵĴ�С
	int minCostHeap::getsize() {
		return this->heap.size();
	}
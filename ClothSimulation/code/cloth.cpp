#include "glCanvas.h"
#include <vector>
#include <fstream>
#include "cloth.h"
#include "argparser.h"
#include "vectors.h"
#include "utils.h"

using namespace std;

// ================================================================================
// ================================================================================

Cloth::Cloth(ArgParser *_args) {
  args =_args;

  // open the file
  std::cout << args->cloth_file.c_str();
  std::ifstream istr(args->cloth_file.c_str());
  //assert (istr != NULL);
  std::string token;

  // read in the simulation parameters
  istr >> token >> k_structural; assert (token == "k_structural");  // (units == N/m)  (N = kg*m/s^2)
  istr >> token >> k_shear; assert (token == "k_shear");
  istr >> token >> k_bend; assert (token == "k_bend");
  istr >> token >> damping; assert (token == "damping");
  // NOTE: correction factor == .1, means springs shouldn't stretch more than 10%
  //       correction factor == 100, means don't do any correction
  istr >> token >> provot_structural_correction; assert (token == "provot_structural_correction");
  istr >> token >> provot_shear_correction; assert (token == "provot_shear_correction");

  // the cloth dimensions
  istr >> token >> nx >> ny; // (units == meters)
  assert (token == "m");
  assert (nx >= 2 && ny >= 2);

  // the corners of the cloth
  Vec3f a,b,c,d;
  istr >> token >> a; assert (token == "p");
  istr >> token >> b; assert (token == "p");
  istr >> token >> c; assert (token == "p");
  istr >> token >> d; assert (token == "p");

  // fabric weight  (units == kg/m^2)
  // denim ~300 g/m^2
  // silk ~70 g/m^2
  double fabric_weight;
  istr >> token >> fabric_weight; assert (token == "fabric_weight");
  double area = AreaOfTriangle(a,b,c) + AreaOfTriangle(a,c,d);

  // create the particles
  particles = new ClothParticle[nx*ny];//������е�point

  double mass = area*fabric_weight / double(nx*ny);
  for (int i = 0; i < nx; i++) {
    double x = i/double(nx-1);
    Vec3f ab = (1-x)*a + x*b;
    Vec3f dc = (1-x)*d + x*c;
    for (int j = 0; j < ny; j++) {
      double y = j/double(ny-1);
      ClothParticle &p = getParticle(i,j);
      Vec3f abdc = (1-y)*ab + y*dc;
      p.setOriginalPosition(abdc);
      p.setPosition(abdc);
	  p.setLastPosition(abdc);
      p.setVelocity(Vec3f(0,0,0));
	  p.setLastVelocity(Vec3f(0, 0, 0));
	  p.setAcceleration(Vec3f(0, 0, 0));
	  p.setLastAcceleration(Vec3f(0, 0, 0));
      p.setMass(mass);
      p.setFixed(false);
    }
  }

  // the fixed particles
  while (istr >> token) {
    assert (token == "f");
    int i,j;
    double x,y,z;
    istr >> i >> j >> x >> y >> z;
    ClothParticle &p = getParticle(i,j);
    p.setPosition(Vec3f(x,y,z));
	p.setLastPosition(Vec3f(x, y, z));
    p.setFixed(true);
  }

  computeBoundingBox();
  initializeVBOs();
  setupVBOs();
}

// ================================================================================

void Cloth::computeBoundingBox() {
  box = BoundingBox(getParticle(0,0).getPosition());
  for (int i = 0; i < nx; i++) {
    for (int j = 0; j < ny; j++) {
      box.Extend(getParticle(i,j).getPosition());
      box.Extend(getParticle(i,j).getOriginalPosition());
    }
  }
}
int frame = 0;
// ================================================================================

//���η�/���׷�
void Cloth::Animate_Trapezoid() {
	//float max_vel = 0;
	//������
	for (int i = 0; i < nx; ++i) {
		for (int j = 0; j < ny; ++j) {
			//��ÿһ����
			ClothParticle &p = getParticle(i, j);//��ǰ��
			//�洢��last��
			p.setLastPosition(p.getPosition());
			p.setLastVelocity(p.getVelocity());
			p.setLastAcceleration(p.getAcceleration());
			
			//�洢��ǰ����������Ӷ�Ӧ�ĸ�����
			vector<ClothParticle> StructuralParticle;
			vector<ClothParticle> ShearParticle;
			vector<ClothParticle> FlexionParticle;
			if (1) {
				//Ѱ��Structural Spring
				int pos_x, pos_y;
				pos_x = i + 1;
				pos_y = j;
				if (pos_x >= 0 && pos_x < nx && pos_y >= 0 && pos_y < ny) {
					StructuralParticle.push_back(getParticle(pos_x, pos_y));
				}
				pos_x = i - 1;
				pos_y = j;
				if (pos_x >= 0 && pos_x < nx && pos_y >= 0 && pos_y < ny) {
					StructuralParticle.push_back(getParticle(pos_x, pos_y));
				}
				pos_x = i;
				pos_y = j + 1;
				if (pos_x >= 0 && pos_x < nx && pos_y >= 0 && pos_y < ny) {
					StructuralParticle.push_back(getParticle(pos_x, pos_y));
				}
				pos_x = i;
				pos_y = j - 1;
				if (pos_x >= 0 && pos_x < nx && pos_y >= 0 && pos_y < ny) {
					StructuralParticle.push_back(getParticle(pos_x, pos_y));
				}
				//Ѱ��Shear Spring
				pos_x = i + 1;
				pos_y = j + 1;
				if (pos_x >= 0 && pos_x < nx && pos_y >= 0 && pos_y < ny) {
					ShearParticle.push_back(getParticle(pos_x, pos_y));
				}
				pos_x = i + 1;
				pos_y = j - 1;
				if (pos_x >= 0 && pos_x < nx && pos_y >= 0 && pos_y < ny) {
					ShearParticle.push_back(getParticle(pos_x, pos_y));
				}
				pos_x = i - 1;
				pos_y = j + 1;
				if (pos_x >= 0 && pos_x < nx && pos_y >= 0 && pos_y < ny) {
					ShearParticle.push_back(getParticle(pos_x, pos_y));
				}
				pos_x = i - 1;
				pos_y = j - 1;
				if (pos_x >= 0 && pos_x < nx && pos_y >= 0 && pos_y < ny) {
					ShearParticle.push_back(getParticle(pos_x, pos_y));
				}
				//Ѱ��Flexion Spring
				pos_x = i + 2;
				pos_y = j;
				if (pos_x >= 0 && pos_x < nx && pos_y >= 0 && pos_y < ny) {
					FlexionParticle.push_back(getParticle(pos_x, pos_y));
				}
				pos_x = i - 2;
				pos_y = j;
				if (pos_x >= 0 && pos_x < nx && pos_y >= 0 && pos_y < ny) {
					FlexionParticle.push_back(getParticle(pos_x, pos_y));
				}
				pos_x = i;
				pos_y = j + 2;
				if (pos_x >= 0 && pos_x < nx && pos_y >= 0 && pos_y < ny) {
					FlexionParticle.push_back(getParticle(pos_x, pos_y));
				}
				pos_x = i;
				pos_y = j - 2;
				if (pos_x >= 0 && pos_x < nx && pos_y >= 0 && pos_y < ny) {
					FlexionParticle.push_back(getParticle(pos_x, pos_y));
				}
			}
			

			/*cout << "StructuralParticle:" << StructuralParticle.size()<<endl;
			cout << "ShearParticle:" << ShearParticle.size() << endl;
			cout << "FlexionParticle:" << FlexionParticle.size() << endl;*/
			//�Ѿ��ҵ���Χ�ĵ�,�洢��aroundparticles��
			//�����������ķ���
			Vec3f origin_pos = p.getOriginalPosition();//��ǰ�����ĳ�ʼλ��
			Vec3f now_pos = p.getPosition();//��ǰ�����ĵ�ǰλ��
			Vec3f get_forcetructural(0, 0, 0);//ͬһ��spring�����ĺ�
			Vec3f get_forcehear(0, 0, 0);
			Vec3f force_flexion(0, 0, 0);
			vector<ClothParticle>::iterator iter_structural = StructuralParticle.begin();
			vector<ClothParticle>::iterator iter_shear = ShearParticle.begin();
			vector<ClothParticle>::iterator iter_flexion = FlexionParticle.begin();
			for (; iter_structural != StructuralParticle.end(); ++iter_structural) {
				//ԭ���ĳ���
				ClothParticle &temp_p = *iter_structural;
				Vec3f temp_origin_pos = temp_p.getOriginalPosition();//��ʼ����
				Vec3f temp_now_pos = temp_p.getPosition();//��ǰ����
				float origin_length = (origin_pos - temp_origin_pos).Length();//��ʼ����L0
				float now_length = (now_pos - temp_now_pos).Length();//��ǰ����L1
				//������������˵����ǰ�����컹��ѹ��
				//��ǰ��ΪA��spring��һ�˵�ΪB
				//���L1����L0��˵�����죻�������ķ�����AB�෴
				//���L1С��L0��ѹ�����������ķ�����AB������ͬ
				Vec3f F_direction = now_pos - temp_now_pos;
				F_direction.Normalize();
				Vec3f temp_force = (-1) * k_structural * F_direction *(now_length - origin_length);
				get_forcetructural = get_forcetructural + temp_force;//���
			}
			
			for (; iter_shear != ShearParticle.end(); ++iter_shear) {
				//ԭ���ĳ���
				ClothParticle &temp_p = *iter_shear;
				Vec3f temp_origin_pos = temp_p.getOriginalPosition();//��ʼ����
				Vec3f temp_now_pos = temp_p.getPosition();//��ǰ����
				float origin_length = (origin_pos - temp_origin_pos).Length();//��ʼ����L0
				float now_length = (now_pos - temp_now_pos).Length();//��ǰ����L1
				Vec3f F_direction = now_pos - temp_now_pos;
				F_direction.Normalize();

				Vec3f temp_force = (-1) * k_shear * F_direction *(now_length - origin_length);
				get_forcehear = get_forcehear + temp_force;//���
			}
			
			for (; iter_flexion != FlexionParticle.end(); ++iter_flexion) {
				//ԭ���ĳ���
				ClothParticle &temp_p = *iter_flexion;
				Vec3f temp_origin_pos = temp_p.getOriginalPosition();//��ʼ����
				Vec3f temp_now_pos = temp_p.getPosition();//��ǰ����
				float origin_length = (origin_pos - temp_origin_pos).Length();//��ʼ����L0
				float now_length = (now_pos - temp_now_pos).Length();//��ǰ����L1
				//������������˵����ǰ�����컹��ѹ��
				//��ǰ��ΪA��spring��һ�˵�ΪB
				//���L1����L0��˵�����죻�������ķ�����AB�෴
				//���L1С��L0��ѹ�����������ķ�����AB������ͬ
				Vec3f F_direction = now_pos - temp_now_pos;
				F_direction.Normalize();
				Vec3f temp_force = (-1) * k_bend * F_direction *(now_length - origin_length);
				force_flexion = force_flexion + temp_force;//���
			}

			Vec3f damping_force = (-1) * damping * p.getVelocity();
			Vec3f F = get_forcetructural + get_forcehear + force_flexion + damping_force+args->gravity*p.getMass();

			//cout << "structural = " << get_forcetructural << endl;
			//cout << "shear = " << get_forcetructural + get_forcehear  << endl;
			//cout << "bend = " << get_forcetructural + get_forcehear + force_flexion << endl;

			if (!p.isFixed()) {
				//F=ma��F��֪��m��֪
				//������ٶ�
				double mass = p.getMass();
				//�����µ�x,v,a
				
				Vec3f acceleration = F * (1 / mass);
				Vec3f velocity = p.getVelocity() + acceleration * args->timestep;//ʹ�øոռ������acceleration

		
				//ȡƽ��ֵ
				velocity = (velocity + p.getVelocity())*0.5;
				acceleration = (acceleration + p.getAcceleration())*0.5;
				//����position
				Vec3f position = p.getLastPosition() + velocity * args->timestep;

				p.setPosition(position);
				p.setVelocity(velocity);
				p.setAcceleration(acceleration);

				/*if (max_vel < velocity.Length()) {
					max_vel = velocity.Length();
				}*/
			}

		}
	}
	//cout << "max vel " << max_vel << endl;

	for (int i = 0; i < nx; ++i) {
		for (int j = 0; j < ny; ++j) {
			//��ÿһ����
			
			ClothParticle &p1 = getParticle(i, j);//��ǰ��
			for (int m = -1; m < 2; m++) {
				for (int n = -1; n < 2; n++) {
					//���p2�����Ƿ�Խ��
					if (i + m < 0 || i + m >= nx || j + n < 0 || j + n >= ny||(m==0&&n==0)) {
						continue;
					}
					
					//��õ�
					ClothParticle & p2 = getParticle(i + m, j + n);
					//���������
					float limit;
					float offset = 1;
					
					if (abs(m) + abs(n) == 1) {
						limit = provot_structural_correction;//��structural
					}
					else if(abs(m) + abs(n) == 2){
						limit = provot_shear_correction;//��shear
					}
					float now_length = (p2.getPosition() - p1.getPosition()).Length();
					float origin_length = (p2.getOriginalPosition() - p1.getOriginalPosition()).Length();
					Vec3f p1_last_pos = p1.getLastPosition();
					Vec3f p2_last_pos = p2.getLastPosition();
					float def_rate = now_length / origin_length;
					if (def_rate > 1 + limit * offset|| def_rate < 1 - limit * offset) {
						//�ж��ǹ������컹��ѹ��
						if (def_rate > 1 + limit * offset) {
							limit = limit;
						}
						else if (def_rate < 1 - limit * offset) {
							limit = -limit;
						}
						if (p1.isFixed()) {
							//p1�ǹ̶��ģ��ƶ�p2
							Vec3f p2_new_pos = p1.getPosition() + (p2.getPosition() - p1.getPosition())*(1 + limit * offset)*(origin_length / now_length);
							Vec3f p2_new_vel = (p2_new_pos - p2_last_pos)*((1+limit*offset)/(p2_new_pos - p2_last_pos).Length() / now_length*origin_length)*p2.getVelocity().Length();
							//Vec3f p2_new_vel = ((p2_new_pos - p2_last_pos).Length()/now_length)*p2.getVelocity();
							p2.setPosition(p2_new_pos);
							p2.setVelocity(p2_new_vel);
							
						}
						else if (p2.isFixed()) {
							//p2�ǹ̶���
							Vec3f p1_new_pos = p2.getPosition() + (p1.getPosition() - p2.getPosition())*(1 + limit * offset)*(origin_length / now_length);
							Vec3f p1_new_vel = (p1_new_pos - p1_last_pos)*((1 + limit * offset) / (p1_new_pos - p1_last_pos).Length() / now_length*origin_length)*p1.getVelocity().Length();
							p1.setPosition(p1_new_pos);
							p1.setVelocity(p1_new_vel);
						}
						else{
							//���������ǹ̶���
							Vec3f p2_new_pos = p1.getPosition() + (p2.getPosition() - p1.getPosition())*((1 + limit * offset)/2+def_rate/2)*(origin_length / now_length);
							Vec3f p1_new_pos = p2.getPosition() + (p1.getPosition() - p2.getPosition())*((1 + limit * offset)/2+def_rate/2)*(origin_length / now_length);
							
							p2.setPosition(p2_new_pos);
							p1.setPosition(p1_new_pos);

							//�����ٶ�
							Vec3f p2_new_vel = (p2_new_pos - p2_last_pos)*((1 + limit * offset) / (p2_new_pos - p2_last_pos).Length() / def_rate)*p2.getVelocity().Length();
							Vec3f p1_new_vel = (p1_new_pos - p1_last_pos)*((1 + limit * offset) / (p1_new_pos - p1_last_pos).Length() / def_rate)*p1.getVelocity().Length();
							p2.setVelocity(p2_new_vel);
							p1.setVelocity(p1_new_vel);
						}

					}


				}
			}
		}
	}



	// redo VBOs for rendering
	setupVBOs();
}

//Midpoint��
void Cloth::Animate_Midpoint() {
	//float max_vel = 0;
	//����������֮���״̬
	for (int i = 0; i < nx; ++i) {
		for (int j = 0; j < ny; ++j) {
			//��ÿһ����
			ClothParticle &p = getParticle(i, j);//��ǰ��
			//�洢��last��
			p.setLastPosition(p.getPosition());
			p.setLastVelocity(p.getVelocity());
			p.setLastAcceleration(p.getAcceleration());

			//�洢��ǰ����������Ӷ�Ӧ�ĸ�����
			vector<ClothParticle> StructuralParticle;
			vector<ClothParticle> ShearParticle;
			vector<ClothParticle> FlexionParticle;
			if (1) {
				//Ѱ��Structural Spring
				int pos_x, pos_y;
				pos_x = i + 1;
				pos_y = j;
				if (pos_x >= 0 && pos_x < nx && pos_y >= 0 && pos_y < ny) {
					StructuralParticle.push_back(getParticle(pos_x, pos_y));
				}
				pos_x = i - 1;
				pos_y = j;
				if (pos_x >= 0 && pos_x < nx && pos_y >= 0 && pos_y < ny) {
					StructuralParticle.push_back(getParticle(pos_x, pos_y));
				}
				pos_x = i;
				pos_y = j + 1;
				if (pos_x >= 0 && pos_x < nx && pos_y >= 0 && pos_y < ny) {
					StructuralParticle.push_back(getParticle(pos_x, pos_y));
				}
				pos_x = i;
				pos_y = j - 1;
				if (pos_x >= 0 && pos_x < nx && pos_y >= 0 && pos_y < ny) {
					StructuralParticle.push_back(getParticle(pos_x, pos_y));
				}
				//Ѱ��Shear Spring
				pos_x = i + 1;
				pos_y = j + 1;
				if (pos_x >= 0 && pos_x < nx && pos_y >= 0 && pos_y < ny) {
					ShearParticle.push_back(getParticle(pos_x, pos_y));
				}
				pos_x = i + 1;
				pos_y = j - 1;
				if (pos_x >= 0 && pos_x < nx && pos_y >= 0 && pos_y < ny) {
					ShearParticle.push_back(getParticle(pos_x, pos_y));
				}
				pos_x = i - 1;
				pos_y = j + 1;
				if (pos_x >= 0 && pos_x < nx && pos_y >= 0 && pos_y < ny) {
					ShearParticle.push_back(getParticle(pos_x, pos_y));
				}
				pos_x = i - 1;
				pos_y = j - 1;
				if (pos_x >= 0 && pos_x < nx && pos_y >= 0 && pos_y < ny) {
					ShearParticle.push_back(getParticle(pos_x, pos_y));
				}
				//Ѱ��Flexion Spring
				pos_x = i + 2;
				pos_y = j;
				if (pos_x >= 0 && pos_x < nx && pos_y >= 0 && pos_y < ny) {
					FlexionParticle.push_back(getParticle(pos_x, pos_y));
				}
				pos_x = i - 2;
				pos_y = j;
				if (pos_x >= 0 && pos_x < nx && pos_y >= 0 && pos_y < ny) {
					FlexionParticle.push_back(getParticle(pos_x, pos_y));
				}
				pos_x = i;
				pos_y = j + 2;
				if (pos_x >= 0 && pos_x < nx && pos_y >= 0 && pos_y < ny) {
					FlexionParticle.push_back(getParticle(pos_x, pos_y));
				}
				pos_x = i;
				pos_y = j - 2;
				if (pos_x >= 0 && pos_x < nx && pos_y >= 0 && pos_y < ny) {
					FlexionParticle.push_back(getParticle(pos_x, pos_y));
				}
			}


			/*cout << "StructuralParticle:" << StructuralParticle.size()<<endl;
			cout << "ShearParticle:" << ShearParticle.size() << endl;
			cout << "FlexionParticle:" << FlexionParticle.size() << endl;*/
			//�Ѿ��ҵ���Χ�ĵ�,�洢��aroundparticles��
			//�����������ķ���
			Vec3f origin_pos = p.getOriginalPosition();//��ǰ�����ĳ�ʼλ��
			Vec3f now_pos = p.getPosition();//��ǰ�����ĵ�ǰλ��
			Vec3f get_forcetructural(0, 0, 0);//ͬһ��spring�����ĺ�
			Vec3f get_forcehear(0, 0, 0);
			Vec3f force_flexion(0, 0, 0);
			vector<ClothParticle>::iterator iter_structural = StructuralParticle.begin();
			vector<ClothParticle>::iterator iter_shear = ShearParticle.begin();
			vector<ClothParticle>::iterator iter_flexion = FlexionParticle.begin();
			for (; iter_structural != StructuralParticle.end(); ++iter_structural) {
				//ԭ���ĳ���
				ClothParticle &temp_p = *iter_structural;
				Vec3f temp_origin_pos = temp_p.getOriginalPosition();//��ʼ����
				Vec3f temp_now_pos = temp_p.getPosition();//��ǰ����
				float origin_length = (origin_pos - temp_origin_pos).Length();//��ʼ����L0
				float now_length = (now_pos - temp_now_pos).Length();//��ǰ����L1
				//������������˵����ǰ�����컹��ѹ��
				//��ǰ��ΪA��spring��һ�˵�ΪB
				//���L1����L0��˵�����죻�������ķ�����AB�෴
				//���L1С��L0��ѹ�����������ķ�����AB������ͬ
				Vec3f F_direction = now_pos - temp_now_pos;
				F_direction.Normalize();
				Vec3f temp_force = (-1) * k_structural * F_direction *(now_length - origin_length);
				get_forcetructural = get_forcetructural + temp_force;//���
			}

			for (; iter_shear != ShearParticle.end(); ++iter_shear) {
				//ԭ���ĳ���
				ClothParticle &temp_p = *iter_shear;
				Vec3f temp_origin_pos = temp_p.getOriginalPosition();//��ʼ����
				Vec3f temp_now_pos = temp_p.getPosition();//��ǰ����
				float origin_length = (origin_pos - temp_origin_pos).Length();//��ʼ����L0
				float now_length = (now_pos - temp_now_pos).Length();//��ǰ����L1
				Vec3f F_direction = now_pos - temp_now_pos;
				F_direction.Normalize();

				Vec3f temp_force = (-1) * k_shear * F_direction *(now_length - origin_length);
				get_forcehear = get_forcehear + temp_force;//���
			}

			for (; iter_flexion != FlexionParticle.end(); ++iter_flexion) {
				//ԭ���ĳ���
				ClothParticle &temp_p = *iter_flexion;
				Vec3f temp_origin_pos = temp_p.getOriginalPosition();//��ʼ����
				Vec3f temp_now_pos = temp_p.getPosition();//��ǰ����
				float origin_length = (origin_pos - temp_origin_pos).Length();//��ʼ����L0
				float now_length = (now_pos - temp_now_pos).Length();//��ǰ����L1
				//������������˵����ǰ�����컹��ѹ��
				//��ǰ��ΪA��spring��һ�˵�ΪB
				//���L1����L0��˵�����죻�������ķ�����AB�෴
				//���L1С��L0��ѹ�����������ķ�����AB������ͬ
				Vec3f F_direction = now_pos - temp_now_pos;
				F_direction.Normalize();
				Vec3f temp_force = (-1) * k_bend * F_direction *(now_length - origin_length);
				force_flexion = force_flexion + temp_force;//���
			}

			Vec3f damping_force = (-1) * damping * p.getVelocity();
			Vec3f F = get_forcetructural + get_forcehear + force_flexion + damping_force + args->gravity*p.getMass();

			//cout << "structural = " << get_forcetructural << endl;
			//cout << "shear = " << get_forcetructural + get_forcehear  << endl;
			//cout << "bend = " << get_forcetructural + get_forcehear + force_flexion << endl;

			if (!p.isFixed()) {
				//F=ma��F��֪��m��֪
				//������ٶ�
				double mass = p.getMass();
				//�����µ�x,v,a

				//����������֮���x��v��a
				Vec3f acceleration = F * (1 / mass);
				Vec3f velocity = p.getVelocity() + acceleration * args->timestep*0.5;//ʹ�øոռ������acceleration
				//����position
				Vec3f position = p.getLastPosition() + velocity * args->timestep*0.5;

				p.setPosition(position);
				p.setVelocity(velocity);
				p.setAcceleration(acceleration);
			}

		}
	}
	//�ð������֮���״̬���㵱ǰ״̬
	for (int i = 0; i < nx; ++i) {
		for (int j = 0; j < ny; ++j) {
			//��ÿһ����
			ClothParticle &p = getParticle(i, j);//��ǰ��
			//�洢��last��
			p.setLastPosition(p.getPosition());
			p.setLastVelocity(p.getVelocity());
			p.setLastAcceleration(p.getAcceleration());

			//�洢��ǰ����������Ӷ�Ӧ�ĸ�����
			vector<ClothParticle> StructuralParticle;
			vector<ClothParticle> ShearParticle;
			vector<ClothParticle> FlexionParticle;
			if (1) {
				//Ѱ��Structural Spring
				int pos_x, pos_y;
				pos_x = i + 1;
				pos_y = j;
				if (pos_x >= 0 && pos_x < nx && pos_y >= 0 && pos_y < ny) {
					StructuralParticle.push_back(getParticle(pos_x, pos_y));
				}
				pos_x = i - 1;
				pos_y = j;
				if (pos_x >= 0 && pos_x < nx && pos_y >= 0 && pos_y < ny) {
					StructuralParticle.push_back(getParticle(pos_x, pos_y));
				}
				pos_x = i;
				pos_y = j + 1;
				if (pos_x >= 0 && pos_x < nx && pos_y >= 0 && pos_y < ny) {
					StructuralParticle.push_back(getParticle(pos_x, pos_y));
				}
				pos_x = i;
				pos_y = j - 1;
				if (pos_x >= 0 && pos_x < nx && pos_y >= 0 && pos_y < ny) {
					StructuralParticle.push_back(getParticle(pos_x, pos_y));
				}
				//Ѱ��Shear Spring
				pos_x = i + 1;
				pos_y = j + 1;
				if (pos_x >= 0 && pos_x < nx && pos_y >= 0 && pos_y < ny) {
					ShearParticle.push_back(getParticle(pos_x, pos_y));
				}
				pos_x = i + 1;
				pos_y = j - 1;
				if (pos_x >= 0 && pos_x < nx && pos_y >= 0 && pos_y < ny) {
					ShearParticle.push_back(getParticle(pos_x, pos_y));
				}
				pos_x = i - 1;
				pos_y = j + 1;
				if (pos_x >= 0 && pos_x < nx && pos_y >= 0 && pos_y < ny) {
					ShearParticle.push_back(getParticle(pos_x, pos_y));
				}
				pos_x = i - 1;
				pos_y = j - 1;
				if (pos_x >= 0 && pos_x < nx && pos_y >= 0 && pos_y < ny) {
					ShearParticle.push_back(getParticle(pos_x, pos_y));
				}
				//Ѱ��Flexion Spring
				pos_x = i + 2;
				pos_y = j;
				if (pos_x >= 0 && pos_x < nx && pos_y >= 0 && pos_y < ny) {
					FlexionParticle.push_back(getParticle(pos_x, pos_y));
				}
				pos_x = i - 2;
				pos_y = j;
				if (pos_x >= 0 && pos_x < nx && pos_y >= 0 && pos_y < ny) {
					FlexionParticle.push_back(getParticle(pos_x, pos_y));
				}
				pos_x = i;
				pos_y = j + 2;
				if (pos_x >= 0 && pos_x < nx && pos_y >= 0 && pos_y < ny) {
					FlexionParticle.push_back(getParticle(pos_x, pos_y));
				}
				pos_x = i;
				pos_y = j - 2;
				if (pos_x >= 0 && pos_x < nx && pos_y >= 0 && pos_y < ny) {
					FlexionParticle.push_back(getParticle(pos_x, pos_y));
				}
			}


			/*cout << "StructuralParticle:" << StructuralParticle.size()<<endl;
			cout << "ShearParticle:" << ShearParticle.size() << endl;
			cout << "FlexionParticle:" << FlexionParticle.size() << endl;*/
			//�Ѿ��ҵ���Χ�ĵ�,�洢��aroundparticles��
			//�����������ķ���
			Vec3f origin_pos = p.getOriginalPosition();//��ǰ�����ĳ�ʼλ��
			Vec3f now_pos = p.getPosition();//��ǰ�����ĵ�ǰλ��
			Vec3f get_forcetructural(0, 0, 0);//ͬһ��spring�����ĺ�
			Vec3f get_forcehear(0, 0, 0);
			Vec3f force_flexion(0, 0, 0);
			vector<ClothParticle>::iterator iter_structural = StructuralParticle.begin();
			vector<ClothParticle>::iterator iter_shear = ShearParticle.begin();
			vector<ClothParticle>::iterator iter_flexion = FlexionParticle.begin();
			for (; iter_structural != StructuralParticle.end(); ++iter_structural) {
				//ԭ���ĳ���
				ClothParticle &temp_p = *iter_structural;
				Vec3f temp_origin_pos = temp_p.getOriginalPosition();//��ʼ����
				Vec3f temp_now_pos = temp_p.getPosition();//��ǰ����
				float origin_length = (origin_pos - temp_origin_pos).Length();//��ʼ����L0
				float now_length = (now_pos - temp_now_pos).Length();//��ǰ����L1
				//������������˵����ǰ�����컹��ѹ��
				//��ǰ��ΪA��spring��һ�˵�ΪB
				//���L1����L0��˵�����죻�������ķ�����AB�෴
				//���L1С��L0��ѹ�����������ķ�����AB������ͬ
				Vec3f F_direction = now_pos - temp_now_pos;
				F_direction.Normalize();
				Vec3f temp_force = (-1) * k_structural * F_direction *(now_length - origin_length);
				get_forcetructural = get_forcetructural + temp_force;//���
			}

			for (; iter_shear != ShearParticle.end(); ++iter_shear) {
				//ԭ���ĳ���
				ClothParticle &temp_p = *iter_shear;
				Vec3f temp_origin_pos = temp_p.getOriginalPosition();//��ʼ����
				Vec3f temp_now_pos = temp_p.getPosition();//��ǰ����
				float origin_length = (origin_pos - temp_origin_pos).Length();//��ʼ����L0
				float now_length = (now_pos - temp_now_pos).Length();//��ǰ����L1
				Vec3f F_direction = now_pos - temp_now_pos;
				F_direction.Normalize();

				Vec3f temp_force = (-1) * k_shear * F_direction *(now_length - origin_length);
				get_forcehear = get_forcehear + temp_force;//���
			}

			for (; iter_flexion != FlexionParticle.end(); ++iter_flexion) {
				//ԭ���ĳ���
				ClothParticle &temp_p = *iter_flexion;
				Vec3f temp_origin_pos = temp_p.getOriginalPosition();//��ʼ����
				Vec3f temp_now_pos = temp_p.getPosition();//��ǰ����
				float origin_length = (origin_pos - temp_origin_pos).Length();//��ʼ����L0
				float now_length = (now_pos - temp_now_pos).Length();//��ǰ����L1
				//������������˵����ǰ�����컹��ѹ��
				//��ǰ��ΪA��spring��һ�˵�ΪB
				//���L1����L0��˵�����죻�������ķ�����AB�෴
				//���L1С��L0��ѹ�����������ķ�����AB������ͬ
				Vec3f F_direction = now_pos - temp_now_pos;
				F_direction.Normalize();
				Vec3f temp_force = (-1) * k_bend * F_direction *(now_length - origin_length);
				force_flexion = force_flexion + temp_force;//���
			}

			Vec3f damping_force = (-1) * damping * p.getVelocity();
			Vec3f F = get_forcetructural + get_forcehear + force_flexion + damping_force + args->gravity*p.getMass();

			//cout << "structural = " << get_forcetructural << endl;
			//cout << "shear = " << get_forcetructural + get_forcehear  << endl;
			//cout << "bend = " << get_forcetructural + get_forcehear + force_flexion << endl;

			if (!p.isFixed()) {
				//F=ma��F��֪��m��֪
				//������ٶ�
				double mass = p.getMass();
				//�����µ�x,v,a

				//����������֮���x��v��a
				Vec3f acceleration = F * (1 / mass);
				Vec3f velocity = p.getVelocity();//ʹ�øոռ������acceleration
				//����position
				Vec3f position = p.getLastPosition() + velocity * args->timestep;

				p.setPosition(position);
				p.setVelocity(velocity);
				p.setAcceleration(acceleration);

				/*if (max_vel < velocity.Length()) {
					max_vel = velocity.Length();
				}*/
			}

		}
	}

	//std::cout << "max vel = " << max_vel << endl;

	for (int i = 0; i < nx; ++i) {
		for (int j = 0; j < ny; ++j) {
			//��ÿһ����

			ClothParticle &p1 = getParticle(i, j);//��ǰ��
			for (int m = -1; m < 2; m++) {
				for (int n = -1; n < 2; n++) {
					//���p2�����Ƿ�Խ��
					if (i + m < 0 || i + m >= nx || j + n < 0 || j + n >= ny || (m == 0 && n == 0)) {
						continue;
					}

					//��õ�
					ClothParticle & p2 = getParticle(i + m, j + n);
					//���������
					float limit;
					float offset = 1;

					if (abs(m) + abs(n) == 1) {
						limit = provot_structural_correction;//��structural
					}
					else if (abs(m) + abs(n) == 2) {
						limit = provot_shear_correction;//��shear
					}
					float now_length = (p2.getPosition() - p1.getPosition()).Length();
					float origin_length = (p2.getOriginalPosition() - p1.getOriginalPosition()).Length();
					Vec3f p1_last_pos = p1.getLastPosition();
					Vec3f p2_last_pos = p2.getLastPosition();
					float def_rate = now_length / origin_length;
					if (def_rate > 1 + limit * offset || def_rate < 1 - limit * offset) {
						//�ж��ǹ������컹��ѹ��
						if (def_rate > 1 + limit * offset) {
							limit = limit;
						}
						else if (def_rate < 1 - limit * offset) {
							limit = -limit;
						}
						if (p1.isFixed()) {
							//p1�ǹ̶��ģ��ƶ�p2
							Vec3f p2_new_pos = p1.getPosition() + (p2.getPosition() - p1.getPosition())*(1 + limit * offset)*(origin_length / now_length);
							Vec3f p2_new_vel = (p2_new_pos - p2_last_pos)*((1 + limit * offset) / (p2_new_pos - p2_last_pos).Length() / now_length * origin_length)*p2.getVelocity().Length();
							//Vec3f p2_new_vel = ((p2_new_pos - p2_last_pos).Length()/now_length)*p2.getVelocity();
							p2.setPosition(p2_new_pos);
							p2.setVelocity(p2_new_vel);

						}
						else if (p2.isFixed()) {
							//p2�ǹ̶���
							Vec3f p1_new_pos = p2.getPosition() + (p1.getPosition() - p2.getPosition())*(1 + limit * offset)*(origin_length / now_length);
							Vec3f p1_new_vel = (p1_new_pos - p1_last_pos)*((1 + limit * offset) / (p1_new_pos - p1_last_pos).Length() / now_length * origin_length)*p1.getVelocity().Length();
							p1.setPosition(p1_new_pos);
							p1.setVelocity(p1_new_vel);
						}
						else {
							//���������ǹ̶���
							Vec3f p2_new_pos = p1.getPosition() + (p2.getPosition() - p1.getPosition())*((1 + limit * offset) / 2 + def_rate / 2)*(origin_length / now_length);
							Vec3f p1_new_pos = p2.getPosition() + (p1.getPosition() - p2.getPosition())*((1 + limit * offset) / 2 + def_rate / 2)*(origin_length / now_length);

							p2.setPosition(p2_new_pos);
							p1.setPosition(p1_new_pos);

							//�����ٶ�
							Vec3f p2_new_vel = (p2_new_pos - p2_last_pos)*((1 + limit * offset) / (p2_new_pos - p2_last_pos).Length() / def_rate)*p2.getVelocity().Length();
							Vec3f p1_new_vel = (p1_new_pos - p1_last_pos)*((1 + limit * offset) / (p1_new_pos - p1_last_pos).Length() / def_rate)*p1.getVelocity().Length();
							p2.setVelocity(p2_new_vel);
							p1.setVelocity(p1_new_vel);
						}

					}


				}
			}
		}
	}



	// redo VBOs for rendering
	setupVBOs();
}


void Cloth::Animate_AdaptiveTimestep() {
	double max_vel = 0.0f;
	double max_length = 0.0f;

	int max_try_limit = 1000;
	int max_try = max_try_limit;//��������
	int time_step_adapt = 0;//�Բ�����ε���
	for (int i = 0; i < nx; ++i) {
		for (int j = 0; j < ny; ++j) {
			//��ÿһ����
			ClothParticle &p = getParticle(i, j);//��ǰ��
			//�洢��last��
			p.setLastPosition(p.getPosition());
			p.setLastVelocity(p.getVelocity());
			p.setLastAcceleration(p.getAcceleration());
		}
	}
	do{
		max_vel = 0.0f;
		//������
		for (int i = 0; i < nx; ++i) {
			for (int j = 0; j < ny; ++j) {
				//��ÿһ����
				ClothParticle &p = getParticle(i, j);//��ǰ��

				//������ǵ�һ�Σ��ָ�����
				p.setPosition(p.getLastPosition());
				p.setVelocity(p.getLastVelocity());
				p.setAcceleration(p.getLastAcceleration());

				//�洢��ǰ����������Ӷ�Ӧ�ĸ�����
				vector<ClothParticle> StructuralParticle;
				vector<ClothParticle> ShearParticle;
				vector<ClothParticle> FlexionParticle;
				if (1) {
					//Ѱ��Structural Spring
					int pos_x, pos_y;
					pos_x = i + 1;
					pos_y = j;
					if (pos_x >= 0 && pos_x < nx && pos_y >= 0 && pos_y < ny) {
						StructuralParticle.push_back(getParticle(pos_x, pos_y));
					}
					pos_x = i - 1;
					pos_y = j;
					if (pos_x >= 0 && pos_x < nx && pos_y >= 0 && pos_y < ny) {
						StructuralParticle.push_back(getParticle(pos_x, pos_y));
					}
					pos_x = i;
					pos_y = j + 1;
					if (pos_x >= 0 && pos_x < nx && pos_y >= 0 && pos_y < ny) {
						StructuralParticle.push_back(getParticle(pos_x, pos_y));
					}
					pos_x = i;
					pos_y = j - 1;
					if (pos_x >= 0 && pos_x < nx && pos_y >= 0 && pos_y < ny) {
						StructuralParticle.push_back(getParticle(pos_x, pos_y));
					}
					//Ѱ��Shear Spring
					pos_x = i + 1;
					pos_y = j + 1;
					if (pos_x >= 0 && pos_x < nx && pos_y >= 0 && pos_y < ny) {
						ShearParticle.push_back(getParticle(pos_x, pos_y));
					}
					pos_x = i + 1;
					pos_y = j - 1;
					if (pos_x >= 0 && pos_x < nx && pos_y >= 0 && pos_y < ny) {
						ShearParticle.push_back(getParticle(pos_x, pos_y));
					}
					pos_x = i - 1;
					pos_y = j + 1;
					if (pos_x >= 0 && pos_x < nx && pos_y >= 0 && pos_y < ny) {
						ShearParticle.push_back(getParticle(pos_x, pos_y));
					}
					pos_x = i - 1;
					pos_y = j - 1;
					if (pos_x >= 0 && pos_x < nx && pos_y >= 0 && pos_y < ny) {
						ShearParticle.push_back(getParticle(pos_x, pos_y));
					}
					//Ѱ��Flexion Spring
					pos_x = i + 2;
					pos_y = j;
					if (pos_x >= 0 && pos_x < nx && pos_y >= 0 && pos_y < ny) {
						FlexionParticle.push_back(getParticle(pos_x, pos_y));
					}
					pos_x = i - 2;
					pos_y = j;
					if (pos_x >= 0 && pos_x < nx && pos_y >= 0 && pos_y < ny) {
						FlexionParticle.push_back(getParticle(pos_x, pos_y));
					}
					pos_x = i;
					pos_y = j + 2;
					if (pos_x >= 0 && pos_x < nx && pos_y >= 0 && pos_y < ny) {
						FlexionParticle.push_back(getParticle(pos_x, pos_y));
					}
					pos_x = i;
					pos_y = j - 2;
					if (pos_x >= 0 && pos_x < nx && pos_y >= 0 && pos_y < ny) {
						FlexionParticle.push_back(getParticle(pos_x, pos_y));
					}
				}

				//�Ѿ��ҵ���Χ�ĵ�,�洢��aroundparticles��
				//�����������ķ���
				Vec3f origin_pos = p.getOriginalPosition();//��ǰ�����ĳ�ʼλ��
				Vec3f now_pos = p.getPosition();//��ǰ�����ĵ�ǰλ��
				Vec3f get_forcetructural(0, 0, 0);//ͬһ��spring�����ĺ�
				Vec3f get_forcehear(0, 0, 0);
				Vec3f force_flexion(0, 0, 0);
				vector<ClothParticle>::iterator iter_structural = StructuralParticle.begin();
				vector<ClothParticle>::iterator iter_shear = ShearParticle.begin();
				vector<ClothParticle>::iterator iter_flexion = FlexionParticle.begin();
				for (; iter_structural != StructuralParticle.end(); ++iter_structural) {
					//ԭ���ĳ���
					ClothParticle &temp_p = *iter_structural;
					Vec3f temp_origin_pos = temp_p.getOriginalPosition();//��ʼ����
					Vec3f temp_now_pos = temp_p.getPosition();//��ǰ����
					float origin_length = (origin_pos - temp_origin_pos).Length();//��ʼ����L0
					float now_length = (now_pos - temp_now_pos).Length();//��ǰ����L1
					//������������˵����ǰ�����컹��ѹ��
					//��ǰ��ΪA��spring��һ�˵�ΪB
					//���L1����L0��˵�����죻�������ķ�����AB�෴
					//���L1С��L0��ѹ�����������ķ�����AB������ͬ
					Vec3f F_direction = now_pos - temp_now_pos;
					F_direction.Normalize();
					Vec3f temp_force = (-1) * k_structural * F_direction *(now_length - origin_length);
					get_forcetructural = get_forcetructural + temp_force;//���

					if (max_length < origin_length) {
						max_length = origin_length;
					}
				}

				for (; iter_shear != ShearParticle.end(); ++iter_shear) {
					//ԭ���ĳ���
					ClothParticle &temp_p = *iter_shear;
					Vec3f temp_origin_pos = temp_p.getOriginalPosition();//��ʼ����
					Vec3f temp_now_pos = temp_p.getPosition();//��ǰ����
					float origin_length = (origin_pos - temp_origin_pos).Length();//��ʼ����L0
					float now_length = (now_pos - temp_now_pos).Length();//��ǰ����L1
					Vec3f F_direction = now_pos - temp_now_pos;
					F_direction.Normalize();

					Vec3f temp_force = (-1) * k_shear * F_direction *(now_length - origin_length);
					get_forcehear = get_forcehear + temp_force;//���
				}

				for (; iter_flexion != FlexionParticle.end(); ++iter_flexion) {
					//ԭ���ĳ���
					ClothParticle &temp_p = *iter_flexion;
					Vec3f temp_origin_pos = temp_p.getOriginalPosition();//��ʼ����
					Vec3f temp_now_pos = temp_p.getPosition();//��ǰ����
					float origin_length = (origin_pos - temp_origin_pos).Length();//��ʼ����L0
					float now_length = (now_pos - temp_now_pos).Length();//��ǰ����L1
					//������������˵����ǰ�����컹��ѹ��
					//��ǰ��ΪA��spring��һ�˵�ΪB
					//���L1����L0��˵�����죻�������ķ�����AB�෴
					//���L1С��L0��ѹ�����������ķ�����AB������ͬ
					Vec3f F_direction = now_pos - temp_now_pos;
					F_direction.Normalize();
					Vec3f temp_force = (-1) * k_bend * F_direction *(now_length - origin_length);
					force_flexion = force_flexion + temp_force;//���
				}

				Vec3f damping_force = (-1) * damping * p.getVelocity();
				Vec3f F = get_forcetructural + get_forcehear + force_flexion + damping_force + args->gravity*p.getMass();

				int max_try = 10;//������10�β������������ƺ�����
				int time_step_adapt = 0;//-1 ��С������1���󲽳���0���޸Ĳ���
			
			
				if (!p.isFixed()) {
					//F=ma��F��֪��m��֪
					//������ٶ�
					double mass = p.getMass();
					//�����µ�x,v,a

					Vec3f acceleration = F * (1 / mass);
					Vec3f velocity = p.getVelocity() + acceleration * args->timestep;//ʹ�øոռ������acceleration

					velocity = (p.getLastVelocity() + velocity)*0.5;
					acceleration = (acceleration + p.getAcceleration())*0.5;
					//����position
					Vec3f position = p.getLastPosition() + velocity * args->timestep;
					//Ѱ������ٶ�
					if (max_vel < velocity.Length()) {
						max_vel = velocity.Length();
					}
					
					p.setPosition(position);
					p.setVelocity(velocity);
					p.setAcceleration(acceleration);
				}
			
			}
		}
		//cout << "max_vel before limit " << max_vel << endl;
	//�����α�
	for (int i = 0; i < nx; ++i) {
		for (int j = 0; j < ny; ++j) {
			//��ÿһ����

			ClothParticle &p1 = getParticle(i, j);//��ǰ��
			for (int m = -1; m < 2; m++) {
				for (int n = -1; n < 2; n++) {
					//���p2�����Ƿ�Խ��
					if (i + m < 0 || i + m >= nx || j + n < 0 || j + n >= ny || (m == 0 && n == 0)) {
						continue;
					}

					//��õ�
					ClothParticle & p2 = getParticle(i + m, j + n);
					//���������
					float limit;
					float offset = 1;

					if (abs(m) + abs(n) == 1) {
						limit = provot_structural_correction;//��structural
					}
					else if (abs(m) + abs(n) == 2) {
						limit = provot_shear_correction;//��shear
					}
					float now_length = (p2.getPosition() - p1.getPosition()).Length();
					float origin_length = (p2.getOriginalPosition() - p1.getOriginalPosition()).Length();
					Vec3f p1_last_pos = p1.getLastPosition();
					Vec3f p2_last_pos = p2.getLastPosition();
					float def_rate = now_length / origin_length;
					if (def_rate > 1 + limit * offset || def_rate < 1 - limit * offset) {
						//�ж��ǹ������컹��ѹ��
						if (def_rate > 1 + limit * offset) {
							limit = limit;
						}
						else if (def_rate < 1 - limit * offset) {
							limit = -limit;
						}
						if (p1.isFixed()) {
							//p1�ǹ̶��ģ��ƶ�p2
							Vec3f p2_new_pos = p1.getPosition() + (p2.getPosition() - p1.getPosition())*(1 + limit * offset)*(origin_length / now_length);
							Vec3f p2_new_vel = (p2_new_pos - p2_last_pos)*((1 + limit * offset) / (p2_new_pos - p2_last_pos).Length() / now_length * origin_length)*p2.getVelocity().Length();
							//Vec3f p2_new_vel = ((p2_new_pos - p2_last_pos).Length()/now_length)*p2.getVelocity();
							p2.setPosition(p2_new_pos);
							p2.setVelocity(p2_new_vel);
							if (max_vel < p2_new_vel.Length()) {
								max_vel = p2_new_vel.Length();
							}

						}
						else if (p2.isFixed()) {
							//p2�ǹ̶���
							Vec3f p1_new_pos = p2.getPosition() + (p1.getPosition() - p2.getPosition())*(1 + limit * offset)*(origin_length / now_length);
							Vec3f p1_new_vel = (p1_new_pos - p1_last_pos)*((1 + limit * offset) / (p1_new_pos - p1_last_pos).Length() / now_length * origin_length)*p1.getVelocity().Length();
							p1.setPosition(p1_new_pos);
							p1.setVelocity(p1_new_vel);
							if (max_vel < p1_new_vel.Length()) {
								max_vel = p1_new_vel.Length();
							}
						}
						else {
							//���������ǹ̶���
							Vec3f p2_new_pos = p1.getPosition() + (p2.getPosition() - p1.getPosition())*((1 + limit * offset) / 2 + def_rate / 2)*(origin_length / now_length);
							Vec3f p1_new_pos = p2.getPosition() + (p1.getPosition() - p2.getPosition())*((1 + limit * offset) / 2 + def_rate / 2)*(origin_length / now_length);

							p2.setPosition(p2_new_pos);
							p1.setPosition(p1_new_pos);

							//�����ٶ�
							Vec3f p2_new_vel = (p2_new_pos - p2_last_pos)*((1 + limit * offset) / (p2_new_pos - p2_last_pos).Length() / def_rate)*p2.getVelocity().Length();
							Vec3f p1_new_vel = (p1_new_pos - p1_last_pos)*((1 + limit * offset) / (p1_new_pos - p1_last_pos).Length() / def_rate)*p1.getVelocity().Length();
							p2.setVelocity(p2_new_vel);
							p1.setVelocity(p1_new_vel);
							if (max_vel <min( p1_new_vel.Length(),p2_new_vel.Length())) {
								max_vel = min(p1_new_vel.Length(), p2_new_vel.Length());
							}
						}

					}


				}
			}
		}
	}

	//printf("try %d   max_vel %f   time_step  %f", max_try, max_vel, args->timestep);
	--max_try;
	if (max_vel> 3) {
		//cout << "-1" << endl;
		time_step_adapt = -1;
	}
	else if (max_vel<0.3) {
		//cout << "+1" << endl;
		time_step_adapt = 1;
	}
	else {
		//cout << "0" << endl;
		time_step_adapt = 0;
	}

	if (time_step_adapt == 1) {
		
		args->timestep = args->timestep * 2;
		//args->timestep = args->timestep + 0.0001;
		if (args->timestep > args->initial_timestep) {
			args->timestep = args->initial_timestep;
		}
	}
	else if (time_step_adapt == -1) {
		args->timestep = args->timestep * 0.5;
		//args->timestep = args->timestep -0.001;
		if (args->timestep < 0.0001) {
			args->timestep = 0.0001;
		}
	}

	}while (max_try > 0 && time_step_adapt != 0);


	// redo VBOs for rendering
	setupVBOs();
}


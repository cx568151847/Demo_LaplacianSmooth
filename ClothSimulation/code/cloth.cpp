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
  particles = new ClothParticle[nx*ny];//存放所有的point

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

//梯形法/二阶法
void Cloth::Animate_Trapezoid() {
	//float max_vel = 0;
	//计算力
	for (int i = 0; i < nx; ++i) {
		for (int j = 0; j < ny; ++j) {
			//对每一个点
			ClothParticle &p = getParticle(i, j);//当前点
			//存储到last项
			p.setLastPosition(p.getPosition());
			p.setLastVelocity(p.getVelocity());
			p.setLastAcceleration(p.getAcceleration());
			
			//存储当前点的三种连接对应的各个点
			vector<ClothParticle> StructuralParticle;
			vector<ClothParticle> ShearParticle;
			vector<ClothParticle> FlexionParticle;
			if (1) {
				//寻找Structural Spring
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
				//寻找Shear Spring
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
				//寻找Flexion Spring
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
			//已经找到周围的点,存储在aroundparticles中
			//计算各个方向的分力
			Vec3f origin_pos = p.getOriginalPosition();//当前处理点的初始位置
			Vec3f now_pos = p.getPosition();//当前处理点的当前位置
			Vec3f get_forcetructural(0, 0, 0);//同一类spring的力的和
			Vec3f get_forcehear(0, 0, 0);
			Vec3f force_flexion(0, 0, 0);
			vector<ClothParticle>::iterator iter_structural = StructuralParticle.begin();
			vector<ClothParticle>::iterator iter_shear = ShearParticle.begin();
			vector<ClothParticle>::iterator iter_flexion = FlexionParticle.begin();
			for (; iter_structural != StructuralParticle.end(); ++iter_structural) {
				//原来的长度
				ClothParticle &temp_p = *iter_structural;
				Vec3f temp_origin_pos = temp_p.getOriginalPosition();//初始坐标
				Vec3f temp_now_pos = temp_p.getPosition();//当前坐标
				float origin_length = (origin_pos - temp_origin_pos).Length();//初始长度L0
				float now_length = (now_pos - temp_now_pos).Length();//当前长度L1
				//两长度作差来说明当前是拉伸还是压缩
				//当前点为A，spring另一端点为B
				//如果L1大于L0，说明拉伸；弹簧力的方向与AB相反
				//如果L1小于L0是压缩；弹簧力的方向与AB方向相同
				Vec3f F_direction = now_pos - temp_now_pos;
				F_direction.Normalize();
				Vec3f temp_force = (-1) * k_structural * F_direction *(now_length - origin_length);
				get_forcetructural = get_forcetructural + temp_force;//求和
			}
			
			for (; iter_shear != ShearParticle.end(); ++iter_shear) {
				//原来的长度
				ClothParticle &temp_p = *iter_shear;
				Vec3f temp_origin_pos = temp_p.getOriginalPosition();//初始坐标
				Vec3f temp_now_pos = temp_p.getPosition();//当前坐标
				float origin_length = (origin_pos - temp_origin_pos).Length();//初始长度L0
				float now_length = (now_pos - temp_now_pos).Length();//当前长度L1
				Vec3f F_direction = now_pos - temp_now_pos;
				F_direction.Normalize();

				Vec3f temp_force = (-1) * k_shear * F_direction *(now_length - origin_length);
				get_forcehear = get_forcehear + temp_force;//求和
			}
			
			for (; iter_flexion != FlexionParticle.end(); ++iter_flexion) {
				//原来的长度
				ClothParticle &temp_p = *iter_flexion;
				Vec3f temp_origin_pos = temp_p.getOriginalPosition();//初始坐标
				Vec3f temp_now_pos = temp_p.getPosition();//当前坐标
				float origin_length = (origin_pos - temp_origin_pos).Length();//初始长度L0
				float now_length = (now_pos - temp_now_pos).Length();//当前长度L1
				//两长度作差来说明当前是拉伸还是压缩
				//当前点为A，spring另一端点为B
				//如果L1大于L0，说明拉伸；弹簧力的方向与AB相反
				//如果L1小于L0是压缩；弹簧力的方向与AB方向相同
				Vec3f F_direction = now_pos - temp_now_pos;
				F_direction.Normalize();
				Vec3f temp_force = (-1) * k_bend * F_direction *(now_length - origin_length);
				force_flexion = force_flexion + temp_force;//求和
			}

			Vec3f damping_force = (-1) * damping * p.getVelocity();
			Vec3f F = get_forcetructural + get_forcehear + force_flexion + damping_force+args->gravity*p.getMass();

			//cout << "structural = " << get_forcetructural << endl;
			//cout << "shear = " << get_forcetructural + get_forcehear  << endl;
			//cout << "bend = " << get_forcetructural + get_forcehear + force_flexion << endl;

			if (!p.isFixed()) {
				//F=ma，F已知，m已知
				//计算加速度
				double mass = p.getMass();
				//计算新的x,v,a
				
				Vec3f acceleration = F * (1 / mass);
				Vec3f velocity = p.getVelocity() + acceleration * args->timestep;//使用刚刚计算出的acceleration

		
				//取平均值
				velocity = (velocity + p.getVelocity())*0.5;
				acceleration = (acceleration + p.getAcceleration())*0.5;
				//计算position
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
			//对每一个点
			
			ClothParticle &p1 = getParticle(i, j);//当前点
			for (int m = -1; m < 2; m++) {
				for (int n = -1; n < 2; n++) {
					//检查p2坐标是否越界
					if (i + m < 0 || i + m >= nx || j + n < 0 || j + n >= ny||(m==0&&n==0)) {
						continue;
					}
					
					//获得点
					ClothParticle & p2 = getParticle(i + m, j + n);
					//检查点的类型
					float limit;
					float offset = 1;
					
					if (abs(m) + abs(n) == 1) {
						limit = provot_structural_correction;//是structural
					}
					else if(abs(m) + abs(n) == 2){
						limit = provot_shear_correction;//是shear
					}
					float now_length = (p2.getPosition() - p1.getPosition()).Length();
					float origin_length = (p2.getOriginalPosition() - p1.getOriginalPosition()).Length();
					Vec3f p1_last_pos = p1.getLastPosition();
					Vec3f p2_last_pos = p2.getLastPosition();
					float def_rate = now_length / origin_length;
					if (def_rate > 1 + limit * offset|| def_rate < 1 - limit * offset) {
						//判断是过度拉伸还是压缩
						if (def_rate > 1 + limit * offset) {
							limit = limit;
						}
						else if (def_rate < 1 - limit * offset) {
							limit = -limit;
						}
						if (p1.isFixed()) {
							//p1是固定的，移动p2
							Vec3f p2_new_pos = p1.getPosition() + (p2.getPosition() - p1.getPosition())*(1 + limit * offset)*(origin_length / now_length);
							Vec3f p2_new_vel = (p2_new_pos - p2_last_pos)*((1+limit*offset)/(p2_new_pos - p2_last_pos).Length() / now_length*origin_length)*p2.getVelocity().Length();
							//Vec3f p2_new_vel = ((p2_new_pos - p2_last_pos).Length()/now_length)*p2.getVelocity();
							p2.setPosition(p2_new_pos);
							p2.setVelocity(p2_new_vel);
							
						}
						else if (p2.isFixed()) {
							//p2是固定的
							Vec3f p1_new_pos = p2.getPosition() + (p1.getPosition() - p2.getPosition())*(1 + limit * offset)*(origin_length / now_length);
							Vec3f p1_new_vel = (p1_new_pos - p1_last_pos)*((1 + limit * offset) / (p1_new_pos - p1_last_pos).Length() / now_length*origin_length)*p1.getVelocity().Length();
							p1.setPosition(p1_new_pos);
							p1.setVelocity(p1_new_vel);
						}
						else{
							//两个都不是固定的
							Vec3f p2_new_pos = p1.getPosition() + (p2.getPosition() - p1.getPosition())*((1 + limit * offset)/2+def_rate/2)*(origin_length / now_length);
							Vec3f p1_new_pos = p2.getPosition() + (p1.getPosition() - p2.getPosition())*((1 + limit * offset)/2+def_rate/2)*(origin_length / now_length);
							
							p2.setPosition(p2_new_pos);
							p1.setPosition(p1_new_pos);

							//计算速度
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

//Midpoint法
void Cloth::Animate_Midpoint() {
	//float max_vel = 0;
	//计算半个步长之后的状态
	for (int i = 0; i < nx; ++i) {
		for (int j = 0; j < ny; ++j) {
			//对每一个点
			ClothParticle &p = getParticle(i, j);//当前点
			//存储到last项
			p.setLastPosition(p.getPosition());
			p.setLastVelocity(p.getVelocity());
			p.setLastAcceleration(p.getAcceleration());

			//存储当前点的三种连接对应的各个点
			vector<ClothParticle> StructuralParticle;
			vector<ClothParticle> ShearParticle;
			vector<ClothParticle> FlexionParticle;
			if (1) {
				//寻找Structural Spring
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
				//寻找Shear Spring
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
				//寻找Flexion Spring
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
			//已经找到周围的点,存储在aroundparticles中
			//计算各个方向的分力
			Vec3f origin_pos = p.getOriginalPosition();//当前处理点的初始位置
			Vec3f now_pos = p.getPosition();//当前处理点的当前位置
			Vec3f get_forcetructural(0, 0, 0);//同一类spring的力的和
			Vec3f get_forcehear(0, 0, 0);
			Vec3f force_flexion(0, 0, 0);
			vector<ClothParticle>::iterator iter_structural = StructuralParticle.begin();
			vector<ClothParticle>::iterator iter_shear = ShearParticle.begin();
			vector<ClothParticle>::iterator iter_flexion = FlexionParticle.begin();
			for (; iter_structural != StructuralParticle.end(); ++iter_structural) {
				//原来的长度
				ClothParticle &temp_p = *iter_structural;
				Vec3f temp_origin_pos = temp_p.getOriginalPosition();//初始坐标
				Vec3f temp_now_pos = temp_p.getPosition();//当前坐标
				float origin_length = (origin_pos - temp_origin_pos).Length();//初始长度L0
				float now_length = (now_pos - temp_now_pos).Length();//当前长度L1
				//两长度作差来说明当前是拉伸还是压缩
				//当前点为A，spring另一端点为B
				//如果L1大于L0，说明拉伸；弹簧力的方向与AB相反
				//如果L1小于L0是压缩；弹簧力的方向与AB方向相同
				Vec3f F_direction = now_pos - temp_now_pos;
				F_direction.Normalize();
				Vec3f temp_force = (-1) * k_structural * F_direction *(now_length - origin_length);
				get_forcetructural = get_forcetructural + temp_force;//求和
			}

			for (; iter_shear != ShearParticle.end(); ++iter_shear) {
				//原来的长度
				ClothParticle &temp_p = *iter_shear;
				Vec3f temp_origin_pos = temp_p.getOriginalPosition();//初始坐标
				Vec3f temp_now_pos = temp_p.getPosition();//当前坐标
				float origin_length = (origin_pos - temp_origin_pos).Length();//初始长度L0
				float now_length = (now_pos - temp_now_pos).Length();//当前长度L1
				Vec3f F_direction = now_pos - temp_now_pos;
				F_direction.Normalize();

				Vec3f temp_force = (-1) * k_shear * F_direction *(now_length - origin_length);
				get_forcehear = get_forcehear + temp_force;//求和
			}

			for (; iter_flexion != FlexionParticle.end(); ++iter_flexion) {
				//原来的长度
				ClothParticle &temp_p = *iter_flexion;
				Vec3f temp_origin_pos = temp_p.getOriginalPosition();//初始坐标
				Vec3f temp_now_pos = temp_p.getPosition();//当前坐标
				float origin_length = (origin_pos - temp_origin_pos).Length();//初始长度L0
				float now_length = (now_pos - temp_now_pos).Length();//当前长度L1
				//两长度作差来说明当前是拉伸还是压缩
				//当前点为A，spring另一端点为B
				//如果L1大于L0，说明拉伸；弹簧力的方向与AB相反
				//如果L1小于L0是压缩；弹簧力的方向与AB方向相同
				Vec3f F_direction = now_pos - temp_now_pos;
				F_direction.Normalize();
				Vec3f temp_force = (-1) * k_bend * F_direction *(now_length - origin_length);
				force_flexion = force_flexion + temp_force;//求和
			}

			Vec3f damping_force = (-1) * damping * p.getVelocity();
			Vec3f F = get_forcetructural + get_forcehear + force_flexion + damping_force + args->gravity*p.getMass();

			//cout << "structural = " << get_forcetructural << endl;
			//cout << "shear = " << get_forcetructural + get_forcehear  << endl;
			//cout << "bend = " << get_forcetructural + get_forcehear + force_flexion << endl;

			if (!p.isFixed()) {
				//F=ma，F已知，m已知
				//计算加速度
				double mass = p.getMass();
				//计算新的x,v,a

				//计算半个步长之后的x，v，a
				Vec3f acceleration = F * (1 / mass);
				Vec3f velocity = p.getVelocity() + acceleration * args->timestep*0.5;//使用刚刚计算出的acceleration
				//计算position
				Vec3f position = p.getLastPosition() + velocity * args->timestep*0.5;

				p.setPosition(position);
				p.setVelocity(velocity);
				p.setAcceleration(acceleration);
			}

		}
	}
	//用半个步长之后的状态计算当前状态
	for (int i = 0; i < nx; ++i) {
		for (int j = 0; j < ny; ++j) {
			//对每一个点
			ClothParticle &p = getParticle(i, j);//当前点
			//存储到last项
			p.setLastPosition(p.getPosition());
			p.setLastVelocity(p.getVelocity());
			p.setLastAcceleration(p.getAcceleration());

			//存储当前点的三种连接对应的各个点
			vector<ClothParticle> StructuralParticle;
			vector<ClothParticle> ShearParticle;
			vector<ClothParticle> FlexionParticle;
			if (1) {
				//寻找Structural Spring
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
				//寻找Shear Spring
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
				//寻找Flexion Spring
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
			//已经找到周围的点,存储在aroundparticles中
			//计算各个方向的分力
			Vec3f origin_pos = p.getOriginalPosition();//当前处理点的初始位置
			Vec3f now_pos = p.getPosition();//当前处理点的当前位置
			Vec3f get_forcetructural(0, 0, 0);//同一类spring的力的和
			Vec3f get_forcehear(0, 0, 0);
			Vec3f force_flexion(0, 0, 0);
			vector<ClothParticle>::iterator iter_structural = StructuralParticle.begin();
			vector<ClothParticle>::iterator iter_shear = ShearParticle.begin();
			vector<ClothParticle>::iterator iter_flexion = FlexionParticle.begin();
			for (; iter_structural != StructuralParticle.end(); ++iter_structural) {
				//原来的长度
				ClothParticle &temp_p = *iter_structural;
				Vec3f temp_origin_pos = temp_p.getOriginalPosition();//初始坐标
				Vec3f temp_now_pos = temp_p.getPosition();//当前坐标
				float origin_length = (origin_pos - temp_origin_pos).Length();//初始长度L0
				float now_length = (now_pos - temp_now_pos).Length();//当前长度L1
				//两长度作差来说明当前是拉伸还是压缩
				//当前点为A，spring另一端点为B
				//如果L1大于L0，说明拉伸；弹簧力的方向与AB相反
				//如果L1小于L0是压缩；弹簧力的方向与AB方向相同
				Vec3f F_direction = now_pos - temp_now_pos;
				F_direction.Normalize();
				Vec3f temp_force = (-1) * k_structural * F_direction *(now_length - origin_length);
				get_forcetructural = get_forcetructural + temp_force;//求和
			}

			for (; iter_shear != ShearParticle.end(); ++iter_shear) {
				//原来的长度
				ClothParticle &temp_p = *iter_shear;
				Vec3f temp_origin_pos = temp_p.getOriginalPosition();//初始坐标
				Vec3f temp_now_pos = temp_p.getPosition();//当前坐标
				float origin_length = (origin_pos - temp_origin_pos).Length();//初始长度L0
				float now_length = (now_pos - temp_now_pos).Length();//当前长度L1
				Vec3f F_direction = now_pos - temp_now_pos;
				F_direction.Normalize();

				Vec3f temp_force = (-1) * k_shear * F_direction *(now_length - origin_length);
				get_forcehear = get_forcehear + temp_force;//求和
			}

			for (; iter_flexion != FlexionParticle.end(); ++iter_flexion) {
				//原来的长度
				ClothParticle &temp_p = *iter_flexion;
				Vec3f temp_origin_pos = temp_p.getOriginalPosition();//初始坐标
				Vec3f temp_now_pos = temp_p.getPosition();//当前坐标
				float origin_length = (origin_pos - temp_origin_pos).Length();//初始长度L0
				float now_length = (now_pos - temp_now_pos).Length();//当前长度L1
				//两长度作差来说明当前是拉伸还是压缩
				//当前点为A，spring另一端点为B
				//如果L1大于L0，说明拉伸；弹簧力的方向与AB相反
				//如果L1小于L0是压缩；弹簧力的方向与AB方向相同
				Vec3f F_direction = now_pos - temp_now_pos;
				F_direction.Normalize();
				Vec3f temp_force = (-1) * k_bend * F_direction *(now_length - origin_length);
				force_flexion = force_flexion + temp_force;//求和
			}

			Vec3f damping_force = (-1) * damping * p.getVelocity();
			Vec3f F = get_forcetructural + get_forcehear + force_flexion + damping_force + args->gravity*p.getMass();

			//cout << "structural = " << get_forcetructural << endl;
			//cout << "shear = " << get_forcetructural + get_forcehear  << endl;
			//cout << "bend = " << get_forcetructural + get_forcehear + force_flexion << endl;

			if (!p.isFixed()) {
				//F=ma，F已知，m已知
				//计算加速度
				double mass = p.getMass();
				//计算新的x,v,a

				//计算半个步长之后的x，v，a
				Vec3f acceleration = F * (1 / mass);
				Vec3f velocity = p.getVelocity();//使用刚刚计算出的acceleration
				//计算position
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
			//对每一个点

			ClothParticle &p1 = getParticle(i, j);//当前点
			for (int m = -1; m < 2; m++) {
				for (int n = -1; n < 2; n++) {
					//检查p2坐标是否越界
					if (i + m < 0 || i + m >= nx || j + n < 0 || j + n >= ny || (m == 0 && n == 0)) {
						continue;
					}

					//获得点
					ClothParticle & p2 = getParticle(i + m, j + n);
					//检查点的类型
					float limit;
					float offset = 1;

					if (abs(m) + abs(n) == 1) {
						limit = provot_structural_correction;//是structural
					}
					else if (abs(m) + abs(n) == 2) {
						limit = provot_shear_correction;//是shear
					}
					float now_length = (p2.getPosition() - p1.getPosition()).Length();
					float origin_length = (p2.getOriginalPosition() - p1.getOriginalPosition()).Length();
					Vec3f p1_last_pos = p1.getLastPosition();
					Vec3f p2_last_pos = p2.getLastPosition();
					float def_rate = now_length / origin_length;
					if (def_rate > 1 + limit * offset || def_rate < 1 - limit * offset) {
						//判断是过度拉伸还是压缩
						if (def_rate > 1 + limit * offset) {
							limit = limit;
						}
						else if (def_rate < 1 - limit * offset) {
							limit = -limit;
						}
						if (p1.isFixed()) {
							//p1是固定的，移动p2
							Vec3f p2_new_pos = p1.getPosition() + (p2.getPosition() - p1.getPosition())*(1 + limit * offset)*(origin_length / now_length);
							Vec3f p2_new_vel = (p2_new_pos - p2_last_pos)*((1 + limit * offset) / (p2_new_pos - p2_last_pos).Length() / now_length * origin_length)*p2.getVelocity().Length();
							//Vec3f p2_new_vel = ((p2_new_pos - p2_last_pos).Length()/now_length)*p2.getVelocity();
							p2.setPosition(p2_new_pos);
							p2.setVelocity(p2_new_vel);

						}
						else if (p2.isFixed()) {
							//p2是固定的
							Vec3f p1_new_pos = p2.getPosition() + (p1.getPosition() - p2.getPosition())*(1 + limit * offset)*(origin_length / now_length);
							Vec3f p1_new_vel = (p1_new_pos - p1_last_pos)*((1 + limit * offset) / (p1_new_pos - p1_last_pos).Length() / now_length * origin_length)*p1.getVelocity().Length();
							p1.setPosition(p1_new_pos);
							p1.setVelocity(p1_new_vel);
						}
						else {
							//两个都不是固定的
							Vec3f p2_new_pos = p1.getPosition() + (p2.getPosition() - p1.getPosition())*((1 + limit * offset) / 2 + def_rate / 2)*(origin_length / now_length);
							Vec3f p1_new_pos = p2.getPosition() + (p1.getPosition() - p2.getPosition())*((1 + limit * offset) / 2 + def_rate / 2)*(origin_length / now_length);

							p2.setPosition(p2_new_pos);
							p1.setPosition(p1_new_pos);

							//计算速度
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
	int max_try = max_try_limit;//重做次数
	int time_step_adapt = 0;//对步长如何调整
	for (int i = 0; i < nx; ++i) {
		for (int j = 0; j < ny; ++j) {
			//对每一个点
			ClothParticle &p = getParticle(i, j);//当前点
			//存储到last项
			p.setLastPosition(p.getPosition());
			p.setLastVelocity(p.getVelocity());
			p.setLastAcceleration(p.getAcceleration());
		}
	}
	do{
		max_vel = 0.0f;
		//计算力
		for (int i = 0; i < nx; ++i) {
			for (int j = 0; j < ny; ++j) {
				//对每一个点
				ClothParticle &p = getParticle(i, j);//当前点

				//如果不是第一次，恢复三项
				p.setPosition(p.getLastPosition());
				p.setVelocity(p.getLastVelocity());
				p.setAcceleration(p.getLastAcceleration());

				//存储当前点的三种连接对应的各个点
				vector<ClothParticle> StructuralParticle;
				vector<ClothParticle> ShearParticle;
				vector<ClothParticle> FlexionParticle;
				if (1) {
					//寻找Structural Spring
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
					//寻找Shear Spring
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
					//寻找Flexion Spring
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

				//已经找到周围的点,存储在aroundparticles中
				//计算各个方向的分力
				Vec3f origin_pos = p.getOriginalPosition();//当前处理点的初始位置
				Vec3f now_pos = p.getPosition();//当前处理点的当前位置
				Vec3f get_forcetructural(0, 0, 0);//同一类spring的力的和
				Vec3f get_forcehear(0, 0, 0);
				Vec3f force_flexion(0, 0, 0);
				vector<ClothParticle>::iterator iter_structural = StructuralParticle.begin();
				vector<ClothParticle>::iterator iter_shear = ShearParticle.begin();
				vector<ClothParticle>::iterator iter_flexion = FlexionParticle.begin();
				for (; iter_structural != StructuralParticle.end(); ++iter_structural) {
					//原来的长度
					ClothParticle &temp_p = *iter_structural;
					Vec3f temp_origin_pos = temp_p.getOriginalPosition();//初始坐标
					Vec3f temp_now_pos = temp_p.getPosition();//当前坐标
					float origin_length = (origin_pos - temp_origin_pos).Length();//初始长度L0
					float now_length = (now_pos - temp_now_pos).Length();//当前长度L1
					//两长度作差来说明当前是拉伸还是压缩
					//当前点为A，spring另一端点为B
					//如果L1大于L0，说明拉伸；弹簧力的方向与AB相反
					//如果L1小于L0是压缩；弹簧力的方向与AB方向相同
					Vec3f F_direction = now_pos - temp_now_pos;
					F_direction.Normalize();
					Vec3f temp_force = (-1) * k_structural * F_direction *(now_length - origin_length);
					get_forcetructural = get_forcetructural + temp_force;//求和

					if (max_length < origin_length) {
						max_length = origin_length;
					}
				}

				for (; iter_shear != ShearParticle.end(); ++iter_shear) {
					//原来的长度
					ClothParticle &temp_p = *iter_shear;
					Vec3f temp_origin_pos = temp_p.getOriginalPosition();//初始坐标
					Vec3f temp_now_pos = temp_p.getPosition();//当前坐标
					float origin_length = (origin_pos - temp_origin_pos).Length();//初始长度L0
					float now_length = (now_pos - temp_now_pos).Length();//当前长度L1
					Vec3f F_direction = now_pos - temp_now_pos;
					F_direction.Normalize();

					Vec3f temp_force = (-1) * k_shear * F_direction *(now_length - origin_length);
					get_forcehear = get_forcehear + temp_force;//求和
				}

				for (; iter_flexion != FlexionParticle.end(); ++iter_flexion) {
					//原来的长度
					ClothParticle &temp_p = *iter_flexion;
					Vec3f temp_origin_pos = temp_p.getOriginalPosition();//初始坐标
					Vec3f temp_now_pos = temp_p.getPosition();//当前坐标
					float origin_length = (origin_pos - temp_origin_pos).Length();//初始长度L0
					float now_length = (now_pos - temp_now_pos).Length();//当前长度L1
					//两长度作差来说明当前是拉伸还是压缩
					//当前点为A，spring另一端点为B
					//如果L1大于L0，说明拉伸；弹簧力的方向与AB相反
					//如果L1小于L0是压缩；弹簧力的方向与AB方向相同
					Vec3f F_direction = now_pos - temp_now_pos;
					F_direction.Normalize();
					Vec3f temp_force = (-1) * k_bend * F_direction *(now_length - origin_length);
					force_flexion = force_flexion + temp_force;//求和
				}

				Vec3f damping_force = (-1) * damping * p.getVelocity();
				Vec3f F = get_forcetructural + get_forcehear + force_flexion + damping_force + args->gravity*p.getMass();

				int max_try = 10;//最多调整10次步长，超过限制后跳出
				int time_step_adapt = 0;//-1 减小步长，1增大步长，0不修改步长
			
			
				if (!p.isFixed()) {
					//F=ma，F已知，m已知
					//计算加速度
					double mass = p.getMass();
					//计算新的x,v,a

					Vec3f acceleration = F * (1 / mass);
					Vec3f velocity = p.getVelocity() + acceleration * args->timestep;//使用刚刚计算出的acceleration

					velocity = (p.getLastVelocity() + velocity)*0.5;
					acceleration = (acceleration + p.getAcceleration())*0.5;
					//计算position
					Vec3f position = p.getLastPosition() + velocity * args->timestep;
					//寻找最大速度
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
	//限制形变
	for (int i = 0; i < nx; ++i) {
		for (int j = 0; j < ny; ++j) {
			//对每一个点

			ClothParticle &p1 = getParticle(i, j);//当前点
			for (int m = -1; m < 2; m++) {
				for (int n = -1; n < 2; n++) {
					//检查p2坐标是否越界
					if (i + m < 0 || i + m >= nx || j + n < 0 || j + n >= ny || (m == 0 && n == 0)) {
						continue;
					}

					//获得点
					ClothParticle & p2 = getParticle(i + m, j + n);
					//检查点的类型
					float limit;
					float offset = 1;

					if (abs(m) + abs(n) == 1) {
						limit = provot_structural_correction;//是structural
					}
					else if (abs(m) + abs(n) == 2) {
						limit = provot_shear_correction;//是shear
					}
					float now_length = (p2.getPosition() - p1.getPosition()).Length();
					float origin_length = (p2.getOriginalPosition() - p1.getOriginalPosition()).Length();
					Vec3f p1_last_pos = p1.getLastPosition();
					Vec3f p2_last_pos = p2.getLastPosition();
					float def_rate = now_length / origin_length;
					if (def_rate > 1 + limit * offset || def_rate < 1 - limit * offset) {
						//判断是过度拉伸还是压缩
						if (def_rate > 1 + limit * offset) {
							limit = limit;
						}
						else if (def_rate < 1 - limit * offset) {
							limit = -limit;
						}
						if (p1.isFixed()) {
							//p1是固定的，移动p2
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
							//p2是固定的
							Vec3f p1_new_pos = p2.getPosition() + (p1.getPosition() - p2.getPosition())*(1 + limit * offset)*(origin_length / now_length);
							Vec3f p1_new_vel = (p1_new_pos - p1_last_pos)*((1 + limit * offset) / (p1_new_pos - p1_last_pos).Length() / now_length * origin_length)*p1.getVelocity().Length();
							p1.setPosition(p1_new_pos);
							p1.setVelocity(p1_new_vel);
							if (max_vel < p1_new_vel.Length()) {
								max_vel = p1_new_vel.Length();
							}
						}
						else {
							//两个都不是固定的
							Vec3f p2_new_pos = p1.getPosition() + (p2.getPosition() - p1.getPosition())*((1 + limit * offset) / 2 + def_rate / 2)*(origin_length / now_length);
							Vec3f p1_new_pos = p2.getPosition() + (p1.getPosition() - p2.getPosition())*((1 + limit * offset) / 2 + def_rate / 2)*(origin_length / now_length);

							p2.setPosition(p2_new_pos);
							p1.setPosition(p1_new_pos);

							//计算速度
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


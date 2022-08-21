/*
by winka9587 date:2019.5.29
鼠标左键移动模型
鼠标右键旋转模型
鼠标滚轮缩放模型
R键重置
C键使用Laplacian算子进行网格光顺
S键切换模型显示
V键翻转顶点向量
*/
#define WindowWidth  600
#define WindowHeight 600
#define WindowTitle  "Laplacian网格光顺"
//定义滚轮操作
#define  GLUT_WHEEL_UP 3           
#define  GLUT_WHEEL_DOWN 4

#include "objLoader.h"


string filepath = "data/bunny-5000.obj";
//string filepath = "data/test.obj";
objLoader loader = objLoader(filepath);

enum NowPressedButton { nonebutton, leftbutton, rightbutton,middlebutton };
NowPressedButton nowpressedbutton = nonebutton;//哪个鼠标被按下

int NowWindowWidth, NowWindowHeight;
int lastx_rotate, lasty_rotate;//用于旋转时鼠标按下时的坐标
int lastx_move, lasty_move;//用于移动时鼠标按下时的坐标
float oldmovex = 0.0f, oldmovey = 0.0f;//总的移动距离
float oldangle_x = 0.0f, oldangle_y = 0.0f;//总的旋转角度
float movex, movey;//这一次按下鼠标的移动距离
float angle_x, angle_y;//这一次按下鼠标的旋转角度
float zoomsize = 1.0f;//鼠标滚轮缩放

//取消旋转移动变换，返回默认视角
void return2default() {
	oldmovex = 0.0f, oldmovey = 0.0f;
	oldangle_x = 0, oldangle_y = 0.0f;
	movex = 0.0f, movey = 0.0f;
	angle_x = 0.0f, angle_y = 0.0f;
	zoomsize = 1.0f;
}

void display(void) {
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	GLfloat sun_light_position[] = { 0.0f, 2000.0f, 2000.0f, 1.0f }; // 光源的位置
	GLfloat sun_light_ambient[] = { 1.0f, 1.0f, 1.0f, 1.0f };
	GLfloat sun_light_diffuse[] = { 1.0f, 1.0f, 1.0f, 1.0f };
	GLfloat sun_light_specular[] = { 1.0f, 1.0f, 1.0f, 1.0f };

	glLightfv(GL_LIGHT0, GL_POSITION, sun_light_position); //指定0光源的位置   
	glLightfv(GL_LIGHT0, GL_AMBIENT, sun_light_ambient); //环境光
	glLightfv(GL_LIGHT0, GL_DIFFUSE, sun_light_diffuse); //漫反射  
	glLightfv(GL_LIGHT0, GL_SPECULAR, sun_light_specular);//镜面反射  

	glEnable(GL_LIGHT0); //启用第0号光
	glEnable(GL_LIGHTING); //启用光照  


	glPushMatrix();
	glScalef(zoomsize, zoomsize, zoomsize);
	glRotatef(-oldangle_y -angle_y, 1, 0, 0);
	glRotatef(oldangle_x + angle_x, 0, 1, 0);
	glTranslatef(oldmovex + movex,0,0);
	glTranslatef(0, 0, -oldmovey - movey);

	loader.drawobj();
	glPopMatrix();

	glutSwapBuffers();
}

//窗口大小变化
void reshape(int w, int h) {
	NowWindowWidth = w;
	NowWindowHeight = h;
	
	if (w > h) {
		glViewport(0, 0, h, h);
	}
	else {
		glViewport(0, 0, w, w);
	}

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(75, 1, 0.0001, 3);
	gluLookAt(0.0, 0.2, 0.2, 0, 0.05, 0, 0, 1, 0);//原视角
	glMatrixMode(GL_MODELVIEW);
}



void onMouseDown(int button,int state, int x, int y) {
	switch (button) {
	case GLUT_LEFT_BUTTON:
		nowpressedbutton = leftbutton;
		break;
	case GLUT_RIGHT_BUTTON:
		nowpressedbutton = rightbutton;
		break;
	case GLUT_MIDDLE_BUTTON:
		nowpressedbutton = middlebutton;
		break;
	}

	if (state == GLUT_DOWN) {
		if (nowpressedbutton == leftbutton) {
			lastx_move = x;
			lasty_move = WindowHeight - y;
		}
		if (nowpressedbutton == rightbutton) {
			lastx_rotate = x;
			lasty_rotate = WindowHeight - y;
		}
	}
	if (state == GLUT_UP) {
		float zoomstep = 0.1;
		if (button == GLUT_WHEEL_UP) {
			zoomsize += zoomstep;
		}
		if (button == GLUT_WHEEL_DOWN) {
			zoomsize -= zoomstep;
		}
		oldangle_x += angle_x;
		oldangle_y += angle_y;
		angle_x = 0;
		angle_y = 0;

		oldmovex += movex;
		oldmovey += movey;
		movex = 0;
		movey = 0;
	}

}
float offsetx_move, offsety_move,offsetx_rotate, offsety_rotate;
float movestep = 0.5,
		rotatestep = 30;
void onMouseDownAndMove(int x, int y) {
	int nowx = x,
		nowy = WindowHeight - y;

	switch (nowpressedbutton) {
		case leftbutton:
			//左键拖拽移动
			offsetx_move = nowx - lastx_move;//先转换int到float，否则下一步除法时变为int只剩0
			offsety_move = nowy - lasty_move;

			movex = offsetx_move / NowWindowWidth* movestep;
			movey = offsety_move / NowWindowHeight* movestep;
			break;
		case rightbutton:
			//右键旋转
			offsetx_rotate = nowx - lastx_rotate;
			offsety_rotate = nowy - lasty_rotate;
			
			angle_x = offsetx_rotate / NowWindowWidth*3.14*rotatestep;
			angle_y = offsety_rotate / NowWindowHeight*3.14*rotatestep;
			break;
	}
	glutPostRedisplay();
}

float scalesize = 10.0f;
void keyboardFunc(unsigned char key, int x, int y) {
	if (key == 's') {
		loader.switchdrawmode();
	}
	if (key == 'r') {
		loader.reset_vset();
		return2default();
	}
	if (key == 'c') {
		loader.LaplacianCal();
	}
	if (key == 'v') {
		loader.ReverseVertexNormal();
	}
}

void myIdle(void)
{
	display();
}
int main(int argc, char* argv[])
{
	printf(" 鼠标左键移动模型\n 鼠标右键旋转模型\n 鼠标滚轮缩放模型\n R 键重置\n C 键使用Laplacian算子进行网格光顺\n S 键切换模型显示\n V 键翻转顶点向量\n");
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA);
	glutInitWindowPosition(300, 300);
	glutInitWindowSize(WindowWidth, WindowHeight);
	glutCreateWindow(WindowTitle);

	glEnable(GL_DEPTH_TEST);
	glutDisplayFunc(&display);
	glutReshapeFunc(reshape);
	glutMouseFunc(onMouseDown);
	glutMotionFunc(onMouseDownAndMove);
	
	glutKeyboardFunc(keyboardFunc);

	glutIdleFunc(&myIdle);
	glutMainLoop();
	
	return 0;
}

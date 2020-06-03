#include <windows.h>
#include <time.h>
#include <math.h>
#include <stdio.h>
#include <sstream>
#include <cstring>
#include <string>
#include <memory>
#include <iostream>
#define PI 3.14285714286
//размер экрана 
int ekranX=1000;
int ekranY=600;
double prib=(float)ekranX/2;//умножение рендера
using namespace std;
//---- глобальные переменные -------------------------------------
HWND          hWnd;   // описание окна приложения
RECT W_Client;         // размер клиентской области окна
PAINTSTRUCT ps;
//контекст устройства и кисть для рисования
HDC hDC ;
HBRUSH brushB, brushC,brushA;     // объявление кистей дл фона, шарика                       
HPEN   penA, penB,  penSh;       // объявление пера для контуров  
//второй буфер и битмап для реализации двухбуферной отрисовки
HDC hDC2 ;
HBITMAP bmp; 
//что-то WinApiшное
LRESULT CALLBACK WndProc(HWND hWnd, UINT nMsg, WPARAM wParam,
	LPARAM lParam);
//для вскытия 24х битного БМП изображения
BITMAPFILEHEADER bfh_l;
BITMAPINFOHEADER bih_l;
RGBTRIPLE rgb_l;
FILE * f1;
//степень детализации
const int N=26;
const int NP=N-1;
// прохождение стен
bool flymode = 0;
// при запуске будет запрет на рендер и прочее, чтобы показать меню
bool start = 1;
// номер карты
int MapCounter=0;
//координаты мыши и разрешение на работу мыши
double xPos=0;double yPos=0;
	bool mousemove;
	bool mouseRight;

const int W = 20;
const int H = 20;
int mapWidth = W;
int mapHeight = H;
//двумерное поле, где число - это высота стенки         
int pole[H][W];
// карта пола 
int poleFloor[H][W];
// карта потолка 
int poleCeiling[H][W];
// карта дверей
int poleO[H][W];
// карты мелких объектов
int poleObj[H][W];
struct stens 
{
	// кол-во стен для 1 клетки
	int num[6];
};
stens poleOvisible[20][20];
// координаты мыши + старые координаты для поворота и для чего-нибудь ещё, например.
double xmouse, ymouse, xoldmouse, yoldmouse;			
double dt=5*PI/180;   // изменение угла поворота
// кол-во обновлений экрана в сек. (откл)
int fps;
//автономный поворот (откл)
bool vX,vY,vZ;
//масштаб рендера
double mast=0;
// кол-во объектов
int numgen=0;
int objposl[2000];// последовательность отрисовки объектов
POINT  point[4];
bool drawscreen=true;//  нужна для того, чтобы ф-ция screen() не вызывалась тогда, когда уже работает
int HP=0;
double cX=3; double cY=0; double cZ=3; // положения камеры
double cA=0;// 90*PI/180 ;// угол поворота камеры. Изначально равен 90 градусам
double cAz=0; // угол поворота камеры по вертикали 
//-- начальная установка датчика случайных чисел ---------------------------------
void randomize()
{
	srand(time(0));
}

template <typename T>
std::string to_string(T value)
{
	//create an output string stream
	std::ostringstream os;

	//throw the value into the string stream
	os << value;

	//convert the string stream into a string and return
	return os.str();
}

int fcloseall(){
	return 0;
}
//-- получить случайное число ----------------------------------------------------
int random (int limit)
{
	int n=rand() % limit;
	return n;
}
//class object
class object
{
	//public 
public :
	struct Tp			// вершина
	{
		double dlina()
		{
			return sqrt(x*x+y*y+z*z);
		}

		double x,y,z;
	};

	struct Tl			// полигон
	{
		//для одного полигона нужно 4 точки
		Tp *a, *b, *c, *d;
	};
	bool visible;// видимость
	Tl l[NP*NP]; //количество полигонов
	int posl[NP*NP];
	Tp p[N*N] ; // кол-во точек
	double x, z;// положения для сортировки
	int Red[NP*NP];
	int Green[NP*NP];
	int Blue[NP*NP];
	//id типа объекта
	int objtype;
	//разрешение на поворот, если == 0, то будет спрайт
	bool turning;
	//нужна ли сортировка полигонов внутри объекта
	bool Zsorting;
	//ради прикола некоторые объекты крутятся вокруг своей оси
	bool autoturning;
	//генерация объета, 3 координаты расположения + тип
	object(double Z ,double X, double Y,int Type,int noise )
	{
		//для установки координат
		Z-=cZ;
		X-=cX;
		autoturning=false;
		Zsorting=false;
		visible=true;
		objtype = Type;
		turning=true;
		 
		for (int i=0;i<NP;i++) 
			for (int j=0;j<NP;j++)
				posl[i*NP+j]=i*NP+j;
		for (int i=0;i<N;i++)
		{
			for (int j=0;j<N;j++)
			{
				double jj=j;
				double ii=i;

				p[i+j*N].x=X;
				p[i+j*N].y=Y;
				p[i+j*N].z=Z;
				// шороховатые стены
				if(i!=0 && j!=0 && i!=(N-1) && j!=(N-1))
				{
					p[i+j*N].x+=((double)random(noise))/10000;
					p[i+j*N].y+=((double)random(noise))/10000;
					p[i+j*N].z+=((double)random(noise))/10000;
				}
				if (Type == 0)
				{
					p[i+j*N].y += jj / (NP);
					p[i+j*N].z += ii / (NP);
				}
				if (Type == 1)
				{
					p[i+j*N].x += ii / (NP);
					p[i+j*N].y += jj / (NP);
				}
				if (Type == 2)
				{
					p[i+j*N].x += jj / (NP);
					p[i+j*N].z += ii / (NP);
				}
				// чуть сжатые ( нужны для двери) 
				if (Type == 3)
				{
					p[i+j*N].x += jj / (NP);
					p[i+j*N].z += ii /(N+5);
				}
				if (Type == 4)
				{
					p[i+j*N].x += jj / (N+5);
					p[i+j*N].z += ii / (NP);
				}
			}
		}
		for (int i = 0; i < NP; i++)
		{
			for (int j = 0; j < NP; j++)
			{ 
				l[NP*j+i].a=&(p[N*j+i]);
				l[NP*j+i].b=&(p[N*j+i+1]);
				l[NP*j+i].c=&(p[N*(j+1)+i+1]);
				l[NP*j+i].d=&(p[N*(j+1)+i]);
				if (i==NP)
				{
					l[NP*j+i].a=&(p[N*j+0]);
					l[NP*j+i].b=&(p[N*j+(N-1)]);
					l[NP*j+i].c=&(p[N*(j)+(N-1)]);
					l[NP*j+i].d=&(p[N*(j)+0]);
				}
			}
		}
		x=p[(N/2)*(N/2)].x;
		z=p[(N/2)*(N/2)].z;
	}
	//процедура загружающая цвета
void init(int vib)
	{	
		string a("textures/Texture");
		a += to_string(static_cast<int>(vib));
		a+=".bmp";
		f1 = fopen(a.c_str(), "r+b");
		fread(&bfh_l,sizeof(bfh_l),1,f1);               //Запихиваем файловый заголовок в структуру BITMAPFILEHEADER
		fread(&bih_l,sizeof(bih_l),1,f1);               //Запихиваем заголовок изображения в структуру BITMAPINFOHEADER
		int padding = 0;
		if ((bih_l.biWidth * 3) % 4) 
			padding = 4 - (bih_l.biWidth * 3) % 4;

		for(int i=0;i < bih_l.biHeight;i++)
		{
			for (int j =  0; j < bih_l.biWidth; j++)
			{
				fread(&rgb_l, sizeof(rgb_l),1, f1);			  
				if (i < NP && j < NP)
				{  
					Red[(NP-1-i)*NP+j] = rgb_l.rgbtRed ;
					Green[(NP-1-i)*NP+j]=rgb_l.rgbtGreen  ;
					Blue[(NP-1-i)*NP+j] =rgb_l.rgbtBlue ;

				}    
			}
			if(padding != 0) 
			{
				fread(&rgb_l, padding,1, f1); 
			}

		}
		fcloseall();
	}
	
};

shared_ptr<object> obj[2048];

//процедура поворота в трёхмерной оси, даются 2 переменные, а затем они перерасчитываюся.
void  turn(double &a, double &b, double d)
{ 
	double a0=a, b0=b;
	a=a0*cos(d) + b0*sin(d);
	b=b0*cos(d) - a0*sin(d);
}
//проверка объектов по их Z для установки порядка отрисовки
void proverkaZobj()
{
	// переменные для перестановки
	double t1;
	double t2;
	for (int B=0;B<numgen;B++)
		for (int BB=B;BB<numgen;BB++)
		{
			//небольшая проверка по центру квадрата. Конечно, был разработан метод, но в качестве памятника пускай тут будет это
			t1 = /*sqrt(
				abs(obj[objposl[B] ]->p[0].z*obj[objposl[B] ]->p[0].z)
				+	abs(obj[objposl[B] ]->p[(N)].z*obj[objposl[B] ]->p[(N)].z)
				+	abs(obj[objposl[B] ]->p[((N)*((N))-1)].z*obj[objposl[B] ]->p[N*N-1].z)
				+	abs(obj[objposl[B] ]->p[(N)*(N)].z*obj[objposl[B] ]->p[(N)*(N)].z)

				+ abs(obj[objposl[B] ]->p[0].x*obj[objposl[B] ]->p[0].x)
				+	abs(obj[objposl[B] ]->p[(N)].x*obj[objposl[B] ]->p[(N)].x)
				+	abs(obj[objposl[B] ]->p[(((N))*((N))-1)].x*obj[objposl[B] ]->p[N*N-1].x)
				+	abs(obj[objposl[B] ]->p[((N))*((N))].x*obj[objposl[B] ]->p[((N))*((N))].x)

				+ abs(obj[objposl[B] ]->p[0].y*obj[objposl[B] ]->p[0].y)
				+	abs(obj[objposl[B] ]->p[(N)].y*obj[objposl[B] ]->p[(N)].y)
				+	abs(obj[objposl[B] ]->p[(((N))*((N))-1)].y*obj[objposl[B] ]->p[(((N))*((N))-1)].y)
				+	abs(obj[objposl[B] ]->p[((N))*((N))].y*obj[objposl[B] ]->p[((N))*((N))].y)
				);*/
				obj[objposl[B]]->p[(N/2) * N + N/2].dlina();

			t2= /*sqrt(
				abs(obj[objposl[BB] ]->p[0].z*obj[objposl[BB] ]->p[0].z)
				+	abs(obj[objposl[B] ]->p[(N)].z*obj[objposl[BB] ]->p[(N)].z)
				+	abs(obj[objposl[BB] ]->p[(((N))*((N))-1)].z*obj[objposl[BB] ]->p[(((N))*((N))-1)].z)
				+	abs(obj[objposl[BB] ]->p[((N))*((N))].z*obj[objposl[BB] ]->p[((N))*((N))].z)

				+ abs(obj[objposl[BB] ]->p[0].x*obj[objposl[BB] ]->p[0].x)
				+	abs(obj[objposl[B] ]->p[(N)].x*obj[objposl[BB] ]->p[(N)].x)
				+	abs(obj[objposl[BB] ]->p[(((N))*((N))-1)].x*obj[objposl[BB] ]->p[(((N))*((N))-1)].x)
				+	abs(obj[objposl[BB] ]->p[((N))*((N))].x*obj[objposl[BB] ]->p[((N))*((N))].x)

				+ abs(obj[objposl[BB] ]->p[0].y*obj[objposl[BB] ]->p[0].y)
				+	abs(obj[objposl[B] ]->p[(N)].y*obj[objposl[BB] ]->p[(N)].y)
				+	abs(obj[objposl[BB] ]->p[(((N))*((N))-1)].y*obj[objposl[BB] ]->p[(((N))*((N))-1)].y)
				+	abs(obj[objposl[BB] ]->p[((N))*((N))].y*obj[objposl[BB] ]->p[((N))*((N))].y)
				);*/
				obj[objposl[BB]]->p[(N/2) * N + N/2].dlina();
			if(t1<=t2)
				swap(objposl[B], objposl[BB]);
		}
}
HBRUSH exlol;
 
// ради забавы можно поменять цветовую насыщенность 
double redK=1;
double greenK=1;
double blueK=1;
double rgbtime=0;
/*                                                                                                                  
 ▄▄▄▄▄▄▄▄▄▄▄  ▄▄▄▄▄▄▄▄▄▄▄  ▄▄▄▄▄▄▄▄▄▄▄  ▄▄▄▄▄▄▄▄▄▄▄  ▄▄▄▄▄▄▄▄▄▄▄  ▄▄        ▄ 
▐░░░░░░░░░░░▌▐░░░░░░░░░░░▌▐░░░░░░░░░░░▌▐░░░░░░░░░░░▌▐░░░░░░░░░░░▌▐░░▌      ▐░▌
▐░█▀▀▀▀▀▀▀▀▀ ▐░█▀▀▀▀▀▀▀▀▀ ▐░█▀▀▀▀▀▀▀█░▌▐░█▀▀▀▀▀▀▀▀▀ ▐░█▀▀▀▀▀▀▀▀▀ ▐░▌░▌     ▐░▌
▐░▌          ▐░▌          ▐░▌   :)  ▐░▌▐░▌          ▐░▌          ▐░▌▐░▌    ▐░▌
▐░█▄▄▄▄▄▄▄▄▄ ▐░▌          ▐░█▄▄▄▄▄▄▄█░▌▐░█▄▄▄▄▄▄▄▄▄ ▐░█▄▄▄▄▄▄▄▄▄ ▐░▌ ▐░▌   ▐░▌
▐░░░░░░░░░░░▌▐░▌          ▐░░░░░░░░░░░▌▐░░░░░░░░░░░▌▐░░░░░░░░░░░▌▐░▌  ▐░▌  ▐░▌
 ▀▀▀▀▀▀▀▀▀█░▌▐░▌          ▐░█▀▀▀▀█░█▀▀ ▐░█▀▀▀▀▀▀▀▀▀ ▐░█▀▀▀▀▀▀▀▀▀ ▐░▌   ▐░▌ ▐░▌
          ▐░▌▐░▌          ▐░▌     ▐░▌  ▐░▌          ▐░▌          ▐░▌    ▐░▌▐░▌
 ▄▄▄▄▄▄▄▄▄█░▌▐░█▄▄▄▄▄▄▄▄▄ ▐░▌      ▐░▌ ▐░█▄▄▄▄▄▄▄▄▄ ▐░█▄▄▄▄▄▄▄▄▄ ▐░▌     ▐░▐░▌
▐░░░░░░░░░░░▌▐░░░░░░░░░░░▌▐░▌       ▐░▌▐░░░░░░░░░░░▌▐░░░░░░░░░░░▌▐░▌      ▐░░▌
 ▀▀▀▀▀▀▀▀▀▀▀  ▀▀▀▀▀▀▀▀▀▀▀  ▀         ▀  ▀▀▀▀▀▀▀▀▀▀▀  ▀▀▀▀▀▀▀▀▀▀▀  ▀        ▀▀                                                                                                            
*/
void screen()
{
if (drawscreen)
{
 //rgbtime += 0.01;

	redK = 1;//1.41421356*abs(cos(rgbtime));
 greenK = 1;//1.41421356*abs(cos(rgbtime+2*PI/3));
 blueK = 1;//1.41421356*abs(cos(rgbtime+PI/3));
 drawscreen=0;
	exlol = CreateSolidBrush(RGB(0,0,0));
	SelectObject(hDC2, exlol);
	SelectObject(hDC2, penB);	 
	SelectObject( hDC2, bmp );
	GetClientRect(hWnd, &W_Client);	 
	Polygon(hDC2,point,4);	
	DeleteObject(exlol);
	point[0].x=0;  point[0].y=0;
	point[1].x=ekranX;  point[1].y=0 ;
	point[2].x=ekranX;  point[2].y=ekranY;
	point[3].x=0;  point[3].y=ekranY;	
	SelectObject(hDC2, exlol);
	SelectObject(hDC2, penB);	 
	SelectObject( hDC2, bmp );
	GetClientRect(hWnd, &W_Client);	 
	Polygon(hDC2,point,4);	
	for (int B=0;B<numgen;B++)	 
		for (int a=0; a<NP; a++)//x
			for (int b=0; b<NP; b++)//y
			{  
				int i = a*NP+b;
				bool dalbool=false;
				//разрешение на отрисовку, будет действительным, если полигон развёрнут к камере.
				bool razresh=false;
				//если это поверхность пола, то она всегда отрисовывается
				razresh=true;
				//проверка на дальность полигона от камеры
				auto& o = obj[objposl[B]];
				auto& polygon = o->l[o->posl[i]];
				double dalnost = polygon.a->dlina();
				if (dalnost < 4.75)
					dalbool = true;
				//проверка 2
				if (abs(polygon.a->x * 0.9) > abs(polygon.a->z))
					dalbool=false;
				if (abs(polygon.a->y * 1.3) > abs(polygon.a->z))
					dalbool=false;
				if(obj[objposl[B]]->visible)
					if (polygon.a->z > 0.1 && dalbool && razresh)
					{
						double jarkost = 1.5 * dalnost; 
						if (jarkost < 1.35)
							jarkost = 1.35;
						jarkost = jarkost * (jarkost / 2);
						double cRed=((double)(o->Red[i]) )  /(jarkost/redK);
						double cGreen=((double)(o->Green[i])/(jarkost/greenK));
						double cBlue=((double)(o->Blue[i]) )/(jarkost/blueK);
						//устранение некоректных цветов
						if (cRed>255) cRed=255;
						if (cGreen>255) cGreen=255;
						if (cBlue>255) cBlue=255;
						//слишком тёмные полигоны отрисовываться не будут!
						if ( (cRed+cGreen+cBlue)/4>6*(redK+greenK+blueK)/3)
						{
							//  присваивание
							point[0].x= ((polygon.a->x)/polygon.a ->z * prib  + ekranX/2);   
							point[0].y= ((polygon.a->y)/polygon.a ->z * prib  + ekranY/2);
							point[1].x= ((polygon.b->x)/polygon.b ->z * prib  + ekranX/2);
							point[1].y= ((polygon.b->y)/polygon.b ->z * prib  + ekranY/2);
							point[2].x= ((polygon.c->x)/polygon.c ->z * prib  + ekranX/2); 
							point[2].y= ((polygon.c->y)/polygon.c ->z * prib  + ekranY/2);
							point[3].x= ((polygon.d->x)/polygon.d ->z * prib  + ekranX/2);
							point[3].y= ((polygon.d->y)/polygon.d ->z * prib  + ekranY/2);	
							exlol = CreateSolidBrush(RGB(cRed ,cGreen ,cBlue ));
							SelectObject(hDC2, exlol);
							Polygon(hDC2,point,4);	
							DeleteObject(exlol);  	 
						}
					}	
			} 

	//нормальные края
	//vverh
	point[0].x=0;  point[0].y=0;
	point[1].x=ekranX;  point[1].y=0 ;
	point[2].x=ekranX;  point[2].y=ekranY/12;
	point[3].x=0;  point[3].y=ekranY/12;	
	SelectObject(hDC2, brushB);
	SelectObject(hDC2, penA);	 
	SelectObject( hDC2, bmp );
	GetClientRect(hWnd, &W_Client);	 
	Polygon(hDC2,point,4);	
	//niz
	point[0].x=0;  point[0].y=ekranY-ekranY/12;
	point[1].x=ekranX;  point[1].y=ekranY-ekranY/12 ;
	point[2].x=ekranX;  point[2].y=ekranY;
	point[3].x=0;  point[3].y=ekranY ;		
	SelectObject(hDC2, brushB);
	SelectObject(hDC2, penA);	 
	SelectObject( hDC2, bmp );
	GetClientRect(hWnd, &W_Client);	 
	Polygon(hDC2,point,4);	
	//levo
	point[0].x=0;  point[0].y=0;
	point[1].x=ekranX/12;  point[1].y=0;
	point[2].x=ekranX/12;  point[2].y=ekranY;
	point[3].x=0;  point[3].y=ekranY ;		
	SelectObject(hDC2, brushB);
	SelectObject(hDC2, penA);	 
	SelectObject( hDC2, bmp );
	GetClientRect(hWnd, &W_Client);	 
	Polygon(hDC2,point,4);	
	//pravo
	point[0].x=ekranX-ekranX/12;  point[0].y=0;
	point[1].x=ekranX;  point[1].y=0;
	point[2].x=ekranX;  point[2].y=ekranY;
	point[3].x=ekranX-ekranX/12;  point[3].y=ekranY ;		
	SelectObject(hDC2, brushB);
	SelectObject(hDC2, penA);	 
	SelectObject( hDC2, bmp );
	GetClientRect(hWnd, &W_Client);	 
	Polygon(hDC2,point,4);	
	//хп бар
	for (int i = 0 ; i<HP ; i++)
	{
	exlol = CreateSolidBrush(RGB(255,0,0));
	point[0].x=ekranX/9+i*ekranX/6;  point[0].y=ekranY/15;
	point[1].x=ekranX/9+ekranX/9+i*ekranX/6;  point[1].y=ekranY/15 ;
	point[2].x=ekranX/9+ekranX/9+i*ekranX/6;  point[2].y=ekranY/15+ekranY/15;
	point[3].x=ekranX/9+i*ekranX/6;  point[3].y=ekranY/15+ekranY/15 ;		
	SelectObject(hDC2, exlol);
	SelectObject(hDC2, penA);	 
	SelectObject( hDC2, bmp );
	GetClientRect(hWnd, &W_Client);	 
	Polygon(hDC2,point,4);	
	}
	//отрисовка буфера
	hDC=BeginPaint(hWnd, &ps);
	hDC = GetDC(hWnd); 
	GetClientRect(hWnd, &W_Client);
	BitBlt(hDC,(ekranX-ekranX)/2 + ekranX/2*mast,(ekranY-ekranY)/2 + ekranY/2*mast, W_Client.right-ekranY*mast, W_Client.bottom-ekranY*mast, hDC2, ekranX/2*mast,ekranY/2*mast, SRCCOPY); // копируем буфер на экран
	ReleaseDC(hWnd, hDC);
	ReleaseDC(hWnd, hDC2);
	drawscreen=1;
 }			
}
/*
 ▄▄▄▄▄▄▄▄▄▄▄  ▄▄▄▄▄▄▄▄▄▄▄  ▄▄▄▄▄▄▄▄▄▄▄  ▄▄▄▄▄▄▄▄▄▄▄  ▄▄▄▄▄▄▄▄▄▄▄  ▄▄▄▄▄▄▄▄▄▄▄       ▄▄       ▄▄  ▄▄▄▄▄▄▄▄▄▄▄  ▄▄▄▄▄▄▄▄▄▄▄ 
▐░░░░░░░░░░░▌▐░░░░░░░░░░░▌▐░░░░░░░░░░░▌▐░░░░░░░░░░░▌▐░░░░░░░░░░░▌▐░░░░░░░░░░░▌     ▐░░▌     ▐░░▌▐░░░░░░░░░░░▌▐░░░░░░░░░░░▌
▐░█▀▀▀▀▀▀▀▀▀ ▐░█▀▀▀▀▀▀▀█░▌▐░█▀▀▀▀▀▀▀▀▀ ▐░█▀▀▀▀▀▀▀█░▌ ▀▀▀▀█░█▀▀▀▀ ▐░█▀▀▀▀▀▀▀▀▀      ▐░▌░▌   ▐░▐░▌▐░█▀▀▀▀▀▀▀█░▌▐░█▀▀▀▀▀▀▀█░▌
▐░▌          ▐░▌       ▐░▌▐░▌          ▐░▌       ▐░▌     ▐░▌     ▐░▌               ▐░▌▐░▌ ▐░▌▐░▌▐░▌       ▐░▌▐░▌       ▐░▌
▐░▌          ▐░█▄▄▄▄▄▄▄█░▌▐░█▄▄▄▄▄▄▄▄▄ ▐░█▄▄▄▄▄▄▄█░▌     ▐░▌     ▐░█▄▄▄▄▄▄▄▄▄      ▐░▌ ▐░▐░▌ ▐░▌▐░█▄▄▄▄▄▄▄█░▌▐░█▄▄▄▄▄▄▄█░▌
▐░▌          ▐░░░░░░░░░░░▌▐░░░░░░░░░░░▌▐░░░░░░░░░░░▌     ▐░▌     ▐░░░░░░░░░░░▌     ▐░▌  ▐░▌  ▐░▌▐░░░░░░░░░░░▌▐░░░░░░░░░░░▌
▐░▌          ▐░█▀▀▀▀█░█▀▀ ▐░█▀▀▀▀▀▀▀▀▀ ▐░█▀▀▀▀▀▀▀█░▌     ▐░▌     ▐░█▀▀▀▀▀▀▀▀▀      ▐░▌   ▀   ▐░▌▐░█▀▀▀▀▀▀▀█░▌▐░█▀▀▀▀▀▀▀▀▀ 
▐░▌          ▐░▌     ▐░▌  ▐░▌          ▐░▌       ▐░▌     ▐░▌     ▐░▌               ▐░▌       ▐░▌▐░▌       ▐░▌▐░▌          
▐░█▄▄▄▄▄▄▄▄▄ ▐░▌      ▐░▌ ▐░█▄▄▄▄▄▄▄▄▄ ▐░▌       ▐░▌     ▐░▌     ▐░█▄▄▄▄▄▄▄▄▄      ▐░▌       ▐░▌▐░▌       ▐░▌▐░▌          
▐░░░░░░░░░░░▌▐░▌       ▐░▌▐░░░░░░░░░░░▌▐░▌       ▐░▌     ▐░▌     ▐░░░░░░░░░░░▌     ▐░▌       ▐░▌▐░▌       ▐░▌▐░▌          
 ▀▀▀▀▀▀▀▀▀▀▀  ▀         ▀  ▀▀▀▀▀▀▀▀▀▀▀  ▀         ▀       ▀       ▀▀▀▀▀▀▀▀▀▀▀       ▀         ▀  ▀         ▀  ▀           
                                                                                                                         
*/
void createmap()
{
	double vis = -0.5;
	int kmax=1;
	for (int i=0;i<H;i++)
		for (int j=0;j<W;j++)
			for (int k=0;k<kmax/*pole[i][j]*/;k++)
			{
				if (pole[i][j]!=0)
				{
					if (i > 0 && (pole[i-1][j]==0 || pole[i-1][j]<pole[i][j]))
					{
						obj[numgen].reset(new object(i,j,-k+vis,1,100));
						obj[numgen]->init(pole[i][j]);
						numgen++; 
					}
					if (i < H - 1 && (pole[i+1][j]==0 || pole[i+1][j]<pole[i][j]))
					{		 
						obj[numgen].reset(new object(i+1,j,-k+vis,1,100));
						obj[numgen]->init(pole[i][j]);
						numgen++; 
					}
					if (j > 0 && (pole[i][j-1]==0  || pole[i][j-1]<pole[i][j]))
					{
						obj[numgen].reset(new object(i,j,-k+vis,0,100));
						 
						obj[numgen]->init(pole[i][j]);
						numgen++; 
					}
					if (j < W - 1 && (pole[i][j+1]==0 || pole[i ][j +1]<pole[i][j]))
					{
						obj[numgen].reset(new object(i,j+1,-k+vis,0,100));
						obj[numgen]->init(pole[i][j]);
						numgen++;
					}
				}
			}
			for (int i=0;i<H;i++)
				for (int j=0;j<W;j++)
					if (pole[i][j]==0)
					{
						if (poleCeiling[i][j] != 0)
						{
							obj[numgen].reset(new object(i,j,-(kmax-1)+vis ,2,100));
							obj[numgen]->init(poleCeiling[i][j]);
							numgen++; 
						}
						else
						{
							//красивая дырка в потолке
                            obj[numgen].reset(new object(i,j,-(kmax)+vis ,1,100));
							obj[numgen]->init(3);
							numgen++; 
							obj[numgen].reset(new object(i+1,j,-(kmax)+vis ,1,100));
							obj[numgen]->init(3);
							numgen++; 
							obj[numgen].reset(new object(i,j,-(kmax)+vis ,0,100));
							obj[numgen]->init(3);
							numgen++; 
							obj[numgen].reset(new object(i,j+1,-(kmax)+vis ,0,100));
							obj[numgen]->init(3);
							numgen++; 
							obj[numgen].reset(new object(i,j,-(kmax)+vis ,2,100));
							obj[numgen]->init(3);
							numgen++; 
						}
						if (poleFloor[i][j] != 0)
						{
							obj[numgen].reset(new object(i,j,1+vis,2,100));
							obj[numgen]->init(poleFloor[i][j]);
							numgen++; 
						}
					}
			//двери
			for (int i=1;i<H-1;i++)
				for (int j=1;j<W-1;j++)
				{
					if (poleO[i][j]>=1)
					{
						//правильный поворот двери
						if ((pole[i][j-1]>0 || poleO[i][j-1]>0) && (pole[i][j+1]>0 || poleO[i][j+1]>0))
						{
							obj[numgen].reset(new object(i+0.9,j,0+vis,1,100));
							obj[numgen]->init(poleO[i][j]);
							poleOvisible[i][j].num[0]=numgen;
							numgen++; 
							obj[numgen].reset(new object(i+0.1,j,0+vis,1,100));
							poleOvisible[i][j].num[1]=numgen;
							obj[numgen]->init(poleO[i][j]);
							numgen++; 
							if(kmax>1)
							{
								obj[numgen].reset(new object(i+0.9,j,-1+vis,1,100));
								obj[numgen]->init(poleO[i][j]);
								poleOvisible[i][j].num[2]=numgen;
								numgen++; 
								obj[numgen].reset(new object(i+0.1,j,-1+vis,1,100));
								poleOvisible[i][j].num[3]=numgen;
								obj[numgen]->init(poleO[i][j]);
								numgen++; 


								obj[numgen].reset(new object(i+0.1,j,0+vis,3,100));
								obj[numgen]->init(poleO[i][j]);
								poleOvisible[i][j].num[4]=numgen;
								numgen++; 
							}
							obj[numgen].reset(new object(i+0.1,j,0+vis,3,100));
							poleOvisible[i][j].num[5]=numgen;
							obj[numgen]->init(poleO[i][j]);
							numgen++; 
							//если высота больше 2х, то над дверьми идёт генерация стен
							if(kmax>2)
							{
								obj[numgen].reset(new object(i,j,-1+vis,2,100));
								obj[numgen]->init(pole[i][j-1]);
								numgen++;
								for(int uu =2;uu<kmax;uu++)
								{
									obj[numgen].reset(new object(i,j,-uu+vis,1,100));
									obj[numgen]->init(pole[i][j-1]);
									numgen++;
									obj[numgen].reset(new object(i+1,j,-uu+vis,1,100));
									obj[numgen]->init(pole[i][j-1]);
									numgen++;
								}
							}
						}
						else
						{
							obj[numgen].reset(new object(i,j+0.9,0+vis,0,100));
							obj[numgen]->init(poleO[i][j]);
							poleOvisible[i][j].num[0]=numgen;
							numgen++; 
							obj[numgen].reset(new object(i,j+0.1,0+vis,0,100));
							obj[numgen]->init(poleO[i][j]);
							poleOvisible[i][j].num[1]=numgen;
							numgen++; 

							obj[numgen].reset(new object(i,j+0.1,0+vis,4,100));
							poleOvisible[i][j].num[5]=numgen;
							obj[numgen]->init(poleO[i][j]);
							numgen++; 
							if(kmax>1)
							{

							obj[numgen].reset(new object(i,j+0.9,-1+vis,0,100));
							obj[numgen]->init(poleO[i][j]);
							poleOvisible[i][j].num[2]=numgen;
							numgen++; 

							obj[numgen].reset(new object(i,j+0.1,-1+vis,0,100));
							obj[numgen]->init(poleO[i][j]);
							poleOvisible[i][j].num[3]=numgen;
							numgen++; 

							obj[numgen].reset(new object(i,j+0.1,0+vis,4,100));
							obj[numgen]->init(poleO[i][j]);
							poleOvisible[i][j].num[4]=numgen;
							numgen++; 
									
							}
							//если высота больше 2х, то над дверьми идёт генерация стен
							if(kmax>2)
							{
								obj[numgen].reset(new object(i,j,-1+vis,2,100));
								obj[numgen]->init(pole[i-1][j]);
								numgen++;
								for(int uu =2;uu<kmax;uu++)
								{
									obj[numgen].reset(new object(i,j,-uu+vis,0,100));
									obj[numgen]->init(pole[i-1][j]);
									numgen++;
									obj[numgen].reset(new object(i,j+1,-uu+vis,0,100));
									obj[numgen]->init(pole[i-1][j]);
									numgen++;
								}
							}
						}				   
					}
				}
		//объекты
		for (int i=0;i<H;i++)
			for (int j=0;j<W;j++)
			{
				if (poleObj[i][j]>0)
				{
					if (!(poleObj[i][j]==5 || poleObj[i][j]==6)) 
						if (poleCeiling[i][j] != 0)
						{
					      obj[numgen].reset(new object(i+0.5,j,0.0+vis-kmax+1,1,100));
						  obj[numgen]->init(poleObj[i][j]);
						  numgen++; 
						}
						else//если подвисной объект расположен в дырке,то продублировать его 
						{
						  obj[numgen].reset(new object(i+0.5,j,0.0+vis-kmax,1,100));
						  obj[numgen]->init(poleObj[i][j]);
					      numgen++; 
						  obj[numgen].reset(new object(i+0.5,j,0.0+vis-kmax+0.4,1,100));
						  obj[numgen]->init(poleObj[i][j]);
					      numgen++;
						}
					else
					{
					obj[numgen].reset(new object(i+0.5,j,0.0+vis,1,100));
					obj[numgen]->init(poleObj[i][j]);
					numgen++; 
					}
					 
				}
			}
	//последовательность
	for (int i =0;i<numgen;i++) 
		objposl[i]=i;
}
/*
 ▄            ▄▄▄▄▄▄▄▄▄▄▄  ▄▄▄▄▄▄▄▄▄▄▄  ▄▄▄▄▄▄▄▄▄▄        ▄▄       ▄▄  ▄▄▄▄▄▄▄▄▄▄▄  ▄▄▄▄▄▄▄▄▄▄▄ 
▐░▌          ▐░░░░░░░░░░░▌▐░░░░░░░░░░░▌▐░░░░░░░░░░▌      ▐░░▌     ▐░░▌▐░░░░░░░░░░░▌▐░░░░░░░░░░░▌
▐░▌          ▐░█▀▀▀▀▀▀▀█░▌▐░█▀▀▀▀▀▀▀█░▌▐░█▀▀▀▀▀▀▀█░▌     ▐░▌░▌   ▐░▐░▌▐░█▀▀▀▀▀▀▀█░▌▐░█▀▀▀▀▀▀▀█░▌
▐░▌          ▐░▌       ▐░▌▐░▌       ▐░▌▐░▌       ▐░▌     ▐░▌▐░▌ ▐░▌▐░▌▐░▌       ▐░▌▐░▌       ▐░▌
▐░▌          ▐░▌       ▐░▌▐░█▄▄▄▄▄▄▄█░▌▐░▌       ▐░▌     ▐░▌ ▐░▐░▌ ▐░▌▐░█▄▄▄▄▄▄▄█░▌▐░█▄▄▄▄▄▄▄█░▌
▐░▌          ▐░▌       ▐░▌▐░░░░░░░░░░░▌▐░▌       ▐░▌     ▐░▌  ▐░▌  ▐░▌▐░░░░░░░░░░░▌▐░░░░░░░░░░░▌
▐░▌          ▐░▌       ▐░▌▐░█▀▀▀▀▀▀▀█░▌▐░▌       ▐░▌     ▐░▌   ▀   ▐░▌▐░█▀▀▀▀▀▀▀█░▌▐░█▀▀▀▀▀▀▀▀▀ 
▐░▌          ▐░▌       ▐░▌▐░▌       ▐░▌▐░▌       ▐░▌     ▐░▌       ▐░▌▐░▌       ▐░▌▐░▌          
▐░█▄▄▄▄▄▄▄▄▄ ▐░█▄▄▄▄▄▄▄█░▌▐░▌       ▐░▌▐░█▄▄▄▄▄▄▄█░▌     ▐░▌       ▐░▌▐░▌       ▐░▌▐░▌          
▐░░░░░░░░░░░▌▐░░░░░░░░░░░▌▐░▌       ▐░▌▐░░░░░░░░░░▌      ▐░▌       ▐░▌▐░▌       ▐░▌▐░▌          
 ▀▀▀▀▀▀▀▀▀▀▀  ▀▀▀▀▀▀▀▀▀▀▀  ▀         ▀  ▀▀▀▀▀▀▀▀▀▀        ▀         ▀  ▀         ▀  ▀           
*/
void LoadMap(string namefile)
{	
	for (int i = 0; i < H; i++)
		for (int j = 0; j < W; j++)
		{
			pole[i][j] = 0;
			poleO[i][j] = 0;
			poleObj[i][j] = 0; 
			poleFloor[i][j] = 0;
			poleCeiling[i][j] = 0;
			for (int k = 0; k < 6; k++)
				poleOvisible[i][j].num[k] = 0;
		}
	string a("/map.bmp");
	string namefileTo(namefile);
	namefileTo += a;
	f1 = fopen(namefileTo.c_str(), "r+b");
	fread(&bfh_l,sizeof(bfh_l),1,f1);               //Запихиваем файловый заголовок в структуру BITMAPFILEHEADER
	fread(&bih_l,sizeof(bih_l),1,f1);               //Запихиваем заголовок изображения в структуру BITMAPINFOHEADER
	int padding = 0;
	for(int i=0;i < H;i++)
	{
		for (int j = 0; j < W; j++)
		{
			fread(&rgb_l, sizeof(rgb_l),1, f1);			  
			if (i*N+j<=N*N)
			{  	
				if (rgb_l.rgbtRed == 255 && rgb_l.rgbtGreen == 0 && rgb_l.rgbtBlue == 0)
					pole[H-i-1][j] = 1;   
				if (rgb_l.rgbtRed == 200 && rgb_l.rgbtGreen == 200 && rgb_l.rgbtBlue == 200)
					pole[H-i-1][j] = 3;   
			}    
		}
		if(padding != 0) 
		{
			fread(&rgb_l, padding,1, f1); 
		}
	}
	fcloseall();
	a="/objects.bmp";
	namefileTo=namefile;
	namefileTo += a;
	f1 = fopen(namefileTo.c_str(), "r+b");
	fread(&bfh_l,sizeof(bfh_l),1,f1);               //Запихиваем файловый заголовок в структуру BITMAPFILEHEADER
	fread(&bih_l,sizeof(bih_l),1,f1);               //Запихиваем заголовок изображения в структуру BITMAPINFOHEADER
	padding = 0;
	if ((bih_l.biWidth * 3) % 4) 
		padding = 4 - (bih_l.biWidth * 3) % 4;
	for (int i=0; i < H; i++)
	{
		for (int j =  0; j < W; j++)
		{
			fread(&rgb_l, sizeof(rgb_l),1, f1);			  
			if (i*N+j<=N*N)
			{  	
				if (rgb_l.rgbtRed == 127 && rgb_l.rgbtGreen == 0 && rgb_l.rgbtBlue == 0)
					poleObj[H-i-1][j] = 5;   
				if (rgb_l.rgbtRed == 0 && rgb_l.rgbtGreen == 127 && rgb_l.rgbtBlue == 0)
					poleObj[H-i-1][j] = 6;   
				if (rgb_l.rgbtRed == 64 && rgb_l.rgbtGreen == 64 && rgb_l.rgbtBlue == 64)
					poleObj[H-i-1][j] = 7;     
				if (rgb_l.rgbtRed == 127 && rgb_l.rgbtGreen == 127 && rgb_l.rgbtBlue == 0)
					poleObj[H-i-1][j] = 9;     
			}    
		}
		if(padding != 0) 
		{
			fread(&rgb_l, padding,1, f1); 
		}
	}
	fcloseall();
	a="/doors.bmp";
	namefileTo=namefile;
	namefileTo += a;
	f1 = fopen(namefileTo.c_str(), "r+b");
	fread(&bfh_l,sizeof(bfh_l),1,f1);               //Запихиваем файловый заголовок в структуру BITMAPFILEHEADER
	fread(&bih_l,sizeof(bih_l),1,f1);               //Запихиваем заголовок изображения в структуру BITMAPINFOHEADER
	padding = 0;
	if ((bih_l.biWidth * 3) % 4) 
		padding = 4 - (bih_l.biWidth * 3) % 4;
	for(int i=0; i < H;i++)
	{
		for (int j = 0; j <W; j++)
		{
			fread(&rgb_l, sizeof(rgb_l),1, f1);			  
			if (i*N+j<=N*N)
			{  	
				if (rgb_l.rgbtRed == 0 && rgb_l.rgbtGreen == 255 && rgb_l.rgbtBlue == 0)
					poleO[H-i-1][j] = 4;   
			}    
		}
		if(padding != 0) 
		{
			fread(&rgb_l, padding,1, f1); 
		}
	}
	fcloseall();
	a="/floormap.bmp";
	namefileTo=namefile;
	namefileTo += a;
	f1 = fopen(namefileTo.c_str(), "r+b");
	fread(&bfh_l,sizeof(bfh_l),1,f1);               //Запихиваем файловый заголовок в структуру BITMAPFILEHEADER
	fread(&bih_l,sizeof(bih_l),1,f1);               //Запихиваем заголовок изображения в структуру BITMAPINFOHEADER
	padding = 0;
	if ((bih_l.biWidth * 3) % 4) 
		padding = 4 - (bih_l.biWidth * 3) % 4;
	for(int i=0;i< H;i++)
	{
		for (int j =  0; j <W; j++)
		{
			fread(&rgb_l, sizeof(rgb_l),1, f1);			  
			if (i*N+j<=N*N)
			{  	
				if (rgb_l.rgbtRed == 255 && rgb_l.rgbtGreen == 0 && rgb_l.rgbtBlue == 0)
					poleFloor[H-i-1][j] = 1;   
				if (rgb_l.rgbtRed == 127 && rgb_l.rgbtGreen == 127 && rgb_l.rgbtBlue == 127)
					poleFloor[H-i-1][j] = 2;   
				if (rgb_l.rgbtRed == 200 && rgb_l.rgbtGreen == 200 && rgb_l.rgbtBlue == 200)
					poleFloor[H-i-1][j] = 3;   
				if (rgb_l.rgbtRed == 64 && rgb_l.rgbtGreen == 255 && rgb_l.rgbtBlue == 64)
					poleFloor[H-i-1][j] = 8;   
			}    
		}
		if(padding != 0) 
		{
			fread(&rgb_l, padding,1, f1); 
		}
	}
	fcloseall();
	a ="/ceilingmap.bmp";
	namefileTo =namefile;
	namefileTo += a;  
	f1 = fopen(namefileTo.c_str(), "r+b");
	fread(&bfh_l,sizeof(bfh_l),1,f1);               //Запихиваем файловый заголовок в структуру BITMAPFILEHEADER
	fread(&bih_l,sizeof(bih_l),1,f1);               //Запихиваем заголовок изображения в структуру BITMAPINFOHEADER
	padding = 0;
	if ((bih_l.biWidth * 3) % 4) 
		padding = 4 - (bih_l.biWidth * 3) % 4;
	for(int i=0;i< H;i++)
	{
		for (int j =  0; j <W; j++)
		{
			fread(&rgb_l, sizeof(rgb_l),1, f1);			  
			if (i*N+j<=N*N)
			{  	
				if (rgb_l.rgbtRed == 255 && rgb_l.rgbtGreen == 0 && rgb_l.rgbtBlue == 0)
					poleCeiling[H-i-1][j] = 1;   
				if (rgb_l.rgbtRed == 127 && rgb_l.rgbtGreen == 127 && rgb_l.rgbtBlue == 127)
					poleCeiling[H-i-1][j] = 2;   
				if (rgb_l.rgbtRed == 200 && rgb_l.rgbtGreen == 200 && rgb_l.rgbtBlue == 200)
					poleCeiling[H-i-1][j] = 3;   
				if (rgb_l.rgbtRed == 64 && rgb_l.rgbtGreen == 64 && rgb_l.rgbtBlue == 64)
					poleCeiling[H-i-1][j] = 7;   
			}    
		}
		if(padding != 0) 
		{
			fread(&rgb_l, padding,1, f1); 
		}
	}
	fcloseall();
}
/*
 ▄▄        ▄  ▄▄▄▄▄▄▄▄▄▄▄  ▄         ▄       ▄▄       ▄▄  ▄▄▄▄▄▄▄▄▄▄▄  ▄▄▄▄▄▄▄▄▄▄▄ 
▐░░▌      ▐░▌▐░░░░░░░░░░░▌▐░▌       ▐░▌     ▐░░▌     ▐░░▌▐░░░░░░░░░░░▌▐░░░░░░░░░░░▌
▐░▌░▌     ▐░▌▐░█▀▀▀▀▀▀▀▀▀ ▐░▌       ▐░▌     ▐░▌░▌   ▐░▐░▌▐░█▀▀▀▀▀▀▀█░▌▐░█▀▀▀▀▀▀▀█░▌
▐░▌▐░▌    ▐░▌▐░▌          ▐░▌       ▐░▌     ▐░▌▐░▌ ▐░▌▐░▌▐░▌       ▐░▌▐░▌       ▐░▌
▐░▌ ▐░▌   ▐░▌▐░█▄▄▄▄▄▄▄▄▄ ▐░▌   ▄   ▐░▌     ▐░▌ ▐░▐░▌ ▐░▌▐░█▄▄▄▄▄▄▄█░▌▐░█▄▄▄▄▄▄▄█░▌
▐░▌  ▐░▌  ▐░▌▐░░░░░░░░░░░▌▐░▌  ▐░▌  ▐░▌     ▐░▌  ▐░▌  ▐░▌▐░░░░░░░░░░░▌▐░░░░░░░░░░░▌
▐░▌   ▐░▌ ▐░▌▐░█▀▀▀▀▀▀▀▀▀ ▐░▌ ▐░▌░▌ ▐░▌     ▐░▌   ▀   ▐░▌▐░█▀▀▀▀▀▀▀█░▌▐░█▀▀▀▀▀▀▀▀▀ 
▐░▌    ▐░▌▐░▌▐░▌          ▐░▌▐░▌ ▐░▌▐░▌     ▐░▌       ▐░▌▐░▌       ▐░▌▐░▌          
▐░▌     ▐░▐░▌▐░█▄▄▄▄▄▄▄▄▄ ▐░▌░▌   ▐░▐░▌     ▐░▌       ▐░▌▐░▌       ▐░▌▐░▌          
▐░▌      ▐░░▌▐░░░░░░░░░░░▌▐░░▌     ▐░░▌     ▐░▌       ▐░▌▐░▌       ▐░▌▐░▌          
 ▀        ▀▀  ▀▀▀▀▀▀▀▀▀▀▀  ▀▀       ▀▀       ▀         ▀  ▀         ▀  ▀         
*/
void newmap(string name,double x,double z,double newX,double newZ,int prewcount,int count)
{
 if ( MapCounter==prewcount && cZ>z-1 && cZ<z+1 && cX>x-1 && cX<x+1)
  {  
//стандартный поворот
   for (int B=0;B<numgen;B++)
	for (int i=0; i<N*N; i++)
	{
		turn(obj[objposl[B]]->p[i].z, obj[objposl[B]]->p[i].x,-cA);
		turn(obj[objposl[B]]->p[i].y, obj[objposl[B]]->p[i].z,-cAz);			
	} 
	MapCounter=count;
	cA=0;
	cZ=newZ;
	cX=newX; 
	cAz=0;			 
	numgen=0;			 
	LoadMap(name);		 
	createmap();
  }
}
/*
 ▄▄       ▄▄  ▄▄▄▄▄▄▄▄▄▄▄  ▄▄▄▄▄▄▄▄▄▄▄  ▄▄        ▄ 
▐░░▌     ▐░░▌▐░░░░░░░░░░░▌▐░░░░░░░░░░░▌▐░░▌      ▐░▌
▐░▌░▌   ▐░▐░▌▐░█▀▀▀▀▀▀▀█░▌ ▀▀▀▀█░█▀▀▀▀ ▐░▌░▌     ▐░▌
▐░▌▐░▌ ▐░▌▐░▌▐░▌       ▐░▌     ▐░▌     ▐░▌▐░▌    ▐░▌
▐░▌ ▐░▐░▌ ▐░▌▐░█▄▄▄▄▄▄▄█░▌     ▐░▌     ▐░▌ ▐░▌   ▐░▌
▐░▌  ▐░▌  ▐░▌▐░░░░░░░░░░░▌     ▐░▌     ▐░▌  ▐░▌  ▐░▌
▐░▌   ▀   ▐░▌▐░█▀▀▀▀▀▀▀█░▌     ▐░▌     ▐░▌   ▐░▌ ▐░▌
▐░▌       ▐░▌▐░▌       ▐░▌     ▐░▌     ▐░▌    ▐░▌▐░▌
▐░▌       ▐░▌▐░▌       ▐░▌ ▄▄▄▄█░█▄▄▄▄ ▐░▌     ▐░▐░▌
▐░▌       ▐░▌▐░▌       ▐░▌▐░░░░░░░░░░░▌▐░▌      ▐░░▌
 ▀         ▀  ▀         ▀  ▀▀▀▀▀▀▀▀▀▀▀  ▀        ▀▀                                                    
*/
int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPreInst,LPSTR lpszCmdLine, int nCmdShow)
{
	MSG            msg;
	WNDCLASSEX     wc;
	//заполняем параметры окна
	wc.cbSize =			sizeof(WNDCLASSEX);
	wc.style =				CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc =		WndProc;
	wc.cbClsExtra =		0;
	wc.cbWndExtra =		0;
	wc.hInstance =			hInst;
	wc.hIcon =				LoadIcon(NULL, /*IDI_SHIELD*/IDI_APPLICATION);
	wc.hCursor =			LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground =		(HBRUSH)GetStockObject(WHITE_BRUSH);
	wc.lpszMenuName =		NULL;
	wc.lpszClassName =		"WINCLASS1";
	wc.hIconSm =			LoadIcon(NULL, IDI_APPLICATION);
	//Регистрация нового класса
	RegisterClassEx(&wc);
	hDC	=	GetDC(0);
	// определение размера экрана 
	ekranX =	GetDeviceCaps(hDC,HORZRES)*1.2;
	ekranY =	GetDeviceCaps(hDC,VERTRES)*1.2; 
	prib=(float)ekranX/2; 
	//создание окна приложения
	hWnd = CreateWindowEx(
		0,
		"WINCLASS1",
		"game",
		WS_VISIBLE | WS_POPUP ,
		GetDeviceCaps(hDC,HORZRES)/2-ekranX/2, GetDeviceCaps(hDC,VERTRES)/2-ekranY/2,     
		ekranX  ,  ekranY ,
		0,
		0,
		hInst,
		0
		);
	//получаем размер клиентской области окна
	GetClientRect(hWnd, &W_Client);
	// заливаем фон цветом       
	FillRect(hDC, &W_Client, brushB); 
	//цикл обработки сообщений
	while(GetMessage(&msg, NULL, 0, 0))
	{
		TranslateMessage(&msg);
		//почему-то правильно работает, если только newX=newZ :C	
		newmap("map2",14,16,1.66,1.66,0,1); 
		newmap("map1",0,2.5,15.3,15.1,1,0);//возврат из второй в первую
		newmap("map3",16,14,2,2,1,2); 
		newmap("map2",0,1,14,14,2,1);//возврат из третьей во вторую	  
		DispatchMessage(&msg);
	}
	//выход с возврщением msg.wParam
	return (msg.wParam);
}
LRESULT CALLBACK WndProc(HWND hWnd, UINT nMsg, WPARAM wParam,
	LPARAM lParam)
{
	//обработка сообщений
	switch(nMsg)
	{ 
	case WM_CREATE:  
		{            
			randomize();
			SetTimer(hWnd, 1, 10, NULL);
			hDC = GetDC(hWnd);
			hDC2 = CreateCompatibleDC( hDC );
			bmp = CreateCompatibleBitmap( hDC, ekranX, ekranY );		 
			GetDeviceCaps(hDC,DT_DISPFILE);
			int penWidth=ekranX/222;
			if (penWidth<2)
				penWidth=2;
			if (penWidth>6)
				penWidth=6;
			penA =  CreatePen(5,penWidth, RGB(0, 0, 0));
			// прозрачная линая, т.к первый параметр=5 . Нужна для красивых краев полигонов
			penB =  CreatePen(5,1, RGB(0, 0, 0));
			brushA = CreateSolidBrush(RGB(70,70,70));
			brushB = CreateSolidBrush( RGB(55,25,0));
			brushC = CreateSolidBrush(RGB(0,255,0));
			//создаёт карту		  
			LoadMap("map1");
			createmap();		 
		}
		break;
	case WM_TIMER :
		
	   if (mouseRight)
	   {
		   	if (flymode || poleO[(int)(cX + 0.75*cos(cA ))][(int)(cZ + 0.75*sin(cA))]<=0) 
				if (flymode || pole[(int)(cX + 0.75*cos(cA ))][(int)(cZ + 0.75*sin(cA))]<=0) 
				{
					for (int B=0;B<numgen;B++)
					{
						for (int i=0; i<N*N; i++)
						{
							obj[objposl[B]]->p[i].z=obj[objposl[B]]->p[i].z-0.05*cos(cAz);
							obj[objposl[B]]->p[i].y=obj[objposl[B]]->p[i].y-0.05*sin(cAz);
						}
					}
					cX+=0.05*cos(cA);
					cZ+=0.05*sin(cA);
				}	
	   }
	   if (mousemove && !mouseRight)
	   {		
		for (int B=0;B<numgen;B++)
			{
				if (obj[objposl[B]]->turning)
					for (int i=0; i<N*N; i++)
					{
						turn(obj[objposl[B]]->p[i].y, obj[objposl[B]]->p[i].z,-cAz);
						turn(obj[objposl[B]]->p[i].z, obj[objposl[B]]->p[i].x,xPos);
						turn(obj[objposl[B]]->p[i].y, obj[objposl[B]]->p[i].z,cAz);
					}
			}
		// по вертикали
		if (cAz>-50*PI/180 && cAz<50*PI/180) 
			{
				for (int B=0;B<numgen;B++)
					for (int i=0; i<N*N; i++)
						turn(obj[objposl[B]]->p[i].y, obj[objposl[B]]->p[i].z,yPos); 
				cAz+=yPos ;
			}
		else//самый лучший алгоритм (необходим для того, чтобы камера не застревала
          {
			  if (cAz<-45*PI/180)
			  {
				for (int B=0;B<numgen;B++)
					for (int i=0; i<N*N; i++)
					{
						turn(obj[objposl[B]]->p[i].y, obj[objposl[B]]->p[i].z,0.003);  
					}
				 cAz+=0.003 ;
			  }
			  else
			  {
				for (int B=0;B<numgen;B++)
					for (int i=0; i<N*N; i++)
					{
						turn(obj[objposl[B]]->p[i].y, obj[objposl[B]]->p[i].z,-0.003);   
					}
			     cAz-=0.003 ;
			  }
			}	
			cA+=xPos;
	    }
	    if (mouseRight + mousemove)
		{
			proverkaZobj(); 
		    screen();
		}
		break;
	case WM_RBUTTONUP :
		xPos   = LOWORD(lParam) -ekranX/2;
        yPos   = HIWORD(lParam) -ekranY/2;
		xPos/=100;
		xPos=xPos*PI/180;
		yPos/=100;
		yPos=-yPos*PI/180;
		xPos*=1000/(double)ekranX;
		yPos*=800/(double)ekranY;
		mouseRight=0;
		break;
	case WM_RBUTTONDOWN :
		xPos   = LOWORD(lParam) -ekranX/2;
        yPos   = HIWORD(lParam) -ekranY/2;
		xPos/=100;
		xPos=xPos*PI/180;
		yPos/=100;
		yPos=-yPos*PI/180;
		xPos*=1000/(double)ekranX;
		yPos*=800/(double)ekranY;
		mouseRight=1;
		break;
	case WM_LBUTTONUP :
		mousemove=0;
		xPos   = LOWORD(lParam) -ekranX/2;
        yPos   = HIWORD(lParam) -ekranY/2;
		xPos/=100;
		xPos=xPos*PI/180;
		yPos/=100;
		yPos=-yPos*PI/180;
		xPos*=1000/(double)ekranX;
		yPos*=800/(double)ekranY;
		break; 
	case WM_LBUTTONDOWN :
		mousemove=1;
		xPos   = LOWORD(lParam) -ekranX/2;
        yPos   = HIWORD(lParam) -ekranY/2;
		xPos/=100;
		xPos=xPos*PI/180;
		yPos/=100;
		yPos=-yPos*PI/180;
		xPos*=1000/(double)ekranX;
		yPos*=800/(double)ekranY;
		break;
    case WM_MOUSEMOVE:
		if (wParam==1)
		{
			xPos   = LOWORD(lParam) -ekranX/2;
			yPos   = HIWORD(lParam) -ekranY/2;
			xPos/=100;
			xPos=xPos*PI/180;
			yPos/=100;
			yPos=-yPos*PI/180;
			xPos*=1000/(double)ekranX;
			yPos*=800/(double)ekranY;
		}
		break;
	case WM_KEYDOWN:
		if (start)
		switch (wParam) 
		{	case VK_ESCAPE:
		PostMessage(hWnd,WM_CLOSE,0,0);
		break;	
		case 83 ://s
			if (flymode || poleO[(int)(cX - 0.75*cos(cA ))][(int)(cZ - 0.75*sin(cA))]<=0) 
				if (flymode || pole[(int)(cX - 0.75*cos(cA ))][(int)(cZ - 0.75*sin(cA))]<=0) 
				{
					for (int B=0;B<numgen;B++)
					{
						for (int i=0; i<N*N; i++)
						{
							obj[objposl[B]]->p[i].z=obj[objposl[B]]->p[i].z+0.35*cos(cAz);
							obj[objposl[B]]->p[i].y=obj[objposl[B]]->p[i].y+0.35*sin(cAz);
						}
					}
					cX-=0.35*cos(cA);
					cZ-=0.35*sin(cA);
					proverkaZobj();
					screen(); 
				}
				break;
		case 87 ://w
			if (flymode || poleO[(int)(cX + 0.75*cos(cA ))][(int)(cZ + 0.75*sin(cA))]<=0) 
				if (flymode || pole[(int)(cX + 0.75*cos(cA ))][(int)(cZ + 0.75*sin(cA))]<=0) 
				{
					// MessageBeep(MB_OK);
					for (int B=0;B<numgen;B++)
					{
						for (int i=0; i<N*N; i++)
						{
							obj[objposl[B]]->p[i].z=obj[objposl[B]]->p[i].z-0.35*cos(cAz);
							obj[objposl[B]]->p[i].y=obj[objposl[B]]->p[i].y-0.35*sin(cAz);
						}
						
					}
					cX+=0.35*cos(cA);
					cZ+=0.35*sin(cA);
					proverkaZobj(); 
					screen();
				}
				break;
		case VK_SPACE ://если впереди дверь и нажат пробел, то поменять состояние двери (открыто/закрыто)
			if (poleO[(int)(cX + 0.75*cos(cA ))][(int)(cZ + 0.75*sin(cA))]>=1) 
			{
			    double newcAz = cAz;
				for (int j=0;j<4;j++)
				{
					for (int B =0 ; B<numgen;B++)
					for (int i=0; i<N*N; i++)
					{
						turn(obj[objposl[B]]->p[i].y, obj[objposl[B]]->p[i].z,-newcAz );
					}
					cAz=0;
					for (int i=0; i<N*N; i++)
					{
						obj[poleOvisible[(int)(cX + 0.75*cos(cA ))][(int)(cZ + 0.75*sin(cA))].num[0]]->p[i].y+=0.30;
						obj[poleOvisible[(int)(cX + 0.75*cos(cA ))][(int)(cZ + 0.75*sin(cA))].num[1]]->p[i].y+=0.30;
						obj[poleOvisible[(int)(cX + 0.75*cos(cA ))][(int)(cZ + 0.75*sin(cA))].num[4]]->p[i].y+=0.30;


						obj[poleOvisible[(int)(cX + 0.75*cos(cA ))][(int)(cZ + 0.75*sin(cA))].num[2]]->p[i].y-=0.30;
						obj[poleOvisible[(int)(cX + 0.75*cos(cA ))][(int)(cZ + 0.75*sin(cA))].num[3]]->p[i].y-=0.30; 
						obj[poleOvisible[(int)(cX + 0.75*cos(cA ))][(int)(cZ + 0.75*sin(cA))].num[5]]->p[i].y-=0.30;

						obj[poleOvisible[(int)(cX + 0.75*cos(cA ))][(int)(cZ + 0.75*sin(cA))].num[0]]->p[i].z-=0.30*sin(cAz); 
						obj[poleOvisible[(int)(cX + 0.75*cos(cA ))][(int)(cZ + 0.75*sin(cA))].num[1]]->p[i].z-=0.30*sin(cAz);
						obj[poleOvisible[(int)(cX + 0.75*cos(cA ))][(int)(cZ + 0.75*sin(cA))].num[4]]->p[i].z-=0.30*sin(cAz);


						obj[poleOvisible[(int)(cX + 0.75*cos(cA ))][(int)(cZ + 0.75*sin(cA))].num[2]]->p[i].z+=0.30*sin(cAz);
						obj[poleOvisible[(int)(cX + 0.75*cos(cA ))][(int)(cZ + 0.75*sin(cA))].num[3]]->p[i].z+=0.30*sin(cAz);
						obj[poleOvisible[(int)(cX + 0.75*cos(cA ))][(int)(cZ + 0.75*sin(cA))].num[5]]->p[i].z+=0.30*sin(cAz); 
					}
					cAz = newcAz;
					for (int B =0 ; B<numgen;B++)
					     for (int i=0; i<N*N; i++)
					      {
						   turn(obj[objposl[B]]->p[i].y, obj[objposl[B]]->p[i].z, cAz );
						  } 
					proverkaZobj();
					screen();
				}
				for (int i=0;i<6;i++)
					obj[poleOvisible[(int)(cX + 0.75*cos(cA ))][(int)(cZ + 0.75*sin(cA))].num[i]]->visible=0;
			 
				poleO[(int)(cX + 0.75*cos(cA ))][(int)(cZ + 0.75*sin(cA))]=poleO[(int)(cX + 0.75*cos(cA ))][(int)(cZ + 0.75*sin(cA))]*-1;
			}
			else
				if ( poleO[(int)(cX + 0.75*cos(cA ))][(int)(cZ + 0.75*sin(cA))]<=-1) 
				{
					for (int i=0;i<6;i++)
						obj[poleOvisible[(int)(cX + 0.75*cos(cA ))][(int)(cZ + 0.75*sin(cA))].num[i]]->visible=1;
				    double newcAz = cAz;	 
					poleO[(int)(cX + 0.75*cos(cA ))][(int)(cZ + 0.75*sin(cA))]=poleO[(int)(cX + 0.75*cos(cA ))][(int)(cZ + 0.75*sin(cA))]*-1;
					for (int j=0;j<4;j++)
					{
						for (int B =0 ; B<numgen;B++)
					      for (int i=0; i<N*N; i++)
					    {
				  		 turn(obj[objposl[B]]->p[i].y, obj[objposl[B]]->p[i].z,-newcAz );
					    }
						cAz=0;
						for (int i=0; i<N*N; i++)
						{
							obj[poleOvisible[(int)(cX + 0.75*cos(cA ))][(int)(cZ + 0.75*sin(cA))].num[0]]->p[i].y-=0.30;
							obj[poleOvisible[(int)(cX + 0.75*cos(cA ))][(int)(cZ + 0.75*sin(cA))].num[1]]->p[i].y-=0.30;
							obj[poleOvisible[(int)(cX + 0.75*cos(cA ))][(int)(cZ + 0.75*sin(cA))].num[4]]->p[i].y-=0.30;


							obj[poleOvisible[(int)(cX + 0.75*cos(cA ))][(int)(cZ + 0.75*sin(cA))].num[2]]->p[i].y+=0.30;
							obj[poleOvisible[(int)(cX + 0.75*cos(cA ))][(int)(cZ + 0.75*sin(cA))].num[3]]->p[i].y+=0.30; 
							obj[poleOvisible[(int)(cX + 0.75*cos(cA ))][(int)(cZ + 0.75*sin(cA))].num[5]]->p[i].y+=0.30;


							obj[poleOvisible[(int)(cX + 0.75*cos(cA ))][(int)(cZ + 0.75*sin(cA))].num[0]]->p[i].z+=0.30*sin(cAz);
							obj[poleOvisible[(int)(cX + 0.75*cos(cA ))][(int)(cZ + 0.75*sin(cA))].num[1]]->p[i].z+=0.30*sin(cAz);
							obj[poleOvisible[(int)(cX + 0.75*cos(cA ))][(int)(cZ + 0.75*sin(cA))].num[4]]->p[i].z+=0.30*sin(cAz);


							obj[poleOvisible[(int)(cX + 0.75*cos(cA ))][(int)(cZ + 0.75*sin(cA))].num[2]]->p[i].z-=0.30*sin(cAz);
							obj[poleOvisible[(int)(cX + 0.75*cos(cA ))][(int)(cZ + 0.75*sin(cA))].num[3]]->p[i].z-=0.30*sin(cAz);
							obj[poleOvisible[(int)(cX + 0.75*cos(cA ))][(int)(cZ + 0.75*sin(cA))].num[5]]->p[i].z-=0.30*sin(cAz);
						}
						cAz = newcAz;
					    for (int B =0 ; B<numgen;B++)
					     for (int i=0; i<N*N; i++)
					      {
						   turn(obj[objposl[B]]->p[i].y, obj[objposl[B]]->p[i].z, cAz );
					      }
						proverkaZobj();
						screen();
					}	 
				}
				//если впереди НЕ дверь и нажат пробел, то прыгнуть)
			  if (poleO[(int)(cX + 0.75*cos(cA ))][(int)(cZ + 0.75*sin(cA))]==0)   
			  {
				for(int k=0;k<5;k++)
				{
                  for (int B =0 ; B<numgen;B++)
					     for (int i=0; i<N*N; i++)
					      {
						   obj[objposl[B]]->p[i].y+=0.03;
					      }
				 screen();
				}
				for(int k=0;k<5;k++)
				{
                  for (int B =0 ; B<numgen;B++)
					     for (int i=0; i<N*N; i++)
					      {
						   obj[objposl[B]]->p[i].y-=0.03;
					      }
				 screen();
				}
			  }
				break;
		case 65 : //a
			for (int B=0;B<numgen;B++)
			{
				if (obj[objposl[B]]->turning)
					for (int i=0; i<N*N; i++)
					{
						turn(obj[objposl[B]]->p[i].y, obj[objposl[B]]->p[i].z,-cAz);
						turn(obj[objposl[B]]->p[i].z, obj[objposl[B]]->p[i].x,-15*PI/180);
						turn(obj[objposl[B]]->p[i].y, obj[objposl[B]]->p[i].z,cAz);
					}
					
			}
			cA-=15*PI/180;
			proverkaZobj(); 
		    screen();
			break;
		case 68 : //d
			for (int B=0;B<numgen;B++)
				if (obj[objposl[B]]->turning)
					for (int i=0; i<N*N; i++)
					{
						turn(obj[objposl[B]]->p[i].y, obj[objposl[B]]->p[i].z,-cAz);
						turn(obj[objposl[B]]->p[i].z, obj[objposl[B]]->p[i].x,15*PI/180);
						turn(obj[objposl[B]]->p[i].y, obj[objposl[B]]->p[i].z,cAz);
					}
			
			proverkaZobj(); 
			cA+=15*PI/180;
			screen(); 
			break;
		case 81 : //q
			if (flymode || poleO[(int)(cX + 0.5*cos(cA-90*PI/180 ))][(int)(cZ +0.5*sin(cA-90*PI/180))]<=0) 
				if (flymode || pole[(int)(cX + 0.5*cos(cA-90*PI/180 ))][(int)(cZ +0.5*sin(cA-90*PI/180))]<=0) 
				{
					for (int B=0;B<numgen;B++)
					{
						for (int i=0; i<N*N; i++)
						{
							obj[objposl[B]]->p[i].x=obj[objposl[B]]->p[i].x+0.25;
						}
					}
					cZ+=0.25*sin(cA-90*PI/180);
					cX+=0.25*cos(cA-90*PI/180 );
					proverkaZobj(); 
					screen();
				} 
				break;
		case 69 : //e
			if (flymode || poleO[(int)(cX - 0.5*cos(cA-90*PI/180 ))][(int)(cZ - 0.5*sin(cA-90*PI/180))]<=0) 
				if (flymode || pole[(int)(cX - 0.5*cos(cA-90*PI/180 ))][(int)(cZ - 0.5*sin(cA-90*PI/180))]<=0) 
				{
					for (int B=0;B<numgen;B++)
					{
						for (int i=0; i<N*N; i++)
						{
							obj[objposl[B]]->p[i].x=obj[objposl[B]]->p[i].x-0.25;
						}
						
					}
					cZ-=0.25*sin(cA-90*PI/180);
					cX-=0.25*cos(cA-90*PI/180 );
					proverkaZobj(); 
					screen();
				}
				break;
		case 107 : //num+
			if (cAz<45*PI/180) 
			{
				for (int B=0;B<numgen;B++)
					for (int i=0; i<N*N; i++)
						turn(obj[objposl[B]]->p[i].y, obj[objposl[B]]->p[i].z,15*PI/180);
				proverkaZobj(); 
				screen();
				cAz+=15*PI/180;
			}
			break;
		case 109 : //num-
			if (cAz>-45*PI/180) 
			{
				for (int B=0;B<numgen;B++)
					for (int i=0; i<N*N; i++)
						turn(obj[objposl[B]]->p[i].y, obj[objposl[B]]->p[i].z,-15*PI/180);
				proverkaZobj(); 
				screen();
				cAz-=15*PI/180;
			}
			break;
		}
		return 0;
		break;
	case WM_DESTROY:
		//отключаем таймер
		KillTimer(hWnd, 1);
		//сообщаем Windows о завершении работы программы
		PostQuitMessage(0);
		return(0);
		break;
	}
	return(DefWindowProc(hWnd, nMsg, wParam, lParam));
}


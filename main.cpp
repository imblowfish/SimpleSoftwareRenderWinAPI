#define UNICODE
#ifdef UNICODE
#define _UNICODE
#endif
#define STRICT
#define WIDTH 600 //желательно делать размер одинаковым для ширины и высоты, иначе модели будут выпуклыми
#define HEIGHT 600
#define DEPTH 255


#include <iostream>
#include <Windows.h>
#include <windowsx.h>
#include <tchar.h>
#include "geom.h"
#include "model.h"
#include "resource.h"
using namespace std;


//прототипы функций
void line(HDC, int, int, int, int, COLORREF);//рисование линии по алгоритму Берзенхема
void triangle(HDC, POINT[3], COLORREF); //полигона
void zeroZBuf(); //обнуление z-буфера
extern "C" void __stdcall initZBuf(int, int);
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

//заготовки цветов
COLORREF red = RGB(255, 0, 0);
COLORREF green = RGB(0, 150, 0);
COLORREF blue = RGB(0, 0, 150);
COLORREF grey = RGB(150, 150, 150);
COLORREF darkgrey = RGB(88, 88, 88);
COLORREF white = RGB(255, 255, 255);
COLORREF black = RGB(0, 0, 0);

int xView = 0, yView = 0;
int zbuffer[WIDTH*HEIGHT]; //z-буфер запоминает значение z вершины, если дальнейшая точка обладает меньшим значением, то она игнорируется
char OBJ_PATH[15]; //путь до файла
HMENU hFileMenu, hHelpMenu, hMode, hMenu; //переменные меню
Vec3f light_dir(0, 0, -1); //положение источника света
Model *model; //указатель на объект типа model, в который будем записывать значения
float coord[2][3] = { {0.5,0,0}, //оси координат
					  {0,0.5,0}};

//рисование линии по алгоритму Берзенхема
void line(HDC lineHdc, int x0, int y0, int x1, int y1, COLORREF color) {
	int save;
	bool steep = false;
	if (fabs(x0 - x1) < fabs(y0 - y1)) {
		save = x0; x0 = y0; y0 = save;
		save = x1; x1 = y1; y1 = save;
		steep = true;
	}
	if (x0 > x1) {
		save = x0; x0 = x1; x1 = save;
		save = y0; y0 = y1; y1 = save;
	}
	int dx = x1 - x0;
	int dy = y1 - y0;
	int derror = fabs(dy) * 2;
	int error = 0;
	int y = y0;
	for (int x = x0; x <= x1; x++) {
		if (steep) {
			SetPixel(lineHdc, y, x, color);
		}
		else {
			SetPixel(lineHdc, x, y, color);
		}

		error += derror;
		if (error > dx) {
			y += (y1 > y0 ? 1 : -1);
			error -= dx * 2;
		}
	}
	//MoveToEx(lineHdc, x0, y0, NULL);
	//LineTo(lineHdc, x1, y1);
}
void triangle(HDC tHDC, Vec3i t0, Vec3i t1, Vec3i t2, COLORREF color, int *zbuffer) {
	if (t0.y == t1.y && t0.y == t2.y) return;
	if (t0.y>t1.y) std::swap(t0, t1);
	if (t0.y>t2.y) std::swap(t0, t2);
	if (t1.y>t2.y) std::swap(t1, t2);
	int total_height = t2.y - t0.y;
	for (int i = 0; i<total_height; i++) {
		bool second_half = i>t1.y - t0.y || t1.y == t0.y;
		int segment_height = second_half ? t2.y - t1.y : t1.y - t0.y;
		float alpha = (float)i / total_height;
		float beta = (float)(i - (second_half ? t1.y - t0.y : 0)) / segment_height;
		Vec3i A = t0 + Vec3i(t2 - t0)*alpha;
		Vec3i B = second_half ? t1 + Vec3i(t2 - t1)*beta : t0 + Vec3i(t1 - t0)*beta;
		if (A.x>B.x) std::swap(A, B);
		for (int j = A.x; j <= B.x; j++) {
			float phi = B.x == A.x ? 1. : (float)(j - A.x) / (float)(B.x - A.x);
			Vec3i P = Vec3i(A) + Vec3i(B - A)*phi;
			int idx = P.x + P.y*WIDTH;
			if (zbuffer[idx]<P.z) {
				zbuffer[idx] = P.z;
				SetPixel(tHDC, P.x, P.y, color);
			}
		}
	}
}
void zeroZBuf() { //инициализация z-буфера минимальным значением
	memset(zbuffer, 0, sizeof(zbuffer));
	initZBuf(WIDTH*HEIGHT, zbuffer[0]);
	//for (int i = 0; i < WIDTH*HEIGHT; i++) {
		//zbuffer[i] = -200000;
	//}
}
//главная функция
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow){
	TCHAR szTitle[] = _TEXT("Курсовая работа");
	TCHAR szWindowClass[] = _TEXT("WinClass");
	MSG	msg;
	WNDCLASSEX wcex;
	HWND hWnd;

	if (hWnd = FindWindow(szWindowClass, NULL)) {
		if (IsIconic(hWnd))
		ShowWindow(hWnd, SW_RESTORE);
		SetForegroundWindow(hWnd);
	}
	memset(&wcex, 0, sizeof(WNDCLASSEX));
	wcex.cbSize = sizeof(WNDCLASSEX);
	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = (WNDPROC)WndProc;
	wcex.hInstance = hInstance;
	wcex.hIcon = (HICON)LoadImage(hInstance, IDI_APPLICATION, IMAGE_ICON, 32, 32, 0);
	wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground = GetStockBrush(LTGRAY_BRUSH);
	wcex.lpszClassName = szWindowClass;
	wcex.hIconSm = NULL;
	if (!RegisterClassEx(&wcex))
		return FALSE;
	hWnd = CreateWindowEx(WS_EX_WINDOWEDGE, szWindowClass, szTitle,
		WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, 0, CW_USEDEFAULT,
		0, NULL, NULL, hInstance, NULL);
	if (!hWnd)
		return FALSE;

	//здесь подключение меню и связывание с окном
	AppendMenu((hFileMenu = CreatePopupMenu()), MF_ENABLED | MFT_STRING, ID_FILE_CUBE, _TEXT("&Cube"));
	AppendMenu(hFileMenu, MF_ENABLED | MFT_STRING, ID_FILE_CAT, _TEXT("&Cat"));
	AppendMenu(hFileMenu, MF_ENABLED | MFT_STRING, ID_FILE_HEAD, _TEXT("&Head"));
	AppendMenu(hFileMenu, MF_ENABLED | MFT_STRING, ID_FILE_EXIT, _TEXT("&Exit"));

	AppendMenu((hMode = CreatePopupMenu()), MF_ENABLED | MFT_STRING, ID_MODE_GRID, _TEXT("&Grid"));
	AppendMenu(hMode, MF_ENABLED | MFT_STRING, ID_MODE_POL, _TEXT("&Polygons"));

	AppendMenu((hHelpMenu = CreatePopupMenu()), MF_ENABLED | MFT_STRING, ID_HELP_ABOUT, _TEXT("&About"));
	AppendMenu((hMenu = CreateMenu()), MF_ENABLED | MF_POPUP, (UINT_PTR)hFileMenu, _TEXT("&File"));
	AppendMenu(hMenu, MF_ENABLED | MF_POPUP, (UINT_PTR)hMode, _TEXT("&Mode"));
	AppendMenu(hMenu, MF_ENABLED | MF_POPUP, (UINT_PTR)hHelpMenu, _TEXT("&Help"));

	SetMenu(hWnd, hMenu);
	ShowWindow(hWnd, SW_SHOWNORMAL);
	UpdateWindow(hWnd);

	while (GetMessage(&msg, NULL, 0, 0)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	return (int)msg.wParam;
}


//главное окно
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
	int xSize = WIDTH;
	int ySize = HEIGHT;
	PAINTSTRUCT ps;
	POINT Points[3];
	LPTSTR info[80];
	memset(&info, 0, sizeof(info));
	wsprintf((LPTSTR)info, _T("Курсовая\nГетьман Ю.В.\nГруппа И962"));
	RECT rect;
	LPMINMAXINFO lpmmi;
	HDC hDc;
	static HBRUSH brush;
	char warning[] = "Выберите режим отображения в меню Mode";
	static HFONT font;
	static bool flag[3] = {FALSE, FALSE, FALSE};

	SetClassLongPtr(hWnd, GCLP_HBRBACKGROUND, (LONG)brush);//для вывода фона другого цвета
	//цикл обработки сообщений
	switch (message) {
	case WM_CREATE:
		brush = CreateSolidBrush(RGB(100, 100, 100)); //кисть, отвечающая за цвет фона
	return TRUE;
	break;
	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case WM_CLOSE:
			if (MessageBox(hWnd, _T("Вы уверены?"), _T("Выход"), MB_YESNO | MB_ICONQUESTION) == IDYES)
				DestroyWindow(hWnd);
			break;
		case ID_FILE_EXIT:
			if (MessageBox(hWnd, _T("Вы уверены?"), _T("Выход"), MB_YESNO | MB_ICONQUESTION) == IDYES)
				PostQuitMessage(0);
			break;
		case ID_HELP_ABOUT:
			MessageBox(NULL, (LPTSTR)info, _T("About"), MB_ICONINFORMATION);
			break;
		case ID_FILE_CUBE://если нажали на кнопку Cube
			model = new Model("models/cube.obj");//записываем путь до файла
			flag[0] = TRUE; //отмечаем флаг
			InvalidateRect(hWnd, NULL, TRUE); //вызываем перерисовку
		break;
		case ID_FILE_HEAD://если нажали на кнопку Cube
			model = new Model("models/head.obj");
			flag[0] = TRUE;
			InvalidateRect(hWnd, NULL, TRUE);
		break;
		case ID_FILE_CAT://если нажали на кнопку Cube
			model = new Model("models/cat.obj");
			flag[0] = TRUE;
			InvalidateRect(hWnd, NULL, TRUE);
		break;
		case ID_MODE_GRID: //выбор режима отображения сетки
			if (flag[2]) {
				flag[2] = FALSE;
				CheckMenuItem(hMode, ID_MODE_GRID, MF_UNCHECKED);
			}else {
				flag[2] = TRUE;
				CheckMenuItem(hMode, ID_MODE_GRID, MF_CHECKED);
			}
				
		break;
		case ID_MODE_POL: //выбор режима отображения полигона
			if (flag[1]) {
				flag[1] = FALSE;
				CheckMenuItem(hMode, ID_MODE_POL, MF_UNCHECKED);
			}
			else {
				flag[1] = TRUE;
				CheckMenuItem(hMode, ID_MODE_POL, MF_CHECKED);
			}
		break;
		}
		break;
	case WM_KEYDOWN:
		switch (wParam) {
			case VK_ESCAPE:
				if (MessageBox(hWnd, _T("Вы уверены?"), _T("Выход"), MB_YESNO | MB_ICONQUESTION) == IDYES)
					SendMessage(hWnd, WM_CLOSE, 0, 0);
			break;
		}
		break;
	case WM_PAINT:
			InvalidateRect(hWnd, NULL, TRUE);
			hDc = BeginPaint(hWnd, &ps);
			GetClientRect(hWnd, &rect);
			SetMapMode(hDc, TRANSPARENT);
			SelectObject(hDc, font); //выбираем шрифт
			zeroZBuf();
			if (flag[0] && (flag[1] || flag[2])){
				if (flag[1]) {
					//цикл рисования полигона
					for (int i = 0; i<model->nfaces(); i++) {
						std::vector<int> face = model->face(i); //выбираем полигон
						Vec3i screen_coords[3]; //экранные координаты
						Vec3f world_coords[3]; //мировые
						for (int j = 0; j<3; j++) { //идем по каждой вершине
							Vec3f v = model->vert(face[j]); //приравниваем новый вектор v данной вершине
							//переводим координаты в экранные по формуле
							screen_coords[j] = Vec3i((v.x + 1.)*xSize / 2., (v.y + 1.)*ySize / 2., (v.z + 1.)*DEPTH / 2.); 
							world_coords[j] = v;
						}
						Vec3f n = (world_coords[2] - world_coords[0]) ^ (world_coords[1] - world_coords[0]); //вектор для нормализации
						n.normalize();
						float intensity = n*light_dir;//вычисляем интенсивность света по формуле в зависимости от нахождения источника
						if (intensity>0) { //если интенсивность больше нуля, рисуем полигон
							triangle(hDc, screen_coords[0], screen_coords[1], screen_coords[2], RGB(intensity * 255, intensity * 255, intensity * 255, 255), zbuffer);
						}
					}
				}
				
				if (flag[2]) {
					//только сетка
					for (int i = 0; i<model->nfaces(); i++) {
						std::vector<int> face = model->face(i);
						for (int j = 0; j<3; j++) {
							Vec3f v0 = model->vert(face[j]);
							Vec3f v1 = model->vert(face[(j + 1) % 3]);
							int x0 = (v0.x + 1.)*xSize / 2.;
							int y0 = (v0.y + 1.)*ySize / 2.;
							int x1 = (v1.x + 1.)*xSize / 2.;
							int y1 = (v1.y + 1.)*ySize / 2.;
							line(hDc, x0, y0, x1, y1, white);
						}
					}
				}
				//система координат
				for (int i = 0; i < 2; i++) {
					float x1, y1 = 0;
					x1 = (coord[i][0] + 1.0)*xSize / 2.;
					y1 = (coord[i][1] + 1.0)*ySize / 2.;
					//z не требуется, так как проекция перспективная и z находится вдоль направления взгляда камеры
					line(hDc, xSize / 2, ySize / 2, x1, y1, i==0?blue:green);
				}
				delete model; //очищаем указатель
				zeroZBuf(); //обнуляем zbuffer
			}else if(!flag[1] && !flag[2] && flag[0]){
				MessageBox(NULL, _T("Вы не выбрали режим отображения в меню 'Mode'"), _T("Предупреждение"), NULL);
				TextOutA(hDc, 150, ySize / 2 - 60, warning, sizeof(warning));
			}
			else {
				TextOutA(hDc, 150, ySize/2-60, warning, sizeof(warning));
			}
			EndPaint(hWnd, &ps);
		break;
		case WM_SIZE:
			xView = LOWORD(lParam);
			yView = HIWORD(lParam);
		break;
		case WM_GETMINMAXINFO:
			lpmmi = (LPMINMAXINFO)lParam;
			lpmmi->ptMinTrackSize.x = xSize;
			lpmmi->ptMinTrackSize.y = ySize;
			lpmmi->ptMaxTrackSize.x = xSize;
			lpmmi->ptMaxTrackSize.y = ySize;
		break;
		case WM_DESTROY:
			PostQuitMessage(0);
			break;
		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
		break;
	}
}
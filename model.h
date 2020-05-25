#ifndef _MODEL
#define _MODEL

#include <vector>
#include "geom.h"

class Model {
private:

public:
	std::vector<Vec3f>verts_; //вектор вершин
	std::vector<std::vector<int>>faces_; //полигонов
	Model(const char *filename); //конструктор, в котором записываем в модель данные из obj файла
	~Model(); //деструктор
	int nverts(); //возвращение кол-ва вершин
	int nfaces(); //возвращение кол-ва полигонов
	Vec3f vert(int i); //получить вершину по номеру
	std::vector<int>face(int idx); //получить полигон, возвращает вектор точек
};

#endif
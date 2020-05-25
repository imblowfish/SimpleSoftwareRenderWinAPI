#ifndef _MODEL
#define _MODEL

#include <vector>
#include "geom.h"

class Model {
private:

public:
	std::vector<Vec3f>verts_; //������ ������
	std::vector<std::vector<int>>faces_; //���������
	Model(const char *filename); //�����������, � ������� ���������� � ������ ������ �� obj �����
	~Model(); //����������
	int nverts(); //����������� ���-�� ������
	int nfaces(); //����������� ���-�� ���������
	Vec3f vert(int i); //�������� ������� �� ������
	std::vector<int>face(int idx); //�������� �������, ���������� ������ �����
};

#endif
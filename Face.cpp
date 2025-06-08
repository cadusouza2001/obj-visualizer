#include "Face.h"

// Implementacao simples para armazenar indices de uma face

Face::Face() {}

Face::Face(const std::vector<int>& v, const std::vector<int>& n, const std::vector<int>& t)
	: verts(v), norms(n), texts(t) {
}

void Face::add(int vIndex, int tIndex, int nIndex) {
	verts.push_back(vIndex);
	texts.push_back(tIndex);
	norms.push_back(nIndex);
}

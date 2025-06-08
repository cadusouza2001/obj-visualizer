#ifndef FACE_H
#define FACE_H

// Armazena indices de vertices, normais e texturas de uma face

#include <vector>

class Face {
public:
	std::vector<int> verts; // indices dos vertices
	std::vector<int> norms; // indices das normais
	std::vector<int> texts; // indices de coordenadas de textura

	Face();
	Face(const std::vector<int>& v, const std::vector<int>& n, const std::vector<int>& t);

	// adiciona um vertice (indices no vetor da malha)
	void add(int vIndex, int tIndex, int nIndex);
};

#endif 

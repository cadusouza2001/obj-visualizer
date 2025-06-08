#ifndef MESH_H
#define MESH_H

// Estrutura principal de uma malha OBJ carregada

#include <string>
#include <vector>
#include <glm/glm.hpp>
#include <GL/glew.h>

class Group;

class Mesh {
public:
	// vetores de vertices, normais e coordenadas de textura
	std::vector<glm::vec3*> vertex;
	std::vector<glm::vec3*> normals;
	std::vector<glm::vec2*> mappings;
	std::vector<Group*> groups;    // grupos da malha
	std::string mtllib;            // arquivo de materiais associado

	Mesh();
	~Mesh();

	// carrega arquivo OBJ e gera buffers
	bool loadOBJ(const std::string& filename);
	// carrega arquivo MTL associado
	bool loadMTL(const std::string& filename);

	// desenha todos os grupos (vao/vbo ja configurados)
	void draw();
};

#endif 

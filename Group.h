#ifndef GROUP_H
#define GROUP_H

// Representa um grupo (g) do arquivo OBJ, responsavel por armazenar
// faces que compartilham o mesmo material

#include <GL/glew.h>
#include <string>
#include <vector>
#include <glm/glm.hpp>

class Face;

class Group {
public:
	std::string name;            // nome do grupo
	std::string material;        // material ativo (usemtl)
	std::vector<Face*> faces;    // faces pertencentes ao grupo
	GLuint vao;                  // Vertex Array Object gerado
	int numVertices;             // quantidade de vertices para render

	Group();
	~Group();

	// adiciona uma face ao grupo
	void addFace(Face* f);
	// gera VAO/VBO com vertices, normais e coordenadas de textura
	void buildBuffers(const std::vector<glm::vec3*>& vertices,
		const std::vector<glm::vec3*>& normals,
		const std::vector<glm::vec2*>& texcoords);
};

#endif 

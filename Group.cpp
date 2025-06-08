#include "Group.h"
#include "Face.h"

// Inicializa grupo sem buffers
Group::Group() : vao(0), numVertices(0) {}

// Libera memoria das faces
Group::~Group() {
	for (Face* f : faces) {
		delete f;
	}
}

// Armazena ponteiro para uma face
void Group::addFace(Face* f) {
	faces.push_back(f);
}

// Construcao dos VBOs e do VAO para enviar dados ao GPU
void Group::buildBuffers(const std::vector<glm::vec3*>& vertices,
	const std::vector<glm::vec3*>& normals,
	const std::vector<glm::vec2*>& texcoords)
{
	// vetores temporarios para enviar ao OpenGL
	std::vector<glm::vec3> finalVerts;
	std::vector<glm::vec3> finalNormals;
	std::vector<glm::vec2> finalTex;

	// percorre cada face coletando vertices, normais e UVs
	for (Face* face : faces) {
		for (size_t i = 0; i < face->verts.size(); ++i) {
			int vIdx = face->verts[i];
			int tIdx = face->texts[i];
			int nIdx = face->norms[i];

			if (vIdx >= 0 && vIdx < (int)vertices.size())
				finalVerts.push_back(*vertices[vIdx]);
			if (nIdx >= 0 && nIdx < (int)normals.size())
				finalNormals.push_back(*normals[nIdx]);
			if (tIdx >= 0 && tIdx < (int)texcoords.size())
				finalTex.push_back(*texcoords[tIdx]);
		}
	}

	numVertices = static_cast<int>(finalVerts.size());

	// cria VAO e VBOs para envio ao pipeline
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	GLuint vbo[3];
	glGenBuffers(3, vbo);

	glBindBuffer(GL_ARRAY_BUFFER, vbo[0]);
	glBufferData(GL_ARRAY_BUFFER, finalVerts.size() * sizeof(glm::vec3), finalVerts.data(), GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);

	glBindBuffer(GL_ARRAY_BUFFER, vbo[1]);
	glBufferData(GL_ARRAY_BUFFER, finalNormals.size() * sizeof(glm::vec3), finalNormals.data(), GL_STATIC_DRAW);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);

	glBindBuffer(GL_ARRAY_BUFFER, vbo[2]);
	glBufferData(GL_ARRAY_BUFFER, finalTex.size() * sizeof(glm::vec2), finalTex.data(), GL_STATIC_DRAW);
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(glm::vec2), (void*)0);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
}

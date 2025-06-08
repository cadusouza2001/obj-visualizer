#include "Mesh.h"
#include "Group.h"
#include "Face.h"
#include <fstream>
#include <sstream>
#include <iostream>
#define sscanf_s sscanf

// Construtor vazio
Mesh::Mesh() {}

// Libera memoria alocada pelos vetores e grupos
Mesh::~Mesh() {
    for (auto v : vertex) delete v;
    for (auto n : normals) delete n;
    for (auto m : mappings) delete m;
    for (auto g : groups) delete g;
}

// Faz o parsing de um arquivo OBJ basico
bool Mesh::loadOBJ(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Nao foi possivel abrir OBJ: " << filename << std::endl;
        return false;
    }

    Group* currentGroup = new Group();
    groups.push_back(currentGroup);

    std::string line;
    while (std::getline(file, line)) {
        std::istringstream iss(line);
        std::string prefix;
        iss >> prefix;  // identifica o tipo de dado

        if (prefix == "v") {
            glm::vec3* v = new glm::vec3();
            iss >> v->x >> v->y >> v->z;
            vertex.push_back(v);
        } else if (prefix == "vn") {
            glm::vec3* n = new glm::vec3();
            iss >> n->x >> n->y >> n->z;
            normals.push_back(n);
        } else if (prefix == "vt") {
            glm::vec2* t = new glm::vec2();
            iss >> t->x >> t->y;
            mappings.push_back(t);
        } else if (prefix == "f") {
            Face* face = new Face();
            std::string token;
            while (iss >> token) {
                int vIdx = -1, tIdx = -1, nIdx = -1;
                sscanf_s(token.c_str(), "%d/%d/%d", &vIdx, &tIdx, &nIdx);
                face->add(vIdx - 1, tIdx - 1, nIdx - 1);
            }
            currentGroup->addFace(face);
        } else if (prefix == "g") { // novo grupo
            std::string name;
            iss >> name;
            if (!currentGroup->faces.empty()) {
                currentGroup = new Group();
                groups.push_back(currentGroup);
            }
            currentGroup->name = name;
        } else if (prefix == "usemtl") {
            iss >> currentGroup->material;
        } else if (prefix == "mtllib") {
            iss >> mtllib;
        }
    }

    file.close();

    // apos leitura, gera buffers para cada grupo
    for (Group* g : groups) {
        g->buildBuffers(vertex, normals, mappings);
    }

    return true;
}

bool Mesh::loadMTL(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Nao foi possivel abrir MTL: " << filename << std::endl;
        return false;
    }
    file.close();
    return true;
}

// Renderiza cada grupo usando os VAOs criados
void Mesh::draw() {
    for (Group* g : groups) {
        glBindVertexArray(g->vao);
        glDrawArrays(GL_TRIANGLES, 0, g->numVertices);
    }
    glBindVertexArray(0);
}

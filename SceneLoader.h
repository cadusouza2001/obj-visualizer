#ifndef SCENE_LOADER_H
#define SCENE_LOADER_H

#include <string>
#include <vector>
#include <map>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "Mesh.h"

// Informacoes de material lidas do arquivo .MTL
// Ka, Kd, Ks e Ns correspondem ao modelo de iluminacao Phong
struct MaterialInfo {
    glm::vec3 Ka{0.2f};   // componente ambiente
    glm::vec3 Kd{0.8f};   // componente difusa
    glm::vec3 Ks{1.0f};   // componente especular
    float Ns{32.0f};      // expoente de brilho (shininess)
    GLuint texture{0};    // textura difusa
};

// Estrutura que representa um objeto na cena
struct Obj3D {
    Mesh* mesh{nullptr};                         // malha carregada
    glm::mat4 transform{1.0f};                   // matriz de transformacao (model)
    glm::vec3 bbMin{0.0f};                       // minimo do bounding box
    glm::vec3 bbMax{0.0f};                       // maximo do bounding box
    // materiais usados por cada grupo da malha
    std::map<std::string, MaterialInfo> materials;
};

// Estrutura lida do arquivo scene.txt que define curva e modelos
struct SceneConfig {
    std::string curveFile;              // caminho para pontos da curva
    std::vector<std::string> objFiles;   // arquivos OBJ a carregar
};

// Converte faces com mais de 3 vertices em triangulos
void triangulate(Mesh* mesh);

// Calcula o bounding box (min e max) da malha
void computeMeshBoundingBox(Mesh* mesh, glm::vec3& mn, glm::vec3& mx);

bool loadOBJWithTriangulation(Mesh* mesh, const std::string& file);
void loadMTL(const std::string& filename, std::map<std::string, MaterialInfo>& mats);
bool loadCurve(const std::string& file, std::vector<glm::vec3>& pts);

bool loadSceneConfig(const std::string& file, SceneConfig& out);
std::vector<Obj3D> loadSceneObjects(const std::vector<std::string>& paths);

void handleCurveKeys(GLFWwindow* win, bool& showCurve, bool& fPressed, size_t& pathIdx);
void animateCarAlongCurve(Obj3D& car, const std::vector<glm::vec3>& pts, size_t& idx, float dt);

#endif

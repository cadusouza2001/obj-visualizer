#include "SceneLoader.h"
#include "Group.h"
#include "Face.h"
#include <fstream>
#include "ShaderUtils.h"
#include <sstream>
#include <iostream>
#include <limits>

// Converte faces com mais de 3 vertices em triangulos.
// OBJ permite polígonos de n lados, mas usamos glDrawArrays(GL_TRIANGLES),
// então quebramos cada face usando um triângulo-fan a partir do primeiro vértice.
void triangulate(Mesh* mesh) {
    for (Group* g : mesh->groups) {
        std::vector<Face*> newFaces;
        for (Face* f : g->faces) {
            if (f->verts.size() <= 3) {
                newFaces.push_back(f);
                continue;
            }
            for (size_t i = 1; i + 1 < f->verts.size(); ++i) {
                Face* tri = new Face();
                tri->add(f->verts[0], f->texts[0], f->norms[0]);
                tri->add(f->verts[i], f->texts[i], f->norms[i]);
                tri->add(f->verts[i + 1], f->texts[i + 1], f->norms[i + 1]);
                newFaces.push_back(tri);
            }
            delete f;
        }
        g->faces = newFaces;
    }
}

// Calcula o bounding box (min e max) da malha
// Determina limites min/max dos vértices, formando a caixa englobante da malha
void computeMeshBoundingBox(Mesh* mesh, glm::vec3& mn, glm::vec3& mx) {
    mn = glm::vec3(std::numeric_limits<float>::max());
    mx = glm::vec3(-std::numeric_limits<float>::max());
    for (auto v : mesh->vertex) {
        mn = glm::min(mn, *v);
        mx = glm::max(mx, *v);
    }
}

bool loadOBJWithTriangulation(Mesh* mesh, const std::string& file) {
    std::ifstream in(file);
    if (!in.is_open()) {
        std::cerr << "Cannot open OBJ " << file << "\n";
        return false;
    }
    Group* current = new Group();
    mesh->groups.push_back(current);
    std::string line;
    while (std::getline(in, line)) {
        std::istringstream iss(line);
        std::string pref; iss >> pref;
        if (pref == "v") {
            glm::vec3* v = new glm::vec3();
            iss >> v->x >> v->y >> v->z;
            mesh->vertex.push_back(v);
        }
        else if (pref == "vn") {
            glm::vec3* n = new glm::vec3();
            iss >> n->x >> n->y >> n->z;
            mesh->normals.push_back(n);
        }
        else if (pref == "vt") {
            glm::vec2* t = new glm::vec2();
            iss >> t->x >> t->y;
            mesh->mappings.push_back(t);
        }
        else if (pref == "f") {
            std::vector<int> vs, ts, ns; std::string tok;
            while (iss >> tok) {
                int vi = -1, ti = -1, ni = -1;
                sscanf_s(tok.c_str(), "%d/%d/%d", &vi, &ti, &ni);
                vs.push_back(vi - 1); ts.push_back(ti - 1); ns.push_back(ni - 1);
            }
            if (vs.size() <= 3) {
                Face* f = new Face();
                for (size_t i = 0;i < vs.size();++i) f->add(vs[i], ts[i], ns[i]);
                current->addFace(f);
            }
            else {
                for (size_t i = 1;i + 1 < vs.size();++i) {
                    Face* f = new Face();
                    f->add(vs[0], ts[0], ns[0]);
                    f->add(vs[i], ts[i], ns[i]);
                    f->add(vs[i + 1], ts[i + 1], ns[i + 1]);
                    current->addFace(f);
                }
            }
        }
        else if (pref == "g") {
            std::string name; iss >> name;
            if (!current->faces.empty()) {
                current = new Group();
                mesh->groups.push_back(current);
            }
            current->name = name;
        }
        else if (pref == "usemtl") {
            std::string mat; iss >> mat;
            if (!current->faces.empty()) {
                Group* ng = new Group();
                ng->name = current->name;
                mesh->groups.push_back(ng);
                current = ng;
            }
            current->material = mat;
        }
        else if (pref == "mtllib") {
            iss >> mesh->mtllib;
        }
    }
    in.close();
    for (Group* g : mesh->groups) {
        g->buildBuffers(mesh->vertex, mesh->normals, mesh->mappings);
    }
    return true;
}

// Le arquivo .MTL e carrega informacoes de material
void loadMTL(const std::string& filename, std::map<std::string, MaterialInfo>& mats) {
    std::ifstream in(filename);
    if (!in.is_open()) { std::cerr << "Cannot open MTL " << filename << "\n"; return; }
    std::string dir = filename.substr(0, filename.find_last_of("/\\") + 1);
    std::string line; MaterialInfo * cur = nullptr; std::string name;
    while (std::getline(in, line)) {
        std::istringstream iss(line); std::string p; iss >> p;
        if (p == "newmtl") { iss >> name; cur = &mats[name]; }
        else if (p == "Ka" && cur) { iss >> cur->Ka.r >> cur->Ka.g >> cur->Ka.b; }
        else if (p == "Kd" && cur) { iss >> cur->Kd.r >> cur->Kd.g >> cur->Kd.b; }
        else if (p == "Ks" && cur) { iss >> cur->Ks.r >> cur->Ks.g >> cur->Ks.b; }
        else if (p == "Ns" && cur) { iss >> cur->Ns; }
        else if (p == "map_Kd" && cur) { std::string tex; iss >> tex; cur->texture = loadTexture(dir + tex); }
    }
}

// Le pontos de uma curva de animacao a partir de um arquivo de texto
bool loadCurve(const std::string& file, std::vector<glm::vec3>& pts) {
    std::ifstream in(file);
    if (!in.is_open()) { std::cerr << "Cannot open curve " << file << "\n";return false; }
    float x, y, z;
    while (in >> x >> y >> z) pts.emplace_back(x, y, z);
    return !pts.empty();
}

bool loadSceneConfig(const std::string& file, SceneConfig& out) {
    std::ifstream in(file);
    if(!in.is_open()) {
        std::cerr << "Cannot open scene file " << file << "\n";
        return false;
    }
    std::string type, path;
    while(in >> type >> path) {
        if(type == "curve") out.curveFile = path;
        else if(type == "obj") out.objFiles.push_back(path);
    }
    return true;
}

std::vector<Obj3D> loadSceneObjects(const std::vector<std::string>& paths) {
    std::vector<Obj3D> objs;
    for(const std::string& path : paths) {
        Mesh* m = new Mesh();
        if(!loadOBJWithTriangulation(m, path))
            continue;
        triangulate(m);
        for(Group* g : m->groups)
            g->buildBuffers(m->vertex, m->normals, m->mappings);
        Obj3D obj; obj.mesh = m; obj.transform = glm::mat4(1.0f);
        if(!m->mtllib.empty()) {
            std::map<std::string, MaterialInfo> mats;
            loadMTL(m->mtllib, mats);
            obj.materials = std::move(mats);
        }
        computeMeshBoundingBox(m, obj.bbMin, obj.bbMax);
        objs.push_back(obj);
    }
    return objs;
}


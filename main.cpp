#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <vector>
#include <string>
#include <map>
#include <sstream>
#include <fstream>
#include <limits>
#include "Mesh.h"      // definicoes de estrutura de um OBJ
#include "Group.h"
#include "Face.h"

// stb_image e utilizado para carregar texturas de arquivos
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

// Informacoes de material lidas do arquivo .MTL
// Ka, Kd, Ks e Ns correspondem ao modelo de iluminacao Phong
struct MaterialInfo {
	glm::vec3 Ka{ 0.2f };   // componente ambiente
	glm::vec3 Kd{ 0.8f };   // componente difusa
	glm::vec3 Ks{ 1.0f };   // componente especular
	float Ns{ 32.0f };      // expoente de brilho (shininess)
	GLuint texture{ 0 };    // textura difusa
};

// Estrutura que representa um objeto na cena
struct Obj3D {
	Mesh* mesh{ nullptr };                             // malha carregada
	glm::mat4 transform{ 1.0f };                       // matriz de transformacao (model)
	glm::vec3 bbMin{ 0.0f };                           // minimo do bounding box
	glm::vec3 bbMax{ 0.0f };                           // maximo do bounding box
	// materiais usados por cada grupo da malha
	std::map<std::string, MaterialInfo> materials;
};

// Estado da camera controlada pelo mouse e teclado
static glm::vec3 camPos(524.0f, 0.0f, 212.0f); // posicao inicial
static float yaw = -90.0f;                 // angulo Yaw (olhando para -Z)
static float pitch = 0.0f;                 // angulo Pitch
static bool rotating = false;              // se o botao esquerdo do mouse esta pressionado
static double lastX = 0.0, lastY = 0.0;    // ultima posicao do mouse
static bool freeCamera = false;            // false: camera segue o carro
static bool oPressed = false;              // controle da tecla O
static const float chaseDistance = 5.0f;   // dist. da camera atras do carro
static const float chaseHeight = 2.0f;     // altura da camera em relacao ao carro
static float carAngleOffset = glm::radians(-90.0f); // offset angular aplicado ao carro (radianos)
static float orbitYaw = 0.0f;              // angulo horizontal ao orbitar o carro
static float orbitPitch = 0.0f;            // angulo vertical ao orbitar o carro
static float lightOrbitAngle = 0.0f;       // angulo base das luzes em orbita
static const float lightOrbitRadius = 5.0f;   // raio da orbita das luzes
static const float lightOrbitHeight = 5.0f;   // altura das luzes em relacao ao carro
static const float lightOrbitSpeed = glm::radians(90.0f); // velocidade de rotacao das luzes

// Carrega uma textura 2D e gera mipmaps
// Uso do sampler2D no fragment shader (texturizacao)
static GLuint loadTexture(const std::string& file) {
	int w, h, n;
	unsigned char* data = stbi_load(file.c_str(), &w, &h, &n, 0);
	if (!data) {
		std::cerr << "Failed to load texture " << file << "\n";
		return 0;
	}
	GLenum format = n == 4 ? GL_RGBA : GL_RGB;
	GLuint tex; glGenTextures(1, &tex);          // gera VBO para a textura
	glBindTexture(GL_TEXTURE_2D, tex);
	glTexImage2D(GL_TEXTURE_2D, 0, format, w, h, 0, format, GL_UNSIGNED_BYTE, data);
	glGenerateMipmap(GL_TEXTURE_2D);             // gera mipmaps para interpolacao
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	stbi_image_free(data);
	return tex;
}

// Utilitario para ler arquivos de texto (shaders)
static std::string readFile(const std::string& path) {
	std::ifstream f(path);
	std::stringstream ss; ss << f.rdbuf();
	return ss.str();
}

// Compilacao de um shader GLSL
static GLuint compileShader(GLenum type, const std::string& src) {
	GLuint s = glCreateShader(type); const char* c = src.c_str();
	glShaderSource(s, 1, &c, nullptr); glCompileShader(s);
	GLint ok; glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
	if (!ok) {
		char log[512];
		glGetShaderInfoLog(s, 512, nullptr, log);
		std::cerr << "Shader error: " << log << "\n";
	}
	return s;
}

// Construcao do programa de shader completo (vertex + fragment)
static GLuint buildProgram(const std::string& vsrc, const std::string& fsrc) {
	GLuint vs = compileShader(GL_VERTEX_SHADER, vsrc);
	GLuint fs = compileShader(GL_FRAGMENT_SHADER, fsrc);
	GLuint p = glCreateProgram();
	glAttachShader(p, vs);
	glAttachShader(p, fs);
	glLinkProgram(p);
	GLint ok;
	glGetProgramiv(p, GL_LINK_STATUS, &ok);
	if (!ok) {
		char log[512];
		glGetProgramInfoLog(p, 512, nullptr, log);
		std::cerr << "Link error: " << log << "\n";
	}
	glDeleteShader(vs);
	glDeleteShader(fs);
	return p;
}

// Converte faces com mais de 3 vertices em triangulos
// (criterio Grau A: triangulacao)
static void triangulate(Mesh* mesh) {
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
// Util para colisao ou visualizacao dos limites
static void computeBoundingBox(Mesh* mesh, glm::vec3& mn, glm::vec3& mx) {
	mn = glm::vec3(std::numeric_limits<float>::max());
	mx = glm::vec3(-std::numeric_limits<float>::max());
	for (auto v : mesh->vertex) {
		mn = glm::min(mn, *v);
		mx = glm::max(mx, *v);
	}
}

static bool loadOBJWithTriangulation(Mesh* mesh, const std::string& file) {
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
                        // If the current group already has faces associated with
                        // a previous material, start a new group so that each
                        // Group corresponds to a single material. This avoids
                        // losing material assignments when multiple materials
                        // are used within the same OBJ group.
                        if (!current->faces.empty()) {
                                Group* ng = new Group();
                                ng->name = current->name; // keep same logical group name
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
static void loadMTL(const std::string& filename, std::map<std::string, MaterialInfo>& mats) {
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
static bool loadCurve(const std::string& file, std::vector<glm::vec3>& pts) {
        std::ifstream in(file);
        if (!in.is_open()) { std::cerr << "Cannot open curve " << file << "\n"; return false; }
        float x, y, z;
        while (in >> x >> y >> z) pts.emplace_back(x, y, z);
        return !pts.empty();
}

// Atualiza a transformacao do carro baseado na sequencia de pontos da curva
// move o carro ao longo da curva em uma velocidade constante controlada por dt
static void animateCarOnCurve(Obj3D& car, const std::vector<glm::vec3>& pts,
                              size_t& idx, float dt) {
        if (pts.size() < 2) return;

        // Progresso suave ao longo da curva
        static float segT = 0.0f;
        const float speed = 15.0f; // controla a velocidade do carro
        segT += dt * speed;
        while (segT >= 1.0f) {
                segT -= 1.0f;
                idx = (idx + 1) % pts.size();
        }

        size_t next = (idx + 1) % pts.size();
        glm::vec3 pos = glm::mix(pts[idx], pts[next], segT);

        // Orientacao seguindo a direcao do segmento atual com offset
        glm::vec3 dir = glm::normalize(pts[next] - pts[idx]);
        float pathAngle = atan2(dir.z, dir.x);

        glm::mat4 model = glm::translate(glm::mat4(1.0f), pos);
        model = glm::rotate(model, - pathAngle + carAngleOffset, glm::vec3(0, 1, 0));
        car.transform = model;
}


// Callback que detecta pressionamento do botao do mouse
static void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
        if (button == GLFW_MOUSE_BUTTON_LEFT && freeCamera) {
                if (action == GLFW_PRESS) {
                        rotating = true;
                        glfwGetCursorPos(window, &lastX, &lastY);
                }
                else if (action == GLFW_RELEASE) {
                        rotating = false;
                }
        }
}

// Callback que atualiza angulos da camera enquanto o mouse move
static void cursorPosCallback(GLFWwindow* window, double xpos, double ypos) {
        if (!rotating || !freeCamera) return;
        float sensitivity = 0.1f;
        float dx = static_cast<float>(xpos - lastX);
        float dy = static_cast<float>(ypos - lastY);
        lastX = xpos;
        lastY = ypos;
	yaw += dx * sensitivity;
	pitch -= dy * sensitivity;
	if (pitch > 89.0f) pitch = 89.0f;
	if (pitch < -89.0f) pitch = -89.0f;
}

int main() {
	if (!glfwInit()) { std::cerr << "Failed to init GLFW\n";return -1; }
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	GLFWwindow* win = glfwCreateWindow(800, 600, "OpenGL", nullptr, nullptr);
	if (!win) { glfwTerminate();return -1; } glfwMakeContextCurrent(win);
	glewExperimental = true; if (glewInit() != GLEW_OK) { std::cerr << "GLEW init failed\n";return -1; }
	glEnable(GL_DEPTH_TEST);
	glfwSetMouseButtonCallback(win, mouseButtonCallback);
	glfwSetCursorPosCallback(win, cursorPosCallback);

	const char* vsrc = "#version 330 core\n"
		"layout(location=0) in vec3 aPos;\n"
		"layout(location=1) in vec3 aNormal;\n"
		"layout(location=2) in vec2 aTex;\n"
		"out vec3 FragPos;\n"
		"out vec3 Normal;\n"
		"out vec2 TexCoord;\n"
		"uniform mat4 model;\n"
		"uniform mat4 view;\n"
		"uniform mat4 projection;\n"
		"void main(){\n"
		"FragPos = vec3(model*vec4(aPos,1.0));\n"
		"Normal = mat3(transpose(inverse(model)))*aNormal;\n"
		"TexCoord=aTex;\n"
		"gl_Position = projection*view*vec4(FragPos,1.0);\n"
		"}";

        const char* fsrc = "#version 330 core\n"
                "struct Material{ sampler2D diffuse; vec3 ambient; vec3 diffuseColor; vec3 specular; float shininess; int useTexture; };\n"
                "struct Light{ vec3 position; vec3 color; float constant; float linear; float quadratic; };\n"
                "struct DirLight{ vec3 direction; vec3 color; };\n"
                "in vec3 FragPos;\n"
                "in vec3 Normal;\n"
                "in vec2 TexCoord;\n"
                "out vec4 FragColor;\n"
                "uniform Material material;\n"
                "uniform Light lights[3];\n"
                "uniform DirLight dirLight;\n"
                "uniform vec3 viewPos;\n"
                "uniform vec3 fogColor;\n"
                "uniform float fogDensity;\n"
                "void main(){\n"
                "vec3 norm = normalize(Normal);\n"
                "vec3 viewDir = normalize(viewPos - FragPos);\n"
                "vec3 texCol = material.useTexture == 1 ? texture(material.diffuse, TexCoord).rgb : vec3(1.0);\n"
                "vec3 baseColor = material.diffuseColor * texCol;\n"
                "vec3 result = vec3(0.0);\n"
                "for(int i=0;i<3;++i){\n"
                "  vec3 ambient = material.ambient * baseColor;\n"
                "  vec3 lightDir = normalize(lights[i].position - FragPos);\n"
                "  float diff = max(dot(norm, lightDir), 0.0);\n"
                "  vec3 diffuse = diff * baseColor;\n"
                "  vec3 reflectDir = reflect(-lightDir, norm);\n"
                "  float spec = pow(max(dot(viewDir, reflectDir),0.0), material.shininess);\n"
                "  vec3 specular = material.specular * spec;\n"
                "  float d = length(lights[i].position - FragPos);\n"
                "  float att = 1.0 / (lights[i].constant + lights[i].linear*d + lights[i].quadratic*d*d);\n"
                "  result += (ambient + diffuse + specular) * lights[i].color * att;\n"
                "}\n"
                "vec3 sunDir = normalize(-dirLight.direction);\n"
                "float diffSun = max(dot(norm, sunDir), 0.0);\n"
                "vec3 diffuseSun = diffSun * baseColor;\n"
                "vec3 reflectSun = reflect(-sunDir, norm);\n"
                "float specSun = pow(max(dot(viewDir, reflectSun),0.0), material.shininess);\n"
                "vec3 specularSun = material.specular * specSun;\n"
                "result += (material.ambient * baseColor + diffuseSun + specularSun) * dirLight.color;\n"
                "float dist = length(viewPos - FragPos);\n"
                "float fogFactor = exp(-fogDensity*dist);\n"
                "fogFactor = clamp(fogFactor,0.0,1.0);\n"
                "result = mix(fogColor, result, fogFactor);\n"
                "FragColor = vec4(result, 1.0);\n"
                "}";

        GLuint program = buildProgram(vsrc, fsrc);
        glUseProgram(program);

        // ---- Leitura do arquivo de cena com caminhos dos recursos ----
        struct SceneConfig {
                std::string curveFile;
                std::vector<std::string> objFiles;
        } cfg;

        auto loadSceneFile = [](const std::string& file, SceneConfig& out) {
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
        };

        if(!loadSceneFile("scene.txt", cfg)) return -1;

        std::string curveFile = cfg.curveFile;
        if(curveFile.empty()) {
                std::cerr << "No curve file specified in scene.txt\n";
                return -1;
        }
        if(cfg.objFiles.empty()) {
                std::cerr << "No OBJ files listed in scene.txt\n";
                return -1;
        }

        // Carrega pontos da curva da animacao
        std::vector<glm::vec3> curvePoints;
        loadCurve(curveFile, curvePoints);

        glm::vec3 lightPositions[3] = { glm::vec3(0.0f) };
        if(!curvePoints.empty()){
                lightPositions[0] = curvePoints[0] + glm::vec3(0,5,0);
                lightPositions[1] = curvePoints[curvePoints.size()/2] + glm::vec3(0,5,0);
                lightPositions[2] = curvePoints.back() + glm::vec3(0,5,0);
        }

        // VAO/VBO para opcionalmente desenhar a curva
        GLuint curveVAO = 0, curveVBO = 0;
        if (!curvePoints.empty()) {
                glGenVertexArrays(1, &curveVAO);
                glGenBuffers(1, &curveVBO);
                glBindVertexArray(curveVAO);
                glBindBuffer(GL_ARRAY_BUFFER, curveVBO);
                glBufferData(GL_ARRAY_BUFFER, curvePoints.size() * sizeof(glm::vec3), curvePoints.data(), GL_STATIC_DRAW);
                glEnableVertexAttribArray(0);
                glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);
                glVertexAttrib3f(1, 0.0f, 1.0f, 0.0f); // normal constante
                glVertexAttrib2f(2, 0.0f, 0.0f);       // texcoord nulo
                glBindVertexArray(0);
        }

        std::vector<Obj3D> scene;

        // ---- Carregamento dos modelos listados no arquivo de cena ----
        std::vector<std::string> objFiles = cfg.objFiles;
        for (const std::string& path : objFiles) {
                Mesh* m = new Mesh();
                if (!loadOBJWithTriangulation(m, path)) continue;
                triangulate(m); // garante que sejam triangulos
                for (Group* g : m->groups) {
                        g->buildBuffers(m->vertex, m->normals, m->mappings);
                }
                Obj3D obj; obj.mesh = m; obj.transform = glm::mat4(1.0f);
                if (!m->mtllib.empty()) {
                        std::map<std::string, MaterialInfo> mats; loadMTL(m->mtllib, mats); obj.materials = std::move(mats);
                }
                computeBoundingBox(m, obj.bbMin, obj.bbMax);
                scene.push_back(obj);
        }

        // indices para acesso rapido
        size_t trackIndex = 0;
        size_t carIndex = scene.size() > 1 ? 1 : 0;
        size_t pathIndex = 0;           // ponto atual da curva
        bool showCurve = false;
        bool fPressed = false;

	GLint modelLoc = glGetUniformLocation(program, "model");
	GLint viewLoc = glGetUniformLocation(program, "view");
	GLint projLoc = glGetUniformLocation(program, "projection");
        GLint lightPosLoc[3];
        GLint lightColorLoc[3];
        GLint lightConstLoc[3];
        GLint lightLinLoc[3];
        GLint lightQuadLoc[3];
        for(int i=0;i<3;++i){
                std::string base = "lights[" + std::to_string(i) + "]";
                lightPosLoc[i] = glGetUniformLocation(program, (base+".position").c_str());
                lightColorLoc[i] = glGetUniformLocation(program, (base+".color").c_str());
                lightConstLoc[i] = glGetUniformLocation(program, (base+".constant").c_str());
                lightLinLoc[i] = glGetUniformLocation(program, (base+".linear").c_str());
                lightQuadLoc[i] = glGetUniformLocation(program, (base+".quadratic").c_str());
        }
        GLint dirLightDirLoc = glGetUniformLocation(program, "dirLight.direction");
        GLint dirLightColorLoc = glGetUniformLocation(program, "dirLight.color");
        GLint viewPosLoc = glGetUniformLocation(program, "viewPos");
        GLint fogColorLoc = glGetUniformLocation(program, "fogColor");
        GLint fogDensityLoc = glGetUniformLocation(program, "fogDensity");
        GLint matDiffuseLoc = glGetUniformLocation(program, "material.diffuse");
        GLint matAmbientLoc = glGetUniformLocation(program, "material.ambient");
        GLint matDiffuseColorLoc = glGetUniformLocation(program, "material.diffuseColor");
        GLint matSpecularLoc = glGetUniformLocation(program, "material.specular");
        GLint matShineLoc = glGetUniformLocation(program, "material.shininess");
        GLint matUseTexLoc = glGetUniformLocation(program, "material.useTexture");

        // luzes orbitais neutras para realcar as cores originais dos materiais
        const glm::vec3 lightColors[3] = {
                glm::vec3(1.0f),
                glm::vec3(1.0f),
                glm::vec3(1.0f)
        };
        for(int i=0;i<3;++i){
                glUniform3fv(lightPosLoc[i], 1, glm::value_ptr(lightPositions[i]));
                glUniform3fv(lightColorLoc[i], 1, glm::value_ptr(lightColors[i]));
                glUniform1f(lightConstLoc[i], 1.0f);
                glUniform1f(lightLinLoc[i], 0.045f);
                glUniform1f(lightQuadLoc[i], 0.0075f);
        }
        // luz direcional principal (sol)
        glm::vec3 sunDir = glm::normalize(glm::vec3(-0.3f, -1.0f, -0.3f));
        glm::vec3 sunColor(0.4f, 0.45f, 0.5f); // luz difusa azulada
        glUniform3fv(dirLightDirLoc, 1, glm::value_ptr(sunDir));
        glUniform3fv(dirLightColorLoc, 1, glm::value_ptr(sunColor));
        // cor do nevoeiro levemente mais clara para nao distorcer tons
        glUniform3f(fogColorLoc, 0.6f, 0.6f, 0.65f);
        glUniform1f(fogDensityLoc, 0.05f);
        glUniform1i(matDiffuseLoc, 0);

        // posicao da camera e orientacao sao definidas globalmente
        float lastTime = (float)glfwGetTime();

        while (!glfwWindowShouldClose(win)) {
                float time = (float)glfwGetTime();
                float dt = time - lastTime; lastTime = time;

                bool oDown = glfwGetKey(win, GLFW_KEY_O) == GLFW_PRESS;
                static bool cameraSwitched = false;
                if (oDown && !oPressed) {
                        freeCamera = !freeCamera;
                        oPressed = true;
                        rotating = false;
                        cameraSwitched = true;
                }
                if (!oDown) oPressed = false;
                if (!freeCamera && cameraSwitched) {
                        orbitYaw = 0.0f;
                        orbitPitch = 0.0f;
                        cameraSwitched = false;
                }


                // tecla F alterna a visualizacao da curva de debug
                if (glfwGetKey(win, GLFW_KEY_F) == GLFW_PRESS && !fPressed) { showCurve = !showCurve; fPressed = true; }
                if (glfwGetKey(win, GLFW_KEY_F) == GLFW_RELEASE) fPressed = false;
                // tecla R reinicia a animacao do carro
                if (glfwGetKey(win, GLFW_KEY_R) == GLFW_PRESS) pathIndex = 0;

                // Atualiza a transformacao do carro a cada frame com velocidade reduzida
                if (carIndex < scene.size())
                        animateCarOnCurve(scene[carIndex], curvePoints, pathIndex, dt);

                glm::vec3 carPos = glm::vec3(scene[carIndex].transform[3]);
                glm::vec3 carFront = glm::normalize(glm::vec3(scene[carIndex].transform * glm::vec4(0, 0, -1, 0)));

                if (!freeCamera) {
                        const float orbitSpeed = 90.0f;
                        if (glfwGetKey(win, GLFW_KEY_W) == GLFW_PRESS || glfwGetKey(win, GLFW_KEY_UP) == GLFW_PRESS)
                                orbitPitch += orbitSpeed * dt;
                        if (glfwGetKey(win, GLFW_KEY_S) == GLFW_PRESS || glfwGetKey(win, GLFW_KEY_DOWN) == GLFW_PRESS)
                                orbitPitch -= orbitSpeed * dt;
                        if (glfwGetKey(win, GLFW_KEY_A) == GLFW_PRESS || glfwGetKey(win, GLFW_KEY_LEFT) == GLFW_PRESS)
                                orbitYaw += orbitSpeed * dt;
                        if (glfwGetKey(win, GLFW_KEY_D) == GLFW_PRESS || glfwGetKey(win, GLFW_KEY_RIGHT) == GLFW_PRESS)
                                orbitYaw -= orbitSpeed * dt;
                        orbitPitch = glm::clamp(orbitPitch, -89.0f, 89.0f);

                        glm::vec3 baseDir = -carFront;
                        glm::mat4 rotYaw = glm::rotate(glm::mat4(1.0f), glm::radians(orbitYaw), glm::vec3(0,1,0));
                        glm::vec3 dir = glm::vec3(rotYaw * glm::vec4(baseDir, 0.0f));
                        glm::vec3 rightAxis = glm::normalize(glm::cross(dir, glm::vec3(0,1,0)));
                        glm::mat4 rotPitch = glm::rotate(glm::mat4(1.0f), glm::radians(orbitPitch), rightAxis);
                        dir = glm::normalize(glm::vec3(rotPitch * glm::vec4(dir, 0.0f)));

                        camPos = carPos + dir * chaseDistance + glm::vec3(0, chaseHeight, 0);
                }

                glm::vec3 front;
                if (freeCamera) {
                        front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
                        front.y = sin(glm::radians(pitch));
                        front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
                        front = glm::normalize(front);
                } else {
                        front = glm::normalize(carPos - camPos);
                        yaw = glm::degrees(atan2(front.z, front.x));
                        pitch = glm::degrees(asin(front.y));
                }

                glm::vec3 right = glm::normalize(glm::cross(front, glm::vec3(0, 1, 0)));

                if (freeCamera) {
                        if (glfwGetKey(win, GLFW_KEY_W) == GLFW_PRESS || glfwGetKey(win, GLFW_KEY_UP) == GLFW_PRESS)
                                camPos += front * dt * 5.0f;
                        if (glfwGetKey(win, GLFW_KEY_S) == GLFW_PRESS || glfwGetKey(win, GLFW_KEY_DOWN) == GLFW_PRESS)
                                camPos -= front * dt * 5.0f;
                        if (glfwGetKey(win, GLFW_KEY_A) == GLFW_PRESS || glfwGetKey(win, GLFW_KEY_LEFT) == GLFW_PRESS)
                                camPos -= right * dt * 5.0f;
                        if (glfwGetKey(win, GLFW_KEY_D) == GLFW_PRESS || glfwGetKey(win, GLFW_KEY_RIGHT) == GLFW_PRESS)
                                camPos += right * dt * 5.0f;
                }
                if (glfwGetKey(win, GLFW_KEY_ESCAPE) == GLFW_PRESS) glfwSetWindowShouldClose(win, 1);

                glm::mat4 view = glm::lookAt(camPos, camPos + front, glm::vec3(0, 1, 0));
                glm::mat4 projection = glm::perspective(glm::radians(45.0f), 800.0f / 600.0f, 0.1f, 100.0f);

                lightOrbitAngle -= lightOrbitSpeed * dt; // sentido horario
                for(int i=0;i<3;++i){
                        float ang = lightOrbitAngle + i * 2.0f * 3.14159265f / 3.0f;
                        lightPositions[i] = carPos + glm::vec3(cos(ang) * lightOrbitRadius,
                                                             lightOrbitHeight,
                                                             sin(ang) * lightOrbitRadius);
                }


		glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glUseProgram(program);
                glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
                glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));
                glUniform3fv(viewPosLoc, 1, glm::value_ptr(camPos));
                for(int i=0;i<3;++i)
                        glUniform3fv(lightPosLoc[i], 1, glm::value_ptr(lightPositions[i]));

                for (const Obj3D& obj : scene) {
                        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(obj.transform));
                        for (Group* g : obj.mesh->groups) {
                                auto it = obj.materials.find(g->material);
                                MaterialInfo mat;
                                if (it != obj.materials.end()) mat = it->second;
                                glUniform3fv(matAmbientLoc, 1, glm::value_ptr(mat.Ka));
                                glUniform3fv(matDiffuseColorLoc, 1, glm::value_ptr(mat.Kd));
                                glUniform3fv(matSpecularLoc, 1, glm::value_ptr(mat.Ks));
                                glUniform1f(matShineLoc, mat.Ns);
                                glUniform1i(matUseTexLoc, mat.texture ? 1 : 0);
                                glActiveTexture(GL_TEXTURE0);
                                glBindTexture(GL_TEXTURE_2D, mat.texture);
                                glBindVertexArray(g->vao);
                                glDrawArrays(GL_TRIANGLES, 0, g->numVertices);
                        }
                }
                // Desenha a linha da curva para depuracao se habilitado
                if (showCurve && curveVAO) {
                        MaterialInfo dbg; dbg.Ka = glm::vec3(1.0f, 0.0f, 0.0f); dbg.Kd = dbg.Ka; dbg.Ks = glm::vec3(0.0f); dbg.Ns = 1.0f; dbg.texture = 0;
                        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(glm::mat4(1.0f)));
                        glUniform3fv(matAmbientLoc, 1, glm::value_ptr(dbg.Ka));
                        glUniform3fv(matDiffuseColorLoc, 1, glm::value_ptr(dbg.Kd));
                        glUniform3fv(matSpecularLoc, 1, glm::value_ptr(dbg.Ks));
                        glUniform1f(matShineLoc, dbg.Ns);
                        glUniform1i(matUseTexLoc, 0);
                        glActiveTexture(GL_TEXTURE0);
                        glBindTexture(GL_TEXTURE_2D, 0);
                        glBindVertexArray(curveVAO);
                        glDrawArrays(GL_LINE_STRIP, 0, (GLsizei)curvePoints.size());
                }
                glBindVertexArray(0);

		glfwSwapBuffers(win);
		glfwPollEvents();
	}

	glfwTerminate();
	return 0;
}

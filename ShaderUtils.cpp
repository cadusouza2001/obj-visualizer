#include "ShaderUtils.h"
#include <glm/gtc/type_ptr.hpp>
#include <fstream>
#include <sstream>
#include <iostream>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include "SceneLoader.h" // for loadTexture used by SceneLoader as well

const glm::vec3 clearColor(0.2f,0.2f,0.2f);

// Carrega uma textura 2D e gera mipmaps
// Uso do sampler2D no fragment shader (texturizacao)
GLuint loadTexture(const std::string& file) {
    int w, h, n;
    unsigned char* data = stbi_load(file.c_str(), &w, &h, &n, 0);
    if (!data) {
        std::cerr << "Failed to load texture " << file << "\n";
        return 0;
    }
    GLenum format = n == 4 ? GL_RGBA : GL_RGB;
    GLuint tex; glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexImage2D(GL_TEXTURE_2D, 0, format, w, h, 0, format, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    stbi_image_free(data);
    return tex;
}

static std::string readFile(const std::string& path) {
    std::ifstream f(path);
    std::stringstream ss; ss << f.rdbuf();
    return ss.str();
}

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

// Compila e linka um programa de shaders.
// Quando 'bindAttribs' for verdadeiro, associa os nomes dos atributos aos
// índices 0,1,2 antes do link. Necessário para GLSL mais antigo.
static GLuint buildProgram(const std::string& vsrc, const std::string& fsrc,
                           bool bindAttribs) {
    GLuint vs = compileShader(GL_VERTEX_SHADER, vsrc);
    GLuint fs = compileShader(GL_FRAGMENT_SHADER, fsrc);
    GLuint p = glCreateProgram();
    glAttachShader(p, vs);
    glAttachShader(p, fs);
    if (bindAttribs) {
        glBindAttribLocation(p, 0, "aPos");
        glBindAttribLocation(p, 1, "aNormal");
        glBindAttribLocation(p, 2, "aTex");
    }
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

// Cria e compila o par de shaders usados na aplicação.
// Mantemos os códigos fonte diretamente aqui para facilitar a distribuição.
GLuint createShaderProgram(){
    int maj = 0, min = 0;
    const char* ver = reinterpret_cast<const char*>(glGetString(GL_SHADING_LANGUAGE_VERSION));
    if (ver) sscanf(ver, "%d.%d", &maj, &min);
    bool modern = (maj * 100 + min) >= 330;

    std::string vsrc;
    std::string fsrc;

    if (modern) {
        vsrc =
            "#version 330 core\n"
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

        fsrc =
            "#version 330 core\n"
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
            "uniform float fogStart;\n"
            "void main(){\n"
            "vec3 norm = normalize(Normal);\n"
            "vec3 viewDir = normalize(viewPos - FragPos);\n"
            "vec3 texCol = material.useTexture == 1 ? texture(material.diffuse, TexCoord).rgb : vec3(1.0);\n"
            "vec3 baseColor = material.diffuseColor * texCol;\n"
            "vec3 ambient = material.ambient * texCol;\n"
            "vec3 result = ambient;\n"
            "for(int i=0;i<3;++i){\n"
            "  vec3 lightDir = normalize(lights[i].position - FragPos);\n"
            "  float diff = max(dot(norm, lightDir), 0.0);\n"
            "  vec3 diffuse = diff * baseColor;\n"
            "  vec3 reflectDir = reflect(-lightDir, norm);\n"
            "  float spec = pow(max(dot(viewDir, reflectDir),0.0), material.shininess);\n"
            "  vec3 specular = material.specular * spec;\n"
            "  float d = length(lights[i].position - FragPos);\n"
            "  float att = 1.0 / (lights[i].constant + lights[i].linear*d + lights[i].quadratic*d*d);\n"
            "  result += (diffuse + specular) * lights[i].color * att;\n"
            "}\n"
            "vec3 sunDir = normalize(-dirLight.direction);\n"
            "float diffSun = max(dot(norm, sunDir), 0.0);\n"
            "vec3 diffuseSun = diffSun * baseColor;\n"
            "vec3 reflectSun = reflect(-sunDir, norm);\n"
            "float specSun = pow(max(dot(viewDir, reflectSun),0.0), material.shininess);\n"
            "vec3 specularSun = material.specular * specSun;\n"
            "result += (diffuseSun + specularSun) * dirLight.color;\n"
            "float dist = length(viewPos - FragPos) - fogStart;\n"
            "float fogFactor = exp(-fogDensity*max(dist,0.0));\n"
            "fogFactor = clamp(fogFactor,0.0,1.0);\n"
            "result = mix(fogColor, result, fogFactor);\n"
            "FragColor = vec4(result, 1.0);\n"
            "}";

        return buildProgram(vsrc, fsrc, false);
    } else {
        vsrc =
            "#version 120\n"
            "attribute vec3 aPos;\n"
            "attribute vec3 aNormal;\n"
            "attribute vec2 aTex;\n"
            "varying vec3 FragPos;\n"
            "varying vec3 Normal;\n"
            "varying vec2 TexCoord;\n"
            "uniform mat4 model;\n"
            "uniform mat4 view;\n"
            "uniform mat4 projection;\n"
            "void main(){\n"
            "FragPos = vec3(model*vec4(aPos,1.0));\n"
            "Normal = mat3(transpose(inverse(model)))*aNormal;\n"
            "TexCoord=aTex;\n"
            "gl_Position = projection*view*vec4(FragPos,1.0);\n"
            "}";

        fsrc =
            "#version 120\n"
            "struct Material{ sampler2D diffuse; vec3 ambient; vec3 diffuseColor; vec3 specular; float shininess; int useTexture; };\n"
            "struct Light{ vec3 position; vec3 color; float constant; float linear; float quadratic; };\n"
            "struct DirLight{ vec3 direction; vec3 color; };\n"
            "varying vec3 FragPos;\n"
            "varying vec3 Normal;\n"
            "varying vec2 TexCoord;\n"
            "uniform Material material;\n"
            "uniform Light lights[3];\n"
            "uniform DirLight dirLight;\n"
            "uniform vec3 viewPos;\n"
            "uniform vec3 fogColor;\n"
            "uniform float fogDensity;\n"
            "uniform float fogStart;\n"
            "void main(){\n"
            "vec3 norm = normalize(Normal);\n"
            "vec3 viewDir = normalize(viewPos - FragPos);\n"
            "vec3 texCol = material.useTexture == 1 ? texture2D(material.diffuse, TexCoord).rgb : vec3(1.0);\n"
            "vec3 baseColor = material.diffuseColor * texCol;\n"
            "vec3 ambient = material.ambient * texCol;\n"
            "vec3 result = ambient;\n"
            "for(int i=0;i<3;++i){\n"
            "  vec3 lightDir = normalize(lights[i].position - FragPos);\n"
            "  float diff = max(dot(norm, lightDir), 0.0);\n"
            "  vec3 diffuse = diff * baseColor;\n"
            "  vec3 reflectDir = reflect(-lightDir, norm);\n"
            "  float spec = pow(max(dot(viewDir, reflectDir),0.0), material.shininess);\n"
            "  vec3 specular = material.specular * spec;\n"
            "  float d = length(lights[i].position - FragPos);\n"
            "  float att = 1.0 / (lights[i].constant + lights[i].linear*d + lights[i].quadratic*d*d);\n"
            "  result += (diffuse + specular) * lights[i].color * att;\n"
            "}\n"
            "vec3 sunDir = normalize(-dirLight.direction);\n"
            "float diffSun = max(dot(norm, sunDir), 0.0);\n"
            "vec3 diffuseSun = diffSun * baseColor;\n"
            "vec3 reflectSun = reflect(-sunDir, norm);\n"
            "float specSun = pow(max(dot(viewDir, reflectSun),0.0), material.shininess);\n"
            "vec3 specularSun = material.specular * specSun;\n"
            "result += (diffuseSun + specularSun) * dirLight.color;\n"
            "float dist = length(viewPos - FragPos) - fogStart;\n"
            "float fogFactor = exp(-fogDensity*max(dist,0.0));\n"
            "fogFactor = clamp(fogFactor,0.0,1.0);\n"
            "result = mix(fogColor, result, fogFactor);\n"
            "gl_FragColor = vec4(result, 1.0);\n"
            "}";

        return buildProgram(vsrc, fsrc, true);
    }
}

UniformLocations getUniformLocations(GLuint program){
    UniformLocations loc{};
    loc.modelLoc = glGetUniformLocation(program, "model");
    loc.viewLoc = glGetUniformLocation(program, "view");
    loc.projLoc = glGetUniformLocation(program, "projection");
    for(int i=0;i<3;++i){
        std::string base = "lights[" + std::to_string(i) + "]";
        loc.lightPosLoc[i] = glGetUniformLocation(program, (base+".position").c_str());
        loc.lightColorLoc[i] = glGetUniformLocation(program, (base+".color").c_str());
        loc.lightConstLoc[i] = glGetUniformLocation(program, (base+".constant").c_str());
        loc.lightLinLoc[i] = glGetUniformLocation(program, (base+".linear").c_str());
        loc.lightQuadLoc[i] = glGetUniformLocation(program, (base+".quadratic").c_str());
    }
    loc.dirLightDirLoc = glGetUniformLocation(program, "dirLight.direction");
    loc.dirLightColorLoc = glGetUniformLocation(program, "dirLight.color");
    loc.viewPosLoc = glGetUniformLocation(program, "viewPos");
    loc.fogColorLoc = glGetUniformLocation(program, "fogColor");
    loc.fogDensityLoc = glGetUniformLocation(program, "fogDensity");
    loc.fogStartLoc = glGetUniformLocation(program, "fogStart");
    loc.matDiffuseLoc = glGetUniformLocation(program, "material.diffuse");
    loc.matAmbientLoc = glGetUniformLocation(program, "material.ambient");
    loc.matDiffuseColorLoc = glGetUniformLocation(program, "material.diffuseColor");
    loc.matSpecularLoc = glGetUniformLocation(program, "material.specular");
    loc.matShineLoc = glGetUniformLocation(program, "material.shininess");
    loc.matUseTexLoc = glGetUniformLocation(program, "material.useTexture");
    return loc;
}

// Gera VAO/VBO para desenhar opcionalmente a curva
void createCurveBuffers(const std::vector<glm::vec3>& points, GLuint& vao, GLuint& vbo){
    vao = vbo = 0;
    if(points.empty()) return;
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, points.size()*sizeof(glm::vec3), points.data(), GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,sizeof(glm::vec3),(void*)0);
    glVertexAttrib3f(1,0.0f,1.0f,0.0f);
    glVertexAttrib2f(2,0.0f,0.0f);
    glBindVertexArray(0);
}

void setupDirectionalLight(GLint dirLoc, GLint colorLoc, const glm::vec3& dir, const glm::vec3& color){
    glUniform3fv(dirLoc, 1, glm::value_ptr(dir));
    glUniform3fv(colorLoc, 1, glm::value_ptr(color));
}

void setupInitialLights(const UniformLocations& loc, const glm::vec3 lightPositions[3],
                        float attLin, float attQuad, float fogDensity, float fogStart){
    const glm::vec3 lightColors[3] = { glm::vec3(1.0f), glm::vec3(1.0f), glm::vec3(1.0f) };
    for(int i=0;i<3;++i){
        glUniform3fv(loc.lightPosLoc[i],1,glm::value_ptr(lightPositions[i]));
        glUniform3fv(loc.lightColorLoc[i],1,glm::value_ptr(lightColors[i]));
        glUniform1f(loc.lightConstLoc[i],1.0f);
        glUniform1f(loc.lightLinLoc[i],attLin);
        glUniform1f(loc.lightQuadLoc[i],attQuad);
    }
    glm::vec3 sunDir = glm::normalize(glm::vec3(-0.3f,-1.0f,-0.3f));
    glm::vec3 sunColor(0.4f,0.45f,0.5f);
    setupDirectionalLight(loc.dirLightDirLoc, loc.dirLightColorLoc, sunDir, sunColor);
    glUniform3fv(loc.fogColorLoc,1,glm::value_ptr(clearColor));
    glUniform1f(loc.fogDensityLoc,fogDensity);
    glUniform1f(loc.fogStartLoc,fogStart);
    glUniform1i(loc.matDiffuseLoc,0);
}

void sendFrameUniforms(const UniformLocations& loc, const glm::mat4& view,
                       const glm::mat4& proj, const glm::vec3& cam,
                       const glm::vec3 lightPositions[3], float attLin, float attQuad,
                       float fogDensity, float fogStart){
    glUniformMatrix4fv(loc.viewLoc,1,GL_FALSE,glm::value_ptr(view));
    glUniformMatrix4fv(loc.projLoc,1,GL_FALSE,glm::value_ptr(proj));
    glUniform3fv(loc.viewPosLoc,1,glm::value_ptr(cam));
    for(int i=0;i<3;++i){
        glUniform3fv(loc.lightPosLoc[i],1,glm::value_ptr(lightPositions[i]));
        glUniform1f(loc.lightLinLoc[i], attLin);
        glUniform1f(loc.lightQuadLoc[i], attQuad);
    }
    glUniform1f(loc.fogDensityLoc, fogDensity);
    glUniform1f(loc.fogStartLoc, fogStart);
}


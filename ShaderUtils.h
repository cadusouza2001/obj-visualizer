#ifndef SHADER_UTILS_H
#define SHADER_UTILS_H

#include <string>
#include <vector>
#include <glm/glm.hpp>
#include <GL/glew.h>

// Cor de fundo padrao da cena
extern const glm::vec3 clearColor;

struct UniformLocations {
    GLint modelLoc;
    GLint viewLoc;
    GLint projLoc;
    GLint lightPosLoc[3];
    GLint lightColorLoc[3];
    GLint lightConstLoc[3];
    GLint lightLinLoc[3];
    GLint lightQuadLoc[3];
    GLint dirLightDirLoc;
    GLint dirLightColorLoc;
    GLint viewPosLoc;
    GLint fogColorLoc;
    GLint fogDensityLoc;
    GLint fogStartLoc;
    GLint matDiffuseLoc;
    GLint matAmbientLoc;
    GLint matDiffuseColorLoc;
    GLint matSpecularLoc;
    GLint matShineLoc;
    GLint matUseTexLoc;
};

GLuint loadTexture(const std::string& file);
GLuint createShaderProgram();
UniformLocations getUniformLocations(GLuint program);
void createCurveBuffers(const std::vector<glm::vec3>& points, GLuint& vao, GLuint& vbo);
void setupDirectionalLight(GLint dirLoc, GLint colorLoc, const glm::vec3& dir, const glm::vec3& color);
void setupInitialLights(const UniformLocations& loc, const glm::vec3 lightPositions[3],
                        float attLin, float attQuad, float fogDensity, float fogStart);
void sendFrameUniforms(const UniformLocations& loc, const glm::mat4& view,
                       const glm::mat4& proj, const glm::vec3& cam,
                       const glm::vec3 lightPositions[3], float attLin, float attQuad,
                       float fogDensity, float fogStart);

#endif

#ifndef LIGHT_CONTROLS_H
#define LIGHT_CONTROLS_H

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

extern bool ambientEnabled;
extern bool diffuseEnabled;
extern bool specularEnabled;
extern float attLinear;
extern float attQuadratic;
extern float currentFogDensity;
extern const float fogStartDistance;

void toggleAmbientLighting();
void toggleDiffuseLighting();
void toggleSpecularLighting();
void adjustAttenuation(float delta);
void adjustFogDensity(float delta);
void handleLightingKeys(GLFWwindow* win);
void updateLightPositions(const glm::vec3& carPos, glm::vec3 lightPositions[3], float dt);

#endif

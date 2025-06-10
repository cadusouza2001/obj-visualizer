#include "LightControls.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <cmath>
#include <glm/glm.hpp>

bool ambientEnabled = true;
bool diffuseEnabled = true;
bool specularEnabled = true;
float attLinear = 0.045f;
float attQuadratic = 0.0075f;
float currentFogDensity = 0.05f;
const float fogStartDistance = 5.0f;

static bool key1Pressed = false;
static bool key2Pressed = false;
static bool key3Pressed = false;

static float lightOrbitAngle = 0.0f;
static const float lightOrbitRadius = 5.0f;
static const float lightOrbitHeight = 5.0f;
static const float lightOrbitSpeed = glm::radians(90.0f);

void toggleAmbientLighting(){ ambientEnabled = !ambientEnabled; }
void toggleDiffuseLighting(){ diffuseEnabled = !diffuseEnabled; }
void toggleSpecularLighting(){ specularEnabled = !specularEnabled; }
void adjustAttenuation(float delta){
    attLinear = glm::max(0.0f, attLinear + delta);
    attQuadratic = glm::max(0.0f, attQuadratic + delta);
}
void adjustFogDensity(float delta){
    currentFogDensity = glm::max(0.0f, currentFogDensity + delta);
}

void handleLightingKeys(GLFWwindow* win){
    if(glfwGetKey(win, GLFW_KEY_1) == GLFW_PRESS && !key1Pressed){
        toggleAmbientLighting();
        key1Pressed = true;
    }
    if(glfwGetKey(win, GLFW_KEY_1) == GLFW_RELEASE)
        key1Pressed = false;

    if(glfwGetKey(win, GLFW_KEY_2) == GLFW_PRESS && !key2Pressed){
        toggleDiffuseLighting();
        key2Pressed = true;
    }
    if(glfwGetKey(win, GLFW_KEY_2) == GLFW_RELEASE)
        key2Pressed = false;

    if(glfwGetKey(win, GLFW_KEY_3) == GLFW_PRESS && !key3Pressed){
        toggleSpecularLighting();
        key3Pressed = true;
    }
    if(glfwGetKey(win, GLFW_KEY_3) == GLFW_RELEASE)
        key3Pressed = false;

    if(glfwGetKey(win, GLFW_KEY_EQUAL) == GLFW_PRESS || glfwGetKey(win, GLFW_KEY_KP_ADD) == GLFW_PRESS)
        adjustAttenuation(0.005f);
    if(glfwGetKey(win, GLFW_KEY_MINUS) == GLFW_PRESS || glfwGetKey(win, GLFW_KEY_KP_SUBTRACT) == GLFW_PRESS)
        adjustAttenuation(-0.005f);

    if(glfwGetKey(win, GLFW_KEY_N) == GLFW_PRESS)
        adjustFogDensity(-0.005f);
    if(glfwGetKey(win, GLFW_KEY_M) == GLFW_PRESS)
        adjustFogDensity(0.005f);
}

void updateLightPositions(const glm::vec3& carPos, glm::vec3 lightPositions[3], float dt){
    lightOrbitAngle -= lightOrbitSpeed * dt;
    for(int i=0;i<3;++i){
        float ang = lightOrbitAngle + i * 2.0f * 3.14159265f / 3.0f;
        lightPositions[i] = carPos + glm::vec3(cos(ang)*lightOrbitRadius, lightOrbitHeight, sin(ang)*lightOrbitRadius);
    }
}


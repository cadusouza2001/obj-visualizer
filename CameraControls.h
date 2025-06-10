#ifndef CAMERA_CONTROLS_H
#define CAMERA_CONTROLS_H

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <vector>
#include "SceneLoader.h"

extern glm::vec3 camPos;
extern float yaw;
extern float pitch;

void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
void cursorPosCallback(GLFWwindow* window, double xpos, double ypos);

void handleCameraToggle(GLFWwindow* win, bool& cameraSwitched);
float deltaTime(float& last);
glm::vec3 updateCamera(GLFWwindow* win, const glm::vec3& carPos, const glm::vec3& carFront, float dt);

#endif

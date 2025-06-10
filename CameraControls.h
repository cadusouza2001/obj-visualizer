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
void handleCurveKeys(GLFWwindow* win, bool& showCurve, bool& fPressed, size_t& pathIdx);

float deltaTime(float& last);

void animateCarAlongCurve(Obj3D& car, const std::vector<glm::vec3>& pts, size_t& idx, float dt);
glm::vec3 updateCamera(GLFWwindow* win, const glm::vec3& carPos, const glm::vec3& carFront, float dt);

#endif

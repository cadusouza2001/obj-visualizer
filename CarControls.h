#ifndef CAR_CONTROLS_H
#define CAR_CONTROLS_H

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <vector>
#include "SceneLoader.h"

extern bool manualDriving;

void handleDriveToggle(GLFWwindow* win, Obj3D& car,
                       const std::vector<glm::vec3>& curve,
                       size_t& pathIdx);

void updateManualDrive(GLFWwindow* win, Obj3D& car, float dt);

#endif

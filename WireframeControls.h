#ifndef WIREFRAME_CONTROLS_H
#define WIREFRAME_CONTROLS_H

#include <GL/glew.h>
#include <GLFW/glfw3.h>

// Flag global indicando se a visualização wireframe está habilitada
extern bool wireframeEnabled;

// Trata o input da tecla T para alternar entre GL_LINE e GL_FILL
void handleWireframeToggle(GLFWwindow* win);

#endif

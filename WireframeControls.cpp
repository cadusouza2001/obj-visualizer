#include "WireframeControls.h"

// Estado inicial: modo preenchido (GL_FILL)
bool wireframeEnabled = false;

// Lida com a tecla T para alternar a visualização da malha
// GL_LINE renderiza apenas as bordas dos triângulos, permitindo
// entender a estrutura da malha após a triangulação automática.
// Ú til como ferramenta de depuração ou visualização adicional.
void handleWireframeToggle(GLFWwindow* win){
    static bool tPressed = false; // garante uma troca por press
    bool tDown = glfwGetKey(win, GLFW_KEY_T) == GLFW_PRESS;
    if(tDown && !tPressed){
        wireframeEnabled = !wireframeEnabled;
        glPolygonMode(GL_FRONT_AND_BACK, wireframeEnabled ? GL_LINE : GL_FILL);
        tPressed = true;
    }
    if(!tDown)
        tPressed = false;
}

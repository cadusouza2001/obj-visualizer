#include "CameraControls.h"
#include "LightControls.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>

// Estado da camera controlada pelo mouse e teclado
glm::vec3 camPos(524.0f, 0.0f, 212.0f); // posicao inicial
float yaw = -90.0f;                 // angulo Yaw (olhando para -Z)
float pitch = 0.0f;                 // angulo Pitch
static bool rotating = false;              // se o botao esquerdo do mouse esta pressionado
static double lastX = 0.0, lastY = 0.0;    // ultima posicao do mouse
static bool freeCamera = false;            // false: camera segue o carro
static bool oPressed = false;              // controle da tecla O
static const float chaseDistance = 5.0f;   // dist. da camera atras do carro
static const float chaseHeight = 2.0f;     // altura da camera em relacao ao carro
static float carAngleOffset = glm::radians(-90.0f); // offset angular aplicado ao carro (radianos)
static float orbitYaw = 0.0f;              // angulo horizontal ao orbitar o carro
static float orbitPitch = 0.0f;            // angulo vertical ao orbitar o carro

void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
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

void cursorPosCallback(GLFWwindow* window, double xpos, double ypos) {
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


void handleCameraToggle(GLFWwindow* win, bool& cameraSwitched){
    bool oDown = glfwGetKey(win, GLFW_KEY_O) == GLFW_PRESS;
    if(oDown && !oPressed){
        freeCamera = !freeCamera;
        oPressed = true;
        rotating = false;
        cameraSwitched = true;
    }
    if(!oDown) oPressed = false;
    if(!freeCamera && cameraSwitched){
        orbitYaw = 0.0f;
        orbitPitch = 0.0f;
        cameraSwitched = false;
    }
}

void handleCurveKeys(GLFWwindow* win, bool& showCurve, bool& fPressed, size_t& pathIdx){
    if(glfwGetKey(win, GLFW_KEY_F) == GLFW_PRESS && !fPressed){
        showCurve = !showCurve;
        fPressed = true;
    }
    if(glfwGetKey(win, GLFW_KEY_F) == GLFW_RELEASE)
        fPressed = false;
    if(glfwGetKey(win, GLFW_KEY_R) == GLFW_PRESS)
        pathIdx = 0;
}

float deltaTime(float& last){
    float now = (float)glfwGetTime();
    float dt = now - last;
    last = now;
    return dt;
}

void animateCarAlongCurve(Obj3D& car, const std::vector<glm::vec3>& pts, size_t& idx, float dt){
    if (pts.size() < 2) return;
    static float segT = 0.0f;
    const float speed = 15.0f;
    segT += dt * speed;
    while (segT >= 1.0f) {
        segT -= 1.0f;
        idx = (idx + 1) % pts.size();
    }
    size_t next = (idx + 1) % pts.size();
    glm::vec3 pos = glm::mix(pts[idx], pts[next], segT);
    glm::vec3 dir = glm::normalize(pts[next] - pts[idx]);
    float pathAngle = atan2(dir.z, dir.x);
    glm::mat4 model = glm::translate(glm::mat4(1.0f), pos);
    model = glm::rotate(model, - pathAngle + carAngleOffset, glm::vec3(0, 1, 0));
    car.transform = model;
}

glm::vec3 updateCamera(GLFWwindow* win, const glm::vec3& carPos, const glm::vec3& carFront, float dt){
    glm::vec3 front;
    if(!freeCamera){
        const float orbitSpeed = 90.0f;
        if(glfwGetKey(win,GLFW_KEY_W)==GLFW_PRESS || glfwGetKey(win,GLFW_KEY_UP)==GLFW_PRESS) orbitPitch += orbitSpeed*dt;
        if(glfwGetKey(win,GLFW_KEY_S)==GLFW_PRESS || glfwGetKey(win,GLFW_KEY_DOWN)==GLFW_PRESS) orbitPitch -= orbitSpeed*dt;
        if(glfwGetKey(win,GLFW_KEY_A)==GLFW_PRESS || glfwGetKey(win,GLFW_KEY_LEFT)==GLFW_PRESS) orbitYaw += orbitSpeed*dt;
        if(glfwGetKey(win,GLFW_KEY_D)==GLFW_PRESS || glfwGetKey(win,GLFW_KEY_RIGHT)==GLFW_PRESS) orbitYaw -= orbitSpeed*dt;
        orbitPitch = glm::clamp(orbitPitch,-89.0f,89.0f);
        glm::vec3 baseDir = -carFront;
        glm::mat4 rotYaw = glm::rotate(glm::mat4(1.0f), glm::radians(orbitYaw),glm::vec3(0,1,0));
        glm::vec3 dir = glm::vec3(rotYaw * glm::vec4(baseDir,0.0f));
        glm::vec3 rightAxis = glm::normalize(glm::cross(dir, glm::vec3(0,1,0)));
        glm::mat4 rotPitch = glm::rotate(glm::mat4(1.0f), glm::radians(orbitPitch), rightAxis);
        dir = glm::normalize(glm::vec3(rotPitch * glm::vec4(dir,0.0f)));
        camPos = carPos + dir * chaseDistance + glm::vec3(0, chaseHeight, 0);
        front = glm::normalize(carPos - camPos);
        yaw = glm::degrees(atan2(front.z, front.x));
        pitch = glm::degrees(asin(front.y));
    }else{
        front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
        front.y = sin(glm::radians(pitch));
        front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
        front = glm::normalize(front);
        glm::vec3 right = glm::normalize(glm::cross(front, glm::vec3(0,1,0)));
        float cameraSpeed = 15.0f;
        if(glfwGetKey(win,GLFW_KEY_W)==GLFW_PRESS || glfwGetKey(win,GLFW_KEY_UP)==GLFW_PRESS) camPos += front * dt * cameraSpeed;
        if(glfwGetKey(win,GLFW_KEY_S)==GLFW_PRESS || glfwGetKey(win,GLFW_KEY_DOWN)==GLFW_PRESS) camPos -= front * dt * cameraSpeed;
        if(glfwGetKey(win,GLFW_KEY_A)==GLFW_PRESS || glfwGetKey(win,GLFW_KEY_LEFT)==GLFW_PRESS) camPos -= right * dt * cameraSpeed;
        if(glfwGetKey(win,GLFW_KEY_D)==GLFW_PRESS || glfwGetKey(win,GLFW_KEY_RIGHT)==GLFW_PRESS) camPos += right * dt * cameraSpeed;
    }
    return front;
}


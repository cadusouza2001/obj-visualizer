#include "CarControls.h"
#include "CameraControls.h"
#include <glm/gtc/matrix_transform.hpp>
#include <cmath>

bool manualDriving = false;

static bool pPressed = false;
static glm::vec3 drivePos(0.0f);
static float driveYaw = 0.0f;

static const float moveSpeed = 15.0f;
static const float turnSpeed = glm::radians(90.0f);
static const float carAngleOffset = glm::radians(-90.0f);

void handleDriveToggle(GLFWwindow* win, Obj3D& car,
                       const std::vector<glm::vec3>& curve,
                       size_t& pathIdx){
    if(glfwGetKey(win, GLFW_KEY_P) == GLFW_PRESS && !pPressed){
        manualDriving = !manualDriving;
        pPressed = true;
        if(manualDriving){
            forceChaseCamera();
            drivePos = glm::vec3(car.transform[3]);
            glm::vec3 front = glm::normalize(glm::vec3(car.transform * glm::vec4(0,0,-1,0)));
            driveYaw = atan2(front.z, front.x);
        }else{
            forceChaseCamera();
            resetCarAnimation();
            pathIdx = 0;
            if(!curve.empty()){
                drivePos = curve[0];
                glm::vec3 dir = glm::normalize(curve.size()>1 ? curve[1]-curve[0] : glm::vec3(1,0,0));
                driveYaw = atan2(dir.z, dir.x);
            }
            glm::mat4 model = glm::translate(glm::mat4(1.0f), drivePos);
            model = glm::rotate(model, -driveYaw + carAngleOffset, glm::vec3(0,1,0));
            car.transform = model;
        }
    }
    if(glfwGetKey(win, GLFW_KEY_P) == GLFW_RELEASE)
        pPressed = false;
}

void updateManualDrive(GLFWwindow* win, Obj3D& car, float dt){
    if(!manualDriving) return;

    bool w = glfwGetKey(win, GLFW_KEY_W)==GLFW_PRESS || glfwGetKey(win, GLFW_KEY_UP)==GLFW_PRESS;
    bool s = glfwGetKey(win, GLFW_KEY_S)==GLFW_PRESS || glfwGetKey(win, GLFW_KEY_DOWN)==GLFW_PRESS;
    bool a = glfwGetKey(win, GLFW_KEY_A)==GLFW_PRESS || glfwGetKey(win, GLFW_KEY_LEFT)==GLFW_PRESS;
    bool d = glfwGetKey(win, GLFW_KEY_D)==GLFW_PRESS || glfwGetKey(win, GLFW_KEY_RIGHT)==GLFW_PRESS;

    glm::vec3 forward(cos(driveYaw), 0.0f, sin(driveYaw));

    if(w) drivePos += forward * dt * moveSpeed;
    if(s) drivePos -= forward * dt * moveSpeed;

    if(w){
        if(a) driveYaw -= turnSpeed * dt;
        if(d) driveYaw += turnSpeed * dt;
    }else if(s){
        if(a) driveYaw += turnSpeed * dt;
        if(d) driveYaw -= turnSpeed * dt;
    }


    glm::mat4 model = glm::translate(glm::mat4(1.0f), drivePos);
    model = glm::rotate(model, -driveYaw + carAngleOffset, glm::vec3(0,1,0));
    car.transform = model;
}



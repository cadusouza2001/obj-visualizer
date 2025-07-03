#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <vector>

#include "Mesh.h"
#include "Group.h"
#include "Face.h"
#include "SceneLoader.h"
#include "ShaderUtils.h"
#include "CameraControls.h"
#include "LightControls.h"
#include "WireframeControls.h"
#include "CarControls.h"

// Inicializa GLFW, cria janela e prepara GLEW. Retorna ponteiro para a janela.
static GLFWwindow* initWindow(){
    if (!glfwInit()) { std::cerr << "Failed to init GLFW\n";return nullptr; }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    GLFWwindow* win = glfwCreateWindow(800, 600, "OpenGL", nullptr, nullptr);
    if (!win) { glfwTerminate();return nullptr; }
    glfwMakeContextCurrent(win);
    glewExperimental = true;
    if (glewInit() != GLEW_OK) {
        std::cerr << "GLEW init failed\n";
        glfwTerminate();
        return nullptr;
    }
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glfwSetMouseButtonCallback(win, mouseButtonCallback);
    glfwSetCursorPosCallback(win, cursorPosCallback);
    return win;
}

// Desenha todos os objetos da cena
static void renderObjects(const std::vector<Obj3D>& scene, const UniformLocations& loc){
    // Primeiro desenha materiais opacos
    for(const Obj3D& obj : scene){
        glUniformMatrix4fv(loc.modelLoc,1,GL_FALSE,glm::value_ptr(obj.transform));
        for(Group* g : obj.mesh->groups){
            auto it = obj.materials.find(g->material);
            MaterialInfo mat; if(it != obj.materials.end()) mat = it->second;
            if(mat.alpha < 1.0f) continue;
            glm::vec3 Ka = ambientEnabled  ? mat.Ka : glm::vec3(0.0f);
            glm::vec3 Kd = diffuseEnabled  ? mat.Kd : glm::vec3(0.0f);
            glm::vec3 Ks = specularEnabled ? mat.Ks : glm::vec3(0.0f);
            glUniform3fv(loc.matAmbientLoc,1,glm::value_ptr(Ka));
            glUniform3fv(loc.matDiffuseColorLoc,1,glm::value_ptr(Kd));
            glUniform3fv(loc.matSpecularLoc,1,glm::value_ptr(Ks));
            glUniform1f(loc.matShineLoc, mat.Ns);
            glUniform1f(loc.matAlphaLoc, mat.alpha);
            glUniform1i(loc.matUseTexLoc, mat.texture ? 1 : 0);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, mat.texture);
            glBindVertexArray(g->vao);
            glDrawArrays(GL_TRIANGLES,0,g->numVertices);
        }
    }
    // Depois, desenha objetos transparentes
    for(const Obj3D& obj : scene){
        glUniformMatrix4fv(loc.modelLoc,1,GL_FALSE,glm::value_ptr(obj.transform));
        for(Group* g : obj.mesh->groups){
            auto it = obj.materials.find(g->material);
            MaterialInfo mat; if(it != obj.materials.end()) mat = it->second;
            if(mat.alpha >= 1.0f) continue;
            glm::vec3 Ka = ambientEnabled  ? mat.Ka : glm::vec3(0.0f);
            glm::vec3 Kd = diffuseEnabled  ? mat.Kd : glm::vec3(0.0f);
            glm::vec3 Ks = specularEnabled ? mat.Ks : glm::vec3(0.0f);
            glUniform3fv(loc.matAmbientLoc,1,glm::value_ptr(Ka));
            glUniform3fv(loc.matDiffuseColorLoc,1,glm::value_ptr(Kd));
            glUniform3fv(loc.matSpecularLoc,1,glm::value_ptr(Ks));
            glUniform1f(loc.matShineLoc, mat.Ns);
            glUniform1f(loc.matAlphaLoc, mat.alpha);
            glUniform1i(loc.matUseTexLoc, mat.texture ? 1 : 0);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, mat.texture);
            glBindVertexArray(g->vao);
            glDrawArrays(GL_TRIANGLES,0,g->numVertices);
        }
    }
}

// Opcionalmente desenha a linha da curva usada na animacao
static void renderCurve(GLuint vao, const std::vector<glm::vec3>& points, const UniformLocations& loc, bool enabled){
    if(!enabled || vao==0) return;
    MaterialInfo dbg; dbg.Ka = glm::vec3(1,0,0); dbg.Kd = dbg.Ka; dbg.Ks = glm::vec3(0); dbg.Ns = 1.0f; dbg.texture = 0; dbg.alpha = 1.0f;
    glUniformMatrix4fv(loc.modelLoc,1,GL_FALSE,glm::value_ptr(glm::mat4(1.0f)));
    glUniform3fv(loc.matAmbientLoc,1,glm::value_ptr(dbg.Ka));
    glUniform3fv(loc.matDiffuseColorLoc,1,glm::value_ptr(dbg.Kd));
    glUniform3fv(loc.matSpecularLoc,1,glm::value_ptr(dbg.Ks));
    glUniform1f(loc.matShineLoc, dbg.Ns);
    glUniform1f(loc.matAlphaLoc, dbg.alpha);
    glUniform1i(loc.matUseTexLoc,0);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D,0);
    glBindVertexArray(vao);
    glDrawArrays(GL_LINE_STRIP,0,(GLsizei)points.size());
}

// Loop principal organizado em chamadas menores
static void run(GLFWwindow* win, GLuint program, std::vector<Obj3D>& scene,
                const std::vector<glm::vec3>& curvePoints, GLuint curveVAO,
                const UniformLocations& loc){
    size_t carIndex = scene.size() > 1 ? 1 : 0;
    size_t pathIndex = 0;
    bool showCurve = false;
    bool fPressed = false;
    float last = (float)glfwGetTime();
    glm::vec3 lightPositions[3] = { glm::vec3(0.0f) };
    if(!curvePoints.empty()){
        lightPositions[0] = curvePoints[0] + glm::vec3(0,5,0);
        lightPositions[1] = curvePoints[curvePoints.size()/2] + glm::vec3(0,5,0);
        lightPositions[2] = curvePoints.back() + glm::vec3(0,5,0);
    }
    setupInitialLights(loc, lightPositions, attLinear, attQuadratic, currentFogDensity, fogStartDistance);

    while(!glfwWindowShouldClose(win)){
        float dt = deltaTime(last);
        bool camSwitched = false;
        handleCameraToggle(win, camSwitched);
        handleCurveKeys(win, showCurve, fPressed, pathIndex);
        handleDriveToggle(win, scene[carIndex], curvePoints, pathIndex);
        handleLightingKeys(win);                // processa teclas de iluminacao
        handleWireframeToggle(win);             // alterna modo wireframe
        if(carIndex < scene.size()){
            if(manualDriving)
                updateManualDrive(win, scene[carIndex], dt);
            else
                animateCarAlongCurve(scene[carIndex], curvePoints, pathIndex, dt);
        }
        glm::vec3 carPos = glm::vec3(scene[carIndex].transform[3]);
        glm::vec3 carFront = glm::normalize(glm::vec3(scene[carIndex].transform * glm::vec4(0,0,-1,0)));
        glm::vec3 front = updateCamera(win, carPos, carFront, dt);
        updateLightPositions(carPos, lightPositions, dt);
        if(glfwGetKey(win, GLFW_KEY_ESCAPE) == GLFW_PRESS)
            glfwSetWindowShouldClose(win,1);
        glm::mat4 view = glm::lookAt(camPos, camPos + front, glm::vec3(0,1,0));
        glm::mat4 proj = glm::perspective(glm::radians(45.0f), 800.0f/600.0f, 0.1f, 1000.0f);

        glClearColor(clearColor.r, clearColor.g, clearColor.b, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glUseProgram(program);
        sendFrameUniforms(loc, view, proj, camPos, lightPositions, attLinear, attQuadratic, currentFogDensity, fogStartDistance);
        renderObjects(scene, loc);
        renderCurve(curveVAO, curvePoints, loc, showCurve);
        glBindVertexArray(0);
        glfwSwapBuffers(win);
        glfwPollEvents();
    }
}

int main(){
    GLFWwindow* win = initWindow();
    if(!win) return -1;
    GLuint program = createShaderProgram();
    glUseProgram(program);
    SceneConfig cfg{};
    if(!loadSceneConfig("scene.txt", cfg)) return -1;
    if(cfg.curveFile.empty() || cfg.objFiles.empty()) return -1;
    std::vector<glm::vec3> curvePoints; loadCurve(cfg.curveFile, curvePoints);
    GLuint curveVAO=0, curveVBO=0; createCurveBuffers(curvePoints, curveVAO, curveVBO);
    std::vector<Obj3D> scene = loadSceneObjects(cfg.objFiles);
    UniformLocations loc = getUniformLocations(program);
    run(win, program, scene, curvePoints, curveVAO, loc);
    glfwTerminate();
    return 0;
}


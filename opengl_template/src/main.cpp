// ----------------------------------------------------------------------------
// main.cpp
//
//  Created on: 24 Jul 2020
//      Author: Kiwon Um
//        Mail: kiwon.um@telecom-paris.fr
//
// Description: IGR201 Practical; OpenGL and Shaders (DO NOT distribute!)
//
// Copyright 2020 Kiwon Um
//
// The copyright to the computer program(s) herein is the property of Kiwon Um,
// Telecom Paris, France. The program(s) may be used and/or copied only with
// the written permission of Kiwon Um or in accordance with the terms and
// conditions stipulated in the agreement/contract under which the program(s)
// have been supplied.
// ----------------------------------------------------------------------------

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/ext.hpp>

#include "mesh.h"

#include <cstdlib>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#define _USE_MATH_DEFINES
#include <cmath>
#include <memory>
#include <map>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

// Window parameters
GLFWwindow* g_window = nullptr;

// GPU objects
GLuint object_program = 0;
GLuint lighting_program = 0;


// OpenGL identifiers
GLuint g_vao = 0;
GLuint g_posVbo = 0;
GLuint g_ibo = 0;
GLuint g_colVbo = 0;

// mesh and textures id
std::shared_ptr<Mesh> sphere_mesh;
GLuint g_earthTexID;
GLuint g_moonTexID;
GLuint g_sunTexID;

// information used for camera mode selection
enum spaceObject { outerSpace, sun, earth, moon };
std::map<spaceObject, glm::mat4> modelMatrices;
spaceObject cameraSpaceObject = earth;
spaceObject lookAtSpaceObject = moon;

// constants and vectors to be used in render()
const static float kSizeSun = 1;
const static float kSizeEarth = 0.5;
const static float kSizeMoon = 0.25;
const static float kRadOrbitEarth = 10;
const static float kRadOrbitMoon = 2;
const static float kPeriodeOrbitEarth = 30;
const static float kPeriodeRotEarth = 0.5 * kPeriodeOrbitEarth;
const static float kPeriodeMoon = 0.5 * kPeriodeRotEarth;

const static float kDeviationEarth = glm::radians(23.5f);
const glm::vec3 earthRotationAxe = glm::vec3(sin(kDeviationEarth), 0.0f, cos(kDeviationEarth));

const static glm::vec3 lightColor = glm::vec3(1.0, 1.0, 0.7);

glm::vec3 earthOrbitalMovement = glm::vec3(0.0);
glm::vec3 moonOrbitalMovement = glm::vec3(0.0);
glm::vec3 freeCameraMovement = glm::vec3(0.0);


// Basic camera model
class Camera {
public:
    inline float getFov() const { return m_fov; }
    inline void setFoV(const float f) { m_fov = f; }
    inline float getAspectRatio() const { return m_aspectRatio; }
    inline void setAspectRatio(const float a) { m_aspectRatio = a; }
    inline float getNear() const { return m_near; }
    inline void setNear(const float n) { m_near = n; }
    inline float getFar() const { return m_far; }
    inline void setFar(const float n) { m_far = n; }

    inline void adjustR(const float dt_r) {
        m_r += dt_r;
        if (m_r < 1.0) { m_r = 1.0; }
    }
    inline void adjustPhi(const float dt_phi) {
        m_phi += dt_phi;
        if (m_theta < glm::radians(0.0)) { m_theta = glm::radians(360.0); }
        if (m_theta > glm::radians(360.0)) { m_theta = glm::radians(0.0); }

    }
    inline void adjustTheta(const float dt_theta) {
        m_theta += dt_theta;
        if (m_theta < glm::radians(5.0)) { m_theta = glm::radians(5.0); }
        if (m_theta > glm::radians(175.0)) { m_theta = glm::radians(175.0); }
    }


    inline void setPosition(const glm::vec3& p) { m_pos = p; }
    inline glm::vec3 getPosition() { return m_pos; }
    inline void setLookAtPoint(const glm::vec3& l) { m_lookAtPoint = l; }
    inline void setUpVector(const glm::vec3& u) { m_upVector = u; }

    inline glm::mat4 computeViewMatrix() const {
        return glm::lookAt(m_pos, m_lookAtPoint, m_upVector);
    }

    // Returns the projection matrix stemming from the camera intrinsic parameter.
    inline glm::mat4 computeProjectionMatrix() const {
        return glm::perspective(glm::radians(m_fov), m_aspectRatio, m_near, m_far);
    }

    inline glm::vec3 calculate_camera_pos() {
        return glm::vec3(modelMatrices[lookAtSpaceObject] * glm::vec4(0.0, 0.0, 0.0, 1.0)) + glm::vec3(m_r * sin(m_theta) * cos(m_phi), m_r * sin(m_theta) * sin(m_phi), m_r * cos(m_theta));
    }

private:
    glm::vec3 m_pos = glm::vec3(25.0, 0.0, 0.0);
    glm::vec3 m_lookAtPoint = glm::vec3(0, 0, 0);
    glm::vec3 m_upVector = glm::vec3(0, 0, 1);
    float m_fov = 45.0f;        // Field of view, in degrees
    float m_aspectRatio = 1.f; // Ratio between the width and the height of the image
    float m_near = -3.f; // Distance before which geometry is excluded fromt he rasterization process
    float m_far = 3.f; // Distance after which the geometry is excluded fromt he rasterization process
    float m_r = 25.0;
    float m_theta = glm::radians(90.0);;
    float m_phi = glm::radians(0.0);;

};
Camera g_camera;

GLuint loadTextureFromFileToGPU(const std::string& filename) {
    int width, height, numComponents;
    // Loading the image in CPU memory using stbd_image
    unsigned char* data = stbi_load(
        filename.c_str(),
        &width, &height,
        &numComponents, // 1 for a 8 bit greyscale image, 3 for 24bits RGB image, 4 for 32bits RGBA image
        0);

    GLuint texID; // OpenGL texture identifier
    glGenTextures(1, &texID); // generate an OpenGL texture container
    glBindTexture(GL_TEXTURE_2D, texID); // activate the texture
    // The following lines setup the texture filtering option and repeat mode; check www.opengl.org for details.
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    // fills the GPU texture with the data stored in the CPU image
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);

    // Freeing the now useless CPU memory
    stbi_image_free(data);
    glBindTexture(GL_TEXTURE_2D, 0); // unbind the texture

    return texID;
}

// Executed each time the window is resized. Adjust the aspect ratio and the rendering viewport to the current window.
void windowSizeCallback(GLFWwindow* window, int width, int height) {
    g_camera.setAspectRatio(static_cast<float>(width) / static_cast<float>(height));
    glViewport(0, 0, (GLint)width, (GLint)height); // Dimension of the rendering region in the window
}

// Executed each time a key is entered.
void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
        
    if (action == GLFW_PRESS && key == GLFW_KEY_W) {
        std::cout << "W key pressed: " << "View line Mode" << std::endl;
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    }
    else if (action == GLFW_PRESS && key == GLFW_KEY_F) {
        std::cout << "F key pressed: " << "View fill Mode" << std::endl;
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }
    else if (action == GLFW_PRESS && key == GLFW_KEY_ESCAPE) {
        glfwSetWindowShouldClose(window, true); // Closes the application if the escape key is pressed
    }
    else if (action == GLFW_PRESS && (key == GLFW_KEY_J)) {
        std::cout << "J key pressed: " << "lookAtpoint = earth" << std::endl;
        if (cameraSpaceObject != earth) { lookAtSpaceObject = earth; }
    }
    else if (action == GLFW_PRESS && (key == GLFW_KEY_K)) {
        std::cout << "K key pressed: " << "lookAtpoint = moon" << std::endl;
        if (cameraSpaceObject != moon) { lookAtSpaceObject = moon; }
    }
    else if (action == GLFW_PRESS && (key == GLFW_KEY_L)) {
        std::cout << "L key pressed: " << "lookAtpoint = sun" << std::endl;
        if (cameraSpaceObject != sun) { lookAtSpaceObject = sun; }
    }
    else if (action == GLFW_PRESS && (key == GLFW_KEY_V)) {
        std::cout << "V key pressed: " << "camera position = earth" << std::endl;
        if (lookAtSpaceObject != earth) { cameraSpaceObject = earth; }
    }
    else if (action == GLFW_PRESS && (key == GLFW_KEY_B)) {
        std::cout << "B key pressed: " << "camera position = moon" << std::endl;
        if (lookAtSpaceObject != moon) { cameraSpaceObject = moon; }
    }
    else if (action == GLFW_PRESS && (key == GLFW_KEY_N)) {
        std::cout << "N key pressed: " << "camera position = sun" << std::endl;
        if (lookAtSpaceObject != sun) { cameraSpaceObject = sun; }
    }
    else if (action == GLFW_PRESS && (key == GLFW_KEY_C)) {
        std::cout << "C key pressed: " << "free camera position" << std::endl;
        cameraSpaceObject = outerSpace;
    }
    else if (cameraSpaceObject == outerSpace)
    {
        if ((action == GLFW_REPEAT || action == GLFW_PRESS) && (key == GLFW_KEY_S)) {
            std::cout << "S key pressed: " << "increase radius" << std::endl;
            g_camera.adjustR(1);
        }
        else if ((action == GLFW_REPEAT || action == GLFW_PRESS) && (key == GLFW_KEY_A)) {
            std::cout << "A key pressed: " << "decrease radius" << std::endl;
            g_camera.adjustR(-1);
        }
        else if ((action == GLFW_REPEAT || action == GLFW_PRESS) && (key == GLFW_KEY_UP)) {
            std::cout << "UP key pressed: " << "increase Theta" << std::endl;
            g_camera.adjustTheta(-glm::radians(1.0));
        }
        else if ((action == GLFW_REPEAT || action == GLFW_PRESS) && (key == GLFW_KEY_DOWN)) {
            std::cout << "DOWN key pressed: " << "decrease Theta" << std::endl;
            g_camera.adjustTheta(glm::radians(1.0));
        }
        else if ((action == GLFW_REPEAT || action == GLFW_PRESS) && (key == GLFW_KEY_LEFT)) {
            std::cout << "LEFT key pressed: " << "decrease Phi" << std::endl;
            g_camera.adjustPhi(-glm::radians(1.0));
        }
        else if ((action == GLFW_REPEAT || action == GLFW_PRESS) && (key == GLFW_KEY_RIGHT)) {
            std::cout << "LEFT key pressed: " << "increase Phi" << std::endl;
            g_camera.adjustPhi(glm::radians(1.0));
        }
    }
}

void errorCallback(int error, const char* desc) {
    std::cout << "Error " << error << ": " << desc << std::endl;
}

void initGLFW() {
    glfwSetErrorCallback(errorCallback);

    // Initialize GLFW, the library responsible for window management
    if (!glfwInit()) {
        std::cerr << "ERROR: Failed to init GLFW" << std::endl;
        std::exit(EXIT_FAILURE);
    }

    // Before creating the window, set some option flags
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_RESIZABLE, GL_TRUE);

    // Create the window
    g_window = glfwCreateWindow(
        1024, 768,
        "Interactive 3D Applications (OpenGL) - Simple Solar System",
        nullptr, nullptr);
    if (!g_window) {
        std::cerr << "ERROR: Failed to open window" << std::endl;
        glfwTerminate();
        std::exit(EXIT_FAILURE);
    }

    // Load the OpenGL context in the GLFW window using GLAD OpenGL wrangler
    glfwMakeContextCurrent(g_window);
    glfwSetWindowSizeCallback(g_window, windowSizeCallback);
    glfwSetKeyCallback(g_window, keyCallback);
}

void initOpenGL() {
    // Load extensions for modern OpenGL
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "ERROR: Failed to initialize OpenGL context" << std::endl;
        glfwTerminate();
        std::exit(EXIT_FAILURE);
    }

    glCullFace(GL_BACK); // Specifies the faces to cull (here the ones pointing away from the camera)
    glEnable(GL_CULL_FACE); // Enables face culling (based on the orientation defined by the CW/CCW enumeration).
    glDepthFunc(GL_LESS);   // Specify the depth test for the z-buffer
    glEnable(GL_DEPTH_TEST);      // Enable the z-buffer test in the rasterization
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f); // specify the background color, used any time the framebuffer is cleared
}

// Loads the content of an ASCII file in a standard C++ string
std::string file2String(const std::string& filename) {
    std::ifstream t(filename.c_str());
    std::stringstream buffer;
    buffer << t.rdbuf();
    return buffer.str();
}

// Loads and compile a shader, before attaching it to a program
void loadShader(GLuint program, GLenum type, const std::string& shaderFilename) {
    GLuint shader = glCreateShader(type); // Create the shader, e.g., a vertex shader to be applied to every single vertex of a mesh
    std::string shaderSourceString = file2String(shaderFilename); // Loads the shader source from a file to a C++ string
    const GLchar* shaderSource = (const GLchar*)shaderSourceString.c_str(); // Interface the C++ string through a C pointer
    glShaderSource(shader, 1, &shaderSource, NULL); // load the vertex shader code
    glCompileShader(shader);

    int  success;
    char infoLog[512];
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);

    if (!success)
    {
        glGetShaderInfoLog(shader, 512, NULL, infoLog);
        std::cout << "ERROR::" << shaderFilename << "::COMPILATION_FAILED\n" << infoLog << std::endl;
    }


    glAttachShader(program, shader);
    glDeleteShader(shader);
}


void check_linking(GLuint program) {
    int  success;
    char infoLog[512];
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(program, 512, NULL, infoLog);
        std::cout << "ERROR::" << "linking shaders" << "::COMPILATION_FAILED\n" << infoLog << std::endl;
    }
}


void initGPUprogram() {
    object_program = glCreateProgram(); // Create a GPU program, i.e., two central shaders of the graphics pipeline
    loadShader(object_program, GL_VERTEX_SHADER, "res/shaders/vShaderObject.glsl");
    loadShader(object_program, GL_FRAGMENT_SHADER, "res/shaders/fShaderObject.glsl");
    glLinkProgram(object_program); // The main GPU program is ready to be handle streams of polygons
    check_linking(object_program);


    lighting_program = glCreateProgram(); // Create a GPU program, i.e., two central shaders of the graphics pipeline
    loadShader(lighting_program, GL_VERTEX_SHADER, "res/shaders/vShaderLighting.glsl");
    loadShader(lighting_program, GL_FRAGMENT_SHADER, "res/shaders/fShaderLighting.glsl");
    glLinkProgram(lighting_program); // The main GPU program is ready to be handle streams of polygons
    check_linking(lighting_program);

}



void initCamera() {
    int width, height;
    glfwGetWindowSize(g_window, &width, &height);
    g_camera.setAspectRatio(static_cast<float>(width) / static_cast<float>(height));
    g_camera.setNear(0.1);
    g_camera.setFar(80.1);
}




void init() {
    modelMatrices[outerSpace] = glm::mat4(1.0);
    modelMatrices[sun] = glm::mat4(1.0);
    modelMatrices[earth] = glm::mat4(1.0);
    modelMatrices[moon] = glm::mat4(1.0);

    initGLFW();
    initOpenGL();

    initGPUprogram();

    sphere_mesh = Mesh::genSphere(32);
    sphere_mesh->init();

    g_earthTexID = loadTextureFromFileToGPU("res/media/earth.jpg");
    g_moonTexID = loadTextureFromFileToGPU("res/media/moon.jpg");
    g_sunTexID = loadTextureFromFileToGPU("res/media/sun.jpg");

    initCamera();
}

void clear() {
    glDeleteProgram(object_program);
    glDeleteProgram(lighting_program);


    glfwDestroyWindow(g_window);
    glfwTerminate();
}


float calculate_phase(const float periode, const float time)
{
    return 2.0 * M_PI * time / periode;
}

void render() {


    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // Erase the color and z buffers

    float time = static_cast<float>(glfwGetTime());

    modelMatrices[earth] = glm::mat4(1.0f);

    earthOrbitalMovement = glm::vec3(kRadOrbitEarth * cos(calculate_phase(kPeriodeOrbitEarth, time)), kRadOrbitEarth * sin(calculate_phase(kPeriodeOrbitEarth, time)), 0.0);
    modelMatrices[earth] = glm::translate(modelMatrices[earth], earthOrbitalMovement);

    moonOrbitalMovement = glm::vec3(kRadOrbitMoon * cos(calculate_phase(kPeriodeMoon, time)), kRadOrbitMoon * sin(calculate_phase(kPeriodeMoon, time)), 0.0);
    modelMatrices[moon] = glm::translate(modelMatrices[earth], moonOrbitalMovement);

    modelMatrices[earth] = glm::rotate(modelMatrices[earth], calculate_phase(kPeriodeRotEarth, time), earthRotationAxe);
    modelMatrices[earth] = glm::scale(modelMatrices[earth], glm::vec3(kSizeEarth));


    modelMatrices[moon] = glm::rotate(modelMatrices[moon], calculate_phase(kPeriodeMoon, time), glm::vec3(0.0f, 0.0f, 1.0f));

    modelMatrices[moon] = glm::scale(modelMatrices[moon], glm::vec3(kSizeMoon));

    modelMatrices[sun] = glm::mat4(1.0f);
    modelMatrices[sun] = glm::scale(modelMatrices[sun], glm::vec3(kSizeSun)); // a smaller cube

    if (cameraSpaceObject == outerSpace)
    {
        modelMatrices[outerSpace] = glm::mat4(1.0f);
        freeCameraMovement = g_camera.calculate_camera_pos();
        modelMatrices[outerSpace] = glm::translate(modelMatrices[outerSpace], freeCameraMovement);
    }
    g_camera.setPosition(glm::vec3(modelMatrices[cameraSpaceObject] * glm::vec4(0.0, 0.0, 0.0, 1.0)));
    g_camera.setLookAtPoint(glm::vec3(modelMatrices[lookAtSpaceObject] * glm::vec4(0.0, 0.0, 0.0, 1.0)));


    const glm::mat4 viewMatrix = g_camera.computeViewMatrix();
    const glm::mat4 projMatrix = g_camera.computeProjectionMatrix();
    const glm::vec3 camPosition = g_camera.getPosition();


    glUseProgram(object_program);

    glUniformMatrix4fv(glGetUniformLocation(object_program, "viewMat"), 1, GL_FALSE, glm::value_ptr(viewMatrix)); // compute the view matrix of the camera and pass it to the GPU program
    glUniformMatrix4fv(glGetUniformLocation(object_program, "projMat"), 1, GL_FALSE, glm::value_ptr(projMatrix)); // compute the projection matrix of the camera and pass it to the GPU program
    glUniform3f(glGetUniformLocation(object_program, "camPos"), camPosition[0], camPosition[1], camPosition[2]);
    glUniform3fv(glGetUniformLocation(object_program, "lColor"), 1, &lightColor[0]);

    glActiveTexture(GL_TEXTURE0);
    glUniform1i(glGetUniformLocation(object_program, "text"), 0);
    glBindTexture(GL_TEXTURE_2D, g_earthTexID);

    glUniformMatrix4fv(glGetUniformLocation(object_program, "modelMat"), 1, GL_FALSE, glm::value_ptr(modelMatrices[earth])); // compute the model matrix
    sphere_mesh->render();

    glActiveTexture(GL_TEXTURE0);
    glUniform1i(glGetUniformLocation(object_program, "text"), 0);
    glBindTexture(GL_TEXTURE_2D, g_moonTexID);

    glUniformMatrix4fv(glGetUniformLocation(object_program, "modelMat"), 1, GL_FALSE, glm::value_ptr(modelMatrices[moon])); // compute the model matrix


    sphere_mesh->render();

    glUseProgram(lighting_program);

    glUniformMatrix4fv(glGetUniformLocation(lighting_program, "viewMat"), 1, GL_FALSE, glm::value_ptr(viewMatrix)); // compute the view matrix of the camera and pass it to the GPU program
    glUniformMatrix4fv(glGetUniformLocation(lighting_program, "projMat"), 1, GL_FALSE, glm::value_ptr(projMatrix)); // compute the projection matrix of the camera and pass it to the GPU program

    glActiveTexture(GL_TEXTURE0);
    glUniform1i(glGetUniformLocation(object_program, "text"), 0);
    glBindTexture(GL_TEXTURE_2D, g_sunTexID);


    glUniformMatrix4fv(glGetUniformLocation(lighting_program, "modelMat"), 1, GL_FALSE, glm::value_ptr(modelMatrices[sun])); // compute the model matrix

    sphere_mesh->render();
}

// Update any accessible variable based on the current time
void update(const float currentTimeInSec) {
    // std::cout << currentTimeInSec << std::endl;

}

int main(int argc, char** argv) {
    init(); // Your initialization code (user interface, OpenGL states, scene with geometry, material, lights, etc)
    while (!glfwWindowShouldClose(g_window)) {
        update(static_cast<float>(glfwGetTime()));
        render();
        glfwSwapBuffers(g_window);
        glfwPollEvents();
    }
    clear();
    return EXIT_SUCCESS;
}

#if defined(__APPLE__)
#include <OpenGL/gl3.h>
#include <GLFW/glfw3.h>
#else
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#endif
#include "shader.h"
#include "app_options.h"
#include "frame_profiler.h"

#include <iostream>

static void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
}

// Mouse state for iMouse
static double gMouseX = 0.0, gMouseY = 0.0;
static double gClickX = 0.0, gClickY = 0.0;
static int gMouseDown = 0;
static float gOrbitYaw = 0.0f;
static float gOrbitPitch = 0.0f;
static float gDragStartYaw = 0.0f;
static float gDragStartPitch = 0.0f;
static double gDragStartX = 0.0, gDragStartY = 0.0;

static void cursor_pos_callback(GLFWwindow* window, double xpos, double ypos) {
    gMouseX = xpos;
    gMouseY = ypos;
    if (gMouseDown) {
        const float kSensitivity = 0.01f;
        const float dy = static_cast<float>(ypos - gDragStartY);
        const float dx = static_cast<float>(xpos - gDragStartX);
        gOrbitYaw = gDragStartYaw - dx * kSensitivity;
        gOrbitPitch = gDragStartPitch + dy * kSensitivity;
        if (gOrbitPitch > 1.4f) gOrbitPitch = 1.4f;
        if (gOrbitPitch < -1.4f) gOrbitPitch = -1.4f;
    }
}

static void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
    if (button == GLFW_MOUSE_BUTTON_LEFT) {
        if (action == GLFW_PRESS) {
            gMouseDown = 1;
            gClickX = gMouseX;
            gClickY = gMouseY;
            gDragStartX = gMouseX;
            gDragStartY = gMouseY;
            gDragStartYaw = gOrbitYaw;
            gDragStartPitch = gOrbitPitch;
            std::cout << "MOUSE PRESS at: " << gMouseX << "," << gMouseY << std::endl;
        } else if (action == GLFW_RELEASE) {
            gMouseDown = 0;
            std::cout << "MOUSE RELEASE at: " << gMouseX << "," << gMouseY << std::endl;
        }
    }
}

int main(int argc, char** argv) {
    AppOptions options;
    ParseStatus parseStatus = parseAppOptions(argc, argv, options);
    if (parseStatus == ParseStatus::ExitSuccess) return 0;
    if (parseStatus == ParseStatus::ExitFailure) return 1;

    if (!glfwInit()) return -1;
    #if defined(__APPLE__)
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    #else
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    #endif

    GLFWwindow* window = glfwCreateWindow(800, 600, "Shader Runner", nullptr, nullptr);
    if (!window) { glfwTerminate(); return -1; }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, cursor_pos_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);

    #if !defined(__APPLE__)
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "Failed to initialize GLAD" << std::endl;
        return -1;
    }
    #endif

    FrameProfiler profiler(options.profilerConfig);
    profiler.initialize();

    const char* vertexSrc = R"GLSL(#version 330 core
layout(location = 0) in vec2 aPos;
out vec2 vUV;
void main(){ vUV = aPos * 0.5 + 0.5; gl_Position = vec4(aPos, 0.0, 1.0); }
)GLSL";

    GLuint program = createProgramFromFiles(vertexSrc, options.fragPath.c_str());
    if (!program) {
        std::cerr << "Failed to create shader program" << std::endl;
        return -1;
    }

    float quad[] = {
        -1.0f, -1.0f,
         1.0f, -1.0f,
        -1.0f,  1.0f,
        -1.0f,  1.0f,
         1.0f, -1.0f,
         1.0f,  1.0f
    };

    GLuint VAO, VBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quad), quad, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);

    while (!glfwWindowShouldClose(window)) {
        profiler.beginFrame();
        int w, h; glfwGetFramebufferSize(window, &w, &h);
        glViewport(0, 0, w, h);
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        glUseProgram(program);
        GLint resLoc = glGetUniformLocation(program, "iResolution");
        GLint timeLoc = glGetUniformLocation(program, "iTime");
        GLint mouseLoc = glGetUniformLocation(program, "iMouse");
        GLint orbitLoc = glGetUniformLocation(program, "iOrbit");
        if (resLoc >= 0) glUniform3f(resLoc, (float)w, (float)h, 1.0f);
        if (timeLoc >= 0) glUniform1f(timeLoc, (float)glfwGetTime());
        if (orbitLoc >= 0) glUniform2f(orbitLoc, gOrbitYaw, gOrbitPitch);
        if (mouseLoc >= 0) {
            // Convert GLFW cursor y (top-left origin) to GLSL fragCoord (bottom-left origin)
            float my = (float)h - (float)gMouseY;
            float cmy = (float)h - (float)gClickY;
            float cx = gMouseDown ? (float)gClickX : 0.0f;
            float cy = gMouseDown ? cmy : 0.0f;
            glUniform4f(mouseLoc, (float)gMouseX, my, cx, cy);
        }

        glBindVertexArray(VAO);
        profiler.beginGpuTiming();
        glDrawArrays(GL_TRIANGLES, 0, 6);
        profiler.endGpuTiming();
        profiler.markCpuSubmitEnd();

        glfwSwapBuffers(window);
        glfwPollEvents();
        profiler.endFrame();

        if (profiler.shouldStop()) glfwSetWindowShouldClose(window, GLFW_TRUE);
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) glfwSetWindowShouldClose(window, GLFW_TRUE);
    }

    profiler.finalize();
    glDeleteProgram(program);
    glDeleteBuffers(1, &VBO);
    glDeleteVertexArrays(1, &VAO);
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}

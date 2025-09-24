#pragma region includes
#include <iostream>
#include <windows.h>
#include <thread>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <glad/glad.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <imgui_stdlib.h>

#include "Header.h"
#include "Renderer.h"
#include "Mesh.h"
#include "ui.h"
#pragma endregion

void CubesMainGameLoop(GLFWwindow* window);

int CALLBACK WinMain(
    HINSTANCE Instance,
    HINSTANCE PrevInstance,
    LPSTR CommandLine,
    int ShowCode)
{
    GLFWwindow* window;
#pragma region GLFW
    if (!glfwInit())
        return -1;

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    //glfwWindowHint(GLFW_SAMPLES, 4); // 4 дл€ 4x MSAA

    window = glfwCreateWindow(1280, 720, "CUBES", NULL, NULL);
    if (!window)
    {
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSwapInterval(0); // disable vsync
#pragma endregion

    //gladLoadGL();
    //gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
    Renderer::init((Renderer::LoadProc)glfwGetProcAddress);

#pragma region Imgui
    const char* glsl_version = "#version 330";

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);
    ImGui::StyleColorsDark();
#pragma endregion

    // компил€ци€ шейдеров
    initShaders();
    UI::Init();

    // MAIN LOOP
    CubesMainGameLoop(window);

    ImGui_ImplGlfw_Shutdown();
    ImGui_ImplOpenGL3_Shutdown();
    ImGui::DestroyContext();

    glfwTerminate();

    return 0;
}
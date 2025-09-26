#ifdef _WIN32

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

#include "Renderer.h"
#include "Mesh.h"
#include "ui.h"
#include "Input.h"
#pragma endregion

void GameInit(GLFWwindow* window);
void GameUpdateAndRender(GLFWwindow* window, float time, Input* input);

double g_MouseScrollYOffset = 0;

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
    g_MouseScrollYOffset = yoffset;
}

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

    window = glfwCreateWindow(1280, 720, "CUBES", NULL, NULL);
    if (!window)
    {
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED); // disable cursor
    glfwSetScrollCallback(window, scroll_callback);
#pragma endregion

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

    Input input[2] = { 0 };
    Input* newInput = &input[0];
    Input* oldInput = &input[1];

    GameInit(window);
    // MAIN LOOP
    while (!glfwWindowShouldClose(window)) {
        // input processing
        {
            ProcessButtonInput(&oldInput->startGame, &newInput->startGame, IsKeyReleased(window, GLFW_KEY_ENTER));
            ProcessButtonInput(&oldInput->switchExitMenu, &newInput->switchExitMenu, IsKeyReleased(window, GLFW_KEY_ESCAPE));

            ProcessButtonInput(&oldInput->scrollUp, &newInput->scrollUp, g_MouseScrollYOffset > 0);
            ProcessButtonInput(&oldInput->scrollDown, &newInput->scrollDown, g_MouseScrollYOffset < 0);
            g_MouseScrollYOffset = 0;

            ProcessButtonInput(&oldInput->forward, &newInput->forward, IsKeyReleased(window, GLFW_KEY_W));
            ProcessButtonInput(&oldInput->backwards, &newInput->backwards, IsKeyReleased(window, GLFW_KEY_S));
            ProcessButtonInput(&oldInput->left, &newInput->left, IsKeyReleased(window, GLFW_KEY_A));
            ProcessButtonInput(&oldInput->right, &newInput->right, IsKeyReleased(window, GLFW_KEY_D));

            ProcessButtonInput(&oldInput->attack, &newInput->attack, IsMouseButtonReleased(window, GLFW_MOUSE_BUTTON_LEFT));
            ProcessButtonInput(&oldInput->placeBlock, &newInput->placeBlock, IsMouseButtonReleased(window, GLFW_MOUSE_BUTTON_RIGHT));

            for (size_t i = 0; i < 9; i++) // TODO: убрать захардкоженое число
            {
                ProcessButtonInput(&oldInput->inventorySlots[i], &newInput->inventorySlots[i], IsKeyReleased(window, GLFW_KEY_1 + i));
            }

#if _DEBUG
            ProcessButtonInput(&oldInput->rebuildShaders, &newInput->rebuildShaders, IsKeyReleased(window, GLFW_KEY_R));
#endif
        }

        GameUpdateAndRender(window, glfwGetTime(), newInput);

        // swap old and new inputs
        Input* tempInput = oldInput;
        oldInput = newInput;
        newInput = tempInput;
    }

    ImGui_ImplGlfw_Shutdown();
    ImGui_ImplOpenGL3_Shutdown();
    ImGui::DestroyContext();

    glfwTerminate();

    return 0;
}

#endif
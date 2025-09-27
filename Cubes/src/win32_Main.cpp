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

#include "Cubes.h"
#include "Renderer.h"
#include "Mesh.h"
#include "ui.h"
#include "Input.h"
#include "Tools.h"
#pragma endregion

static GLFWwindow* window;
double g_MouseScrollYOffset = 0;

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
    g_MouseScrollYOffset = yoffset;
}

void GetFramebufferSize(int* width, int* height) {
    int w, h;
    glfwGetFramebufferSize(window, &w, &h);
    *width = std::max(1, w);
    *height = std::max(1, h);
}

void SetVsync(bool vsyncOn) {
    glfwSwapInterval(vsyncOn);
}

void GetCursorPos(double* xpos, double* ypos) {
    glfwGetCursorPos(window, xpos, ypos);
}

void SetCursorMode(bool enabled) {
    glfwSetInputMode(window, GLFW_CURSOR, enabled ? GLFW_CURSOR_NORMAL : GLFW_CURSOR_DISABLED);
}

void CloseWindow() {
    glfwSetWindowShouldClose(window, true);
}

int CALLBACK WinMain(
    HINSTANCE Instance,
    HINSTANCE PrevInstance,
    LPSTR CommandLine,
    int ShowCode)
{
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

    GameInit();
    // MAIN LOOP
    while (!glfwWindowShouldClose(window)) {
#define IsMouseButtonReleased(window, button) (glfwGetMouseButton(window, button) == GLFW_RELEASE)
#define IsKeyReleased(window, key) (glfwGetKey(window, key) == GLFW_RELEASE)
        // input processing
        {
            ProcessButtonInput(&oldInput->uiClick, &newInput->uiClick, IsMouseButtonReleased(window, GLFW_MOUSE_BUTTON_LEFT));

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

            ProcessButtonInput(&oldInput->openInventory, &newInput->openInventory, IsKeyReleased(window, GLFW_KEY_TAB));
            for (size_t i = 0; i < ArraySize(oldInput->inventorySlots); i++)
            {
                ProcessButtonInput(&oldInput->inventorySlots[i], &newInput->inventorySlots[i], IsKeyReleased(window, GLFW_KEY_1 + i));
            }

#if _DEBUG
            ProcessButtonInput(&oldInput->rebuildShaders, &newInput->rebuildShaders, IsKeyReleased(window, GLFW_KEY_R));
#endif
        }

        FrameBufferInfo fbInfo;
        GetFramebufferSize(&fbInfo.sizeX, &fbInfo.sizeY);

        GameUpdateAndRender(glfwGetTime(), newInput, &fbInfo);

        // swap old and new inputs
        Input* tempInput = oldInput;
        oldInput = newInput;
        newInput = tempInput;

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    ImGui_ImplGlfw_Shutdown();
    ImGui_ImplOpenGL3_Shutdown();
    ImGui::DestroyContext();

    glfwTerminate();

    return 0;
}

#endif
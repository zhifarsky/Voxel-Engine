#include "Cubes.h"

#ifdef PLATFORM_GLFW
#pragma region includes
#include <iostream>
#include <thread>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <glad/glad.h>
//#include <imgui.h>
//#include <imgui_impl_glfw.h>
//#include <imgui_impl_opengl3.h>
//#include <imgui_stdlib.h>

#include "stb_image.h"
#include "Renderer.h"
#include "Mesh.h"
#include "ui.h"
#include "Input.h"
#include "Tools.h"
#include "Files.h"
#pragma endregion

#define DEFAULT_WIDTH 1280
#define DEFAULT_HEIGHT 720

static GLFWwindow* window;
static WindowMode g_windowMode = WindowMode::Windowed;
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

WindowMode WindowGetCurrentMode() {
    return g_windowMode;
}

void WindowSwitchMode(WindowMode windowMode)
{
    GLFWmonitor* monitor = glfwGetPrimaryMonitor();
    const GLFWvidmode* mode = glfwGetVideoMode(monitor);

    static int lastWidth = DEFAULT_WIDTH, lastHeight = DEFAULT_HEIGHT;
    static int lastPosX = 100, lastPosY = 100;

    switch (windowMode)
    {
    case WindowMode::Windowed:
        g_windowMode = WindowMode::Windowed;
        glfwSetWindowAttrib(window, GLFW_DECORATED, GLFW_TRUE);
        glfwSetWindowMonitor(window, NULL, 0, 0, lastWidth, lastHeight, mode->refreshRate);
        glfwSetWindowPos(window, lastPosX, lastPosY);
        break;
    case WindowMode::WindowedFullScreen:
        g_windowMode = WindowMode::WindowedFullScreen;
        glfwGetWindowSize(window, &lastWidth, &lastHeight);
        glfwGetWindowPos(window, &lastPosX, &lastPosY);
        //glfwSetWindowMonitor(window, monitor, 0, 0, mode->width, mode->height, mode->refreshRate);
        glfwSetWindowAttrib(window, GLFW_DECORATED, GLFW_FALSE);
        glfwSetWindowMonitor(window, NULL, 0, 0, mode->width, mode->height, mode->refreshRate);
        break;
    default:
        break;
    }
}

void CloseWindow() {
    glfwSetWindowShouldClose(window, true);
}

int main()
{
#pragma region GLFW
    if (!glfwInit())
        return -1;

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    window = glfwCreateWindow(DEFAULT_WIDTH, DEFAULT_HEIGHT, "CUBES", NULL, NULL);
    //auto monitor = glfwGetPrimaryMonitor();
    //auto mode = glfwGetVideoMode(monitor);
    //window = glfwCreateWindow(mode->width, mode->height, "CUBES", monitor, NULL);
    //glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL); // disable cursor
    if (!window)
    {
        glfwTerminate();
        return -1;
    }

    // set window icon
    {
        GLFWimage image;
        image.pixels = stbi_load(TEX_FOLDER"icon.png", &image.width, &image.height, NULL, 4);
        glfwSetWindowIcon(window, 1, &image);
        stbi_image_free(image.pixels);
    }

    //WindowSwitchMode(WindowMode::Windowed); // оконный при запуске

    glfwMakeContextCurrent(window);
    //glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED); // disable cursor
    glfwSetScrollCallback(window, scroll_callback);
#pragma endregion

    Renderer::Init((Renderer::LoadProc)glfwGetProcAddress);

#pragma region Imgui
    //const char* glsl_version = "#version 330";

    //IMGUI_CHECKVERSION();
    //ImGui::CreateContext();
    //ImGuiIO& io = ImGui::GetIO();
    //ImGui_ImplGlfw_InitForOpenGL(window, true);
    //ImGui_ImplOpenGL3_Init(glsl_version);
    //ImGui::StyleColorsDark();
#pragma endregion
    
    //
    // game inputs
    //

    Input input[2] = { 0 };
    Input* newInput = &input[0];
    Input* oldInput = &input[1];

    //
    // game memory
    //

    GameMemory memory = {0};
    memory.permStorage.alloc(Megabytes(128), Gigabytes(1));
    memory.tempStorage.alloc(Megabytes(128), Gigabytes(1));
    memory.chunkStorage.alloc(Gigabytes(2), Gigabytes(4));

    // MAIN LOOP
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        //
        // input processing
        //

#define IsMouseButtonReleased(window, button) (glfwGetMouseButton(window, button) == GLFW_RELEASE)
#define IsKeyReleased(window, key) (glfwGetKey(window, key) == GLFW_RELEASE)

        {
            ProcessButtonInput(&oldInput->uiClick, &newInput->uiClick, IsMouseButtonReleased(window, GLFW_MOUSE_BUTTON_LEFT));

            ProcessButtonInput(&oldInput->startGame, &newInput->startGame, IsKeyReleased(window, GLFW_KEY_ENTER));
            ProcessButtonInput(&oldInput->switchExitMenu, &newInput->switchExitMenu, IsKeyReleased(window, GLFW_KEY_ESCAPE));
            ProcessButtonInput(&oldInput->switchFullscreenMode, &newInput->switchFullscreenMode, IsKeyReleased(window, GLFW_KEY_F11));

            ProcessButtonInput(&oldInput->scrollUp, &newInput->scrollUp, g_MouseScrollYOffset > 0);
            ProcessButtonInput(&oldInput->scrollDown, &newInput->scrollDown, g_MouseScrollYOffset < 0);
            g_MouseScrollYOffset = 0;

            ProcessButtonInput(&oldInput->forward, &newInput->forward, IsKeyReleased(window, GLFW_KEY_W));
            ProcessButtonInput(&oldInput->backwards, &newInput->backwards, IsKeyReleased(window, GLFW_KEY_S));
            ProcessButtonInput(&oldInput->left, &newInput->left, IsKeyReleased(window, GLFW_KEY_A));
            ProcessButtonInput(&oldInput->right, &newInput->right, IsKeyReleased(window, GLFW_KEY_D));
            ProcessButtonInput(&oldInput->up, &newInput->up, IsKeyReleased(window, GLFW_KEY_SPACE));
            ProcessButtonInput(&oldInput->down, &newInput->down, IsKeyReleased(window, GLFW_KEY_LEFT_CONTROL));

            ProcessButtonInput(&oldInput->attack, &newInput->attack, IsMouseButtonReleased(window, GLFW_MOUSE_BUTTON_LEFT));
            ProcessButtonInput(&oldInput->placeBlock, &newInput->placeBlock, IsMouseButtonReleased(window, GLFW_MOUSE_BUTTON_RIGHT));

            ProcessButtonInput(&oldInput->openInventory, &newInput->openInventory, IsKeyReleased(window, GLFW_KEY_TAB));
            for (size_t i = 0; i < ArrayCount(oldInput->inventorySlots); i++)
            {
                ProcessButtonInput(&oldInput->inventorySlots[i], &newInput->inventorySlots[i], IsKeyReleased(window, GLFW_KEY_1 + i));
            }

            ProcessButtonInput(&oldInput->showDebugInfo, &newInput->showDebugInfo, IsKeyReleased(window, GLFW_KEY_F1));

#if _DEBUG
            ProcessButtonInput(&oldInput->rebuildShaders, &newInput->rebuildShaders, IsKeyReleased(window, GLFW_KEY_R));
            ProcessButtonInput(&oldInput->regenerateChunks, &newInput->regenerateChunks, IsKeyReleased(window, GLFW_KEY_G));
#endif
        }

        FrameBufferInfo fbInfo;
        GetFramebufferSize(&fbInfo.sizeX, &fbInfo.sizeY);

        GameUpdateAndRender(&memory, glfwGetTime(), newInput, &fbInfo);

        // swap old and new inputs
        Input* tempInput = oldInput;
        oldInput = newInput;
        newInput = tempInput;

        // clear temporal memory
        memory.tempStorage.clear();

        glfwSwapBuffers(window);
    }

    //ImGui_ImplGlfw_Shutdown();
    //ImGui_ImplOpenGL3_Shutdown();
    //ImGui::DestroyContext();

    glfwTerminate();

    return 0;
}
#endif
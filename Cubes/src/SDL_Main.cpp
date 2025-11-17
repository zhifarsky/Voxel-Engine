#include "Cubes.h"

#ifdef PLATFORM_SDL
#include <SDL3/SDL.h>
#include <SDL3/SDL_init.h>
#include <SDL3/SDL_video.h>
#include <SDL3/SDL_timer.h>
#include <SDL3/SDL_mouse.h>
#include <stb_image.h>
#include "Renderer.h"
#include "Tools.h"
#include "Files.h"

#define DEFAULT_WIDTH 1280
#define DEFAULT_HEIGHT 720
SDL_Window* window;
SDL_GLContext context;
bool gameRunning;
double g_MouseScrollYOffset;

bool GetVsync() {
    int interval;
    SDL_GL_GetSwapInterval(&interval);
    return interval == 1 ? true : false;
}

void SetVsync(bool vsyncOn) {
    SDL_GL_SetSwapInterval(vsyncOn);
}

void GetCursorPos(double* xpos, double* ypos) {
    if (SDL_GetWindowRelativeMouseMode(window)) {
        
        static float _xpos = 0, _ypos = 0;
        if (SDL_GetWindowFlags(window) & SDL_WINDOW_INPUT_FOCUS) {
            float xdelta, ydelta;

            SDL_GetRelativeMouseState(&xdelta, &ydelta);
            _xpos += xdelta;
            _ypos += ydelta;
        }

        *xpos = _xpos;
        *ypos = _ypos;
    }
    else {
        float _xpos, _ypos;
        SDL_GetMouseState(&_xpos, &_ypos);
        *xpos = _xpos;
        *ypos = _ypos;
    }

}

void SetCursorMode(bool enabled) {
    if (enabled)
        SDL_SetWindowRelativeMouseMode(window, false);
    else
        SDL_SetWindowRelativeMouseMode(window, true);
}

void WindowSwitchMode(WindowMode windowMode) {
    static int lastWidth = DEFAULT_WIDTH, lastHeight = DEFAULT_HEIGHT;
    static int lastPosX = 100, lastPosY = 100;
    
    switch (windowMode)
    {
    case WindowMode::Windowed:
        SDL_SetWindowFullscreen(window, false);
        SDL_SetWindowSize(window, lastWidth, lastHeight);
        SDL_SetWindowPosition(window, lastPosX, lastPosY);
        break;
    case WindowMode::WindowedFullScreen:
        SDL_GetWindowSize(window, &lastWidth, &lastHeight);
        SDL_GetWindowPosition(window, &lastPosX, &lastPosY);
        SDL_SetWindowFullscreen(window, true);
        SDL_SetWindowFullscreenMode(window, NULL);
        break;
    }
}

WindowMode WindowGetCurrentMode() {
    SDL_WindowFlags flags = SDL_GetWindowFlags(window);
    if (flags & SDL_WINDOW_FULLSCREEN) {
        return WindowMode::WindowedFullScreen;
    }
    else {
        return WindowMode::Windowed;
    }
}

void CloseWindow() {
    gameRunning = false;
}
int main () {
    if (!SDL_Init(SDL_INIT_VIDEO))
        return -1;

    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_ACCELERATED_VISUAL, 1);
    SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8);

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

    bool loadRes = SDL_GL_LoadLibrary(NULL);

    window = SDL_CreateWindow(
        "CUBES", DEFAULT_WIDTH, DEFAULT_HEIGHT,
        SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
    if (!window) {
        SDL_Quit();
        return -1;
    }

    // set window icon
    {
        int w, h;
        u8* pixels = stbi_load(TEX_FOLDER"icon.png", &w, &h, NULL, 4);
        SDL_Surface* surface = SDL_CreateSurface(w, h, SDL_PixelFormat::SDL_PIXELFORMAT_ABGR8888);
        surface->pixels = pixels;
        SDL_SetWindowIcon(window, surface);
        stbi_image_free(pixels);
    }

    context = SDL_GL_CreateContext(window);
    if (!context) {
        SDL_DestroyWindow(window);
        SDL_Quit();
        return -1;
    }

    Renderer::Init((Renderer::LoadProc)SDL_GL_GetProcAddress);;

    GameMemory memory = { 0 };
    memory.permStorage.alloc(Megabytes(128), Gigabytes(1));
    memory.tempStorage.alloc(Megabytes(128), Gigabytes(1));
    memory.chunkStorage.alloc(Megabytes(128), Gigabytes(1));
    
    Input inputs[2] = { 0 };
    Input* oldInput = &inputs[0];
    Input* newInput = &inputs[1];

    gameRunning = true;
    while (gameRunning) {

        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
            case SDL_EVENT_QUIT:
                gameRunning = false;
                break;

            case SDL_EVENT_MOUSE_WHEEL:
                g_MouseScrollYOffset = event.wheel.y;
                break;
            }
        }

#define IsMouseButtonReleased(window, button) ((mouseButtons & SDL_BUTTON_MASK(button)) == false)
#define IsKeyReleased(window, key) (keys[key] == false)
        
        const bool* keys =  SDL_GetKeyboardState(NULL);
        SDL_MouseButtonFlags mouseButtons = SDL_GetMouseState(NULL, NULL);
        
        // handle inputs
        {
            ProcessButtonInput(&oldInput->uiClick, &newInput->uiClick, IsMouseButtonReleased(window, SDL_BUTTON_LEFT));
            
            ProcessButtonInput(&oldInput->startGame, &newInput->startGame, IsKeyReleased(window, SDL_SCANCODE_KP_ENTER));
            ProcessButtonInput(&oldInput->switchExitMenu, &newInput->switchExitMenu, IsKeyReleased(window, SDL_SCANCODE_ESCAPE));
            ProcessButtonInput(&oldInput->switchFullscreenMode, &newInput->switchFullscreenMode, IsKeyReleased(window, SDL_SCANCODE_F11));

            ProcessButtonInput(&oldInput->scrollUp, &newInput->scrollUp, g_MouseScrollYOffset > 0);
            ProcessButtonInput(&oldInput->scrollDown, &newInput->scrollDown, g_MouseScrollYOffset < 0);
            g_MouseScrollYOffset = 0;

            ProcessButtonInput(&oldInput->forward, &newInput->forward, IsKeyReleased(window, SDL_SCANCODE_W));
            ProcessButtonInput(&oldInput->backwards, &newInput->backwards, IsKeyReleased(window, SDL_SCANCODE_S));
            ProcessButtonInput(&oldInput->left, &newInput->left, IsKeyReleased(window, SDL_SCANCODE_A));
            ProcessButtonInput(&oldInput->right, &newInput->right, IsKeyReleased(window, SDL_SCANCODE_D));
            ProcessButtonInput(&oldInput->up, &newInput->up, IsKeyReleased(window, SDL_SCANCODE_SPACE));
            ProcessButtonInput(&oldInput->down, &newInput->down, IsKeyReleased(window, SDL_SCANCODE_LCTRL));

            ProcessButtonInput(&oldInput->attack, &newInput->attack, IsMouseButtonReleased(window, SDL_BUTTON_LEFT));
            ProcessButtonInput(&oldInput->placeBlock, &newInput->placeBlock, IsMouseButtonReleased(window, SDL_BUTTON_RIGHT));

            ProcessButtonInput(&oldInput->openInventory, &newInput->openInventory, IsKeyReleased(window, SDL_SCANCODE_TAB));
            for (size_t i = 0; i < ArrayCount(oldInput->inventorySlots); i++)
            {
                ProcessButtonInput(&oldInput->inventorySlots[i], &newInput->inventorySlots[i], IsKeyReleased(window, SDL_SCANCODE_1 + i));
            }

            ProcessButtonInput(&oldInput->showDebugInfo, &newInput->showDebugInfo, IsKeyReleased(window, SDL_SCANCODE_F1));

#if _DEBUG
            ProcessButtonInput(&oldInput->rebuildShaders, &newInput->rebuildShaders, IsKeyReleased(window, SDL_SCANCODE_R));
            ProcessButtonInput(&oldInput->regenerateChunks, &newInput->regenerateChunks, IsKeyReleased(window, SDL_SCANCODE_G));
#endif
        }

        Uint64 time = SDL_GetTicks(); 
        FrameBufferInfo fbInfo;
        SDL_GetWindowSize(window, &fbInfo.sizeX, &fbInfo.sizeY);

        GameUpdateAndRender(&memory, time / 1000.0f, newInput, &fbInfo);

        // swap old and new inputs
        Input* tempInput = oldInput;
        oldInput = newInput;
        newInput = tempInput;

        // clear temporal memory
        memory.tempStorage.clear();

        SDL_GL_SwapWindow(window);
    }

    SDL_DestroyWindow(window);
    SDL_Quit();

	return 0;
}
#endif
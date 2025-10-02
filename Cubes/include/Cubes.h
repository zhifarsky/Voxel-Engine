#pragma once
#include "Input.h"

// platform layer
struct FrameBufferInfo {
	int sizeX, sizeY;
};

enum class WindowMode {
	Windowed,
	WindowedFullScreen
};

void SetVsync(bool vsyncOn);
void GetCursorPos(double* xpos, double* ypos);
void SetCursorMode(bool enabled);
void WindowSwitchMode(WindowMode windowMode);
WindowMode WindowGetCurrentMode();
void CloseWindow();

// game
void GameInit();
void GameUpdateAndRender(float time, Input* input, FrameBufferInfo* frameBufferInfo);

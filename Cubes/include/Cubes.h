#pragma once
#include "Input.h"

// platform layer
struct FrameBufferInfo {
	int sizeX, sizeY;
};
//void GetFramebufferSize(int* width, int* height);
void SetVsync(bool vsyncOn);
void GetCursorPos(double* xpos, double* ypos);
void SetCursorMode(bool enabled);
void CloseWindow();

// game
void GameInit();
void GameUpdateAndRender(float time, Input* input, FrameBufferInfo* frameBufferInfo);

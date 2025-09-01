#pragma once
#include <chrono>
#include "Typedefs.h"

#ifndef ArraySize(a)
#define ArraySize(a) (sizeof(a) / sizeof(*a))
#endif // !ArraySize(a)


void FatalError(const char* msg, int exitCode = 1);

void dbgprint(const wchar_t* str, ...);

void dbgprint(const char* str, ...);

// печатает последнюю ошибку, св€занную с системой (полученную при помощи GetLastError())
void syserrprint(const char* msg);

struct Timer {
	std::chrono::steady_clock::time_point startTime, stopTime;
	void start();
	void stop();
	void printMS(const char* msg = NULL);
	void printS(const char* msg = NULL);
};

bool pointRectCollision(
    float px, float py,
    float bottomLeftX, float bottomLeftY,
    float topRightX, float topRightY);

bool rectRectCollision(
    float aBottomLeftX, float aBottomLeftY,
    float aTopRightX, float aTopRightY,
    float bBottomLeftX, float bBottomLeftY,
    float bTopRightX, float bTopRightY
);

enum class FileType {
    binary,
    text
};

// читает весь файл, выдел€€ под него пам€ть
u8* readEntireFile(const char* path, u32* outBufferSize, FileType fileType);
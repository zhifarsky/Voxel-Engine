#pragma once
#include <chrono>
#include "Typedefs.h"

#define ArraySize(a) (sizeof(a) / sizeof(*a))

void FatalError(const char* msg, int exitCode = 1);

void dbgprint(const wchar_t* str, ...);

void dbgprint(const char* str, ...);

// печатает последнюю ошибку, связанную с системой (полученную при помощи GetLastError())
void syserrprint(const char* msg);

struct Timer {
	std::chrono::steady_clock::time_point startTime, stopTime;
	void start();
	void stop();
	void printMilliseconds(const char* msg = NULL);
    void printMicroseconds(const char* msg = NULL);
	void printS(const char* msg = NULL);
};

//#define DIR_WATCHER_BUF_SIZE 1024
//
//struct DirectoryWatcher {
//    HANDLE dirHandle;
//    OVERLAPPED overlapped;
//    BYTE buffer[DIR_WATCHER_BUF_SIZE];
//    DWORD bytesReturned;
//    void create(const wchar_t* directoryPath);
//    void startWatching();
//    void release();
//};

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

int GetThreadsCount();

bool IsFileExists(const char* filepath);
bool CreateNewDirectory(const char* path);

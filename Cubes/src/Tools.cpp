#include <windows.h>
#include <iostream>
#include "Tools.h"

void FatalError(const char* msg, int exitCode) {
	OutputDebugStringA("\n\n[!] FATAL ERROR: ");
	OutputDebugStringA(msg);
	OutputDebugStringA("\n\n");
	DebugBreak();
	exit(exitCode);
}

void dbgprint(const wchar_t* str, ...) {
	va_list argp;
	va_start(argp, str);
	wchar_t dbg_out[512];
	vswprintf_s(dbg_out, str, argp);
	va_end(argp);
	OutputDebugString(dbg_out);
}

void dbgprint(const char* str, ...) {
	va_list argp;
	va_start(argp, str);
	char dbg_out[512];
	vsprintf_s(dbg_out, str, argp);
	va_end(argp);
	OutputDebugStringA(dbg_out);
}

// печатает последнюю ошибку, связанную с системой (полученную при помощи GetLastError())
void syserrprint(const char* msg) {
	LPSTR dbg_out;
	auto err = GetLastError();
	WORD maxLen = 0xFF;
	FormatMessageA(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL, err,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPSTR)&dbg_out, 0, NULL);
	OutputDebugStringA(msg);
	OutputDebugStringA(dbg_out);
	LocalFree(dbg_out);
}

void Timer::start() {
	startTime = std::chrono::high_resolution_clock::now();
}

void Timer::stop() {
	stopTime = std::chrono::high_resolution_clock::now();
}

void Timer::printMS(const char* msg) {
	auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(stopTime - startTime);
	if (!msg) msg = "Timer:";
	dbgprint("%s %dms\n", msg, duration);
}

void Timer::printS(const char* msg) {
	auto duration = std::chrono::duration_cast<std::chrono::seconds>(stopTime - startTime);
	if (!msg) msg = "Timer:";
	dbgprint("%s %dms\n", msg, duration);
}

bool pointRectCollision(
	float px, float py,
	float bottomLeftX, float bottomLeftY,
	float topRightX, float topRightY)
{
	return
		px > bottomLeftX && px < topRightX &&
		py > bottomLeftY && py < topRightY;
}

bool rectRectCollision(
	float aBottomLeftX, float aBottomLeftY,
	float aTopRightX, float aTopRightY,
	float bBottomLeftX, float bBottomLeftY,
	float bTopRightX, float bTopRightY
) {
	// Проверяем, что прямоугольники не пересекаются по оси X
	if (aTopRightX <= bBottomLeftX ||
		bTopRightX <= aBottomLeftX) {
		return false;
	}

	// Проверяем, что прямоугольники не пересекаются по оси Y
	if (aTopRightY <= bBottomLeftY ||
		bTopRightY <= aBottomLeftY) {
		return false;
	}

	// Если обе проверки пройдены - есть коллизия
	return true;
}

u8* readEntireFile(const char* path, u32* outBufferSize, FileType fileType) {
	const char* mode = 0;
	switch (fileType)
	{
	case FileType::binary:
		mode = "rb"; break;
	case FileType::text:
		mode = "rt"; break;
	default:
		FatalError("Not implemented"); break;
	}

	FILE* file = fopen(path, mode);

	if (!file) {
		FatalError("Error on reading file\n");
		*outBufferSize = 0;
		return 0;
	}

	fseek(file, 0, SEEK_END);
	u32 fileSize = ftell(file);
	fseek(file, 0, SEEK_SET);

	u8* fileBuffer = (u8*)malloc(fileSize + 1);
	u32 bytesRead = fread(fileBuffer, 1, fileSize, file);
	fclose(file);

	if (fileType == FileType::text)
		fileBuffer[bytesRead] = '\0';

	*outBufferSize = fileSize;
	return fileBuffer;
}
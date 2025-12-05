#ifdef _WIN32

#include <windows.h>
#include <shlwapi.h>
#include <iostream>
#include "Tools.h"

#ifdef _DEBUG
#ifdef _MSC_VER
#define DEBUG_BREAK if (IsDebuggerPresent()) { __debugbreak(); } 
#endif
#else
#define DEBUG_BREAK ;
#endif

void FatalError(const char* msg, int exitCode) {
	dbgprint("\n\n[FATAL ERROR]: %s\n\n", msg);
	DEBUG_BREAK
		exit(exitCode);
}

void WarningMessage(const char* msg) {
	MessageBoxA(0, msg, "Error", MB_OK);
}

void dbgprint(const wchar_t* str, ...) {
#if _DEBUG
	va_list argp;
	va_start(argp, str);
	wchar_t dbg_out[512];
	vswprintf_s(dbg_out, str, argp);
	va_end(argp);
	OutputDebugString(dbg_out);
#endif
}

void dbgprint(const char* str, ...) {
#if _DEBUG
	va_list argp;
	va_start(argp, str);
	char dbg_out[512];
	vsprintf_s(dbg_out, str, argp);
	va_end(argp);
	OutputDebugStringA(dbg_out);
#endif
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

u64 GetMinCommitSize() {
	return 64 * 1024ULL;
}

void* MemReserve(u64 size) {
	return VirtualAlloc(0, size, MEM_RESERVE, PAGE_READWRITE);
}

void* MemCommit(void* base, u64 size) {
	return VirtualAlloc(base, size, MEM_COMMIT, PAGE_READWRITE);
}

bool MemFree(void* base) {
	return VirtualFree(base, 0, MEM_RELEASE);
}

int GetThreadsCount() {
	SYSTEM_INFO sysInfo;
	GetSystemInfo(&sysInfo);
	return max(1, sysInfo.dwNumberOfProcessors);
}

bool IsFileExists(const char* filepath) {
	BOOL res = PathFileExistsA(filepath);
	return res;
}

bool CreateNewDirectory(const char* path) {
	return CreateDirectoryA(path, NULL);
}

#endif
#include <iostream>
#include "Tools.h"

void Timer::start() {
	startTime = std::chrono::high_resolution_clock::now();
}

void Timer::stop() {
	stopTime = std::chrono::high_resolution_clock::now();
}

void Timer::printMilliseconds(const char* msg) {
	auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(stopTime - startTime);
	if (!msg) msg = "Timer:";
	dbgprint("%s %dmilliseconds\n", msg, duration);
}

void Timer::printMicroseconds(const char* msg) {
	auto duration = std::chrono::duration_cast<std::chrono::microseconds>(stopTime - startTime);
	if (!msg) msg = "Timer:";
	dbgprint("%s %dmicroseconds\n", msg, duration);
}

void Timer::printS(const char* msg) {
	auto duration = std::chrono::duration_cast<std::chrono::seconds>(stopTime - startTime);
	if (!msg) msg = "Timer:";
	dbgprint("%s %dms\n", msg, duration);
}

//void DirectoryWatcher::create(const wchar_t* directoryPath) {
//	dirHandle = CreateFileW(
//		directoryPath, 
//		FILE_LIST_DIRECTORY,
//		FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
//		NULL,
//		OPEN_EXISTING,
//		FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED,
//		NULL);
//	if (dirHandle == INVALID_HANDLE_VALUE)
//		FatalError("Directory not found");
//
//	overlapped = { 0 };
//	overlapped.hEvent = this;
//}
//
//void DirectoryWatcher::release() {
//	CloseHandle(dirHandle);
//}
//
//void CALLBACK FileIOCompletionRoutine(
//	DWORD dwErrorCode, DWORD dwNumberOfBytesTransfered, LPOVERLAPPED lpOverlapped)
//{
//	dbgprint("\n\nWATCHER CALLED\n\n");
//	if (dwErrorCode != ERROR_SUCCESS) {
//		syserrprint("Error in file watching");
//		return;
//	}
//
//
//	DirectoryWatcher* watcher = CONTAINING_RECORD(lpOverlapped, DirectoryWatcher, overlapped);
//	BYTE* buffer = watcher->buffer;
//	DWORD offset = 0;
//
//	while (offset < dwNumberOfBytesTransfered) {
//		FILE_NOTIFY_INFORMATION* info = (FILE_NOTIFY_INFORMATION*)&buffer[offset];
//
//		dbgprint("[DIR WATCHER]%d\n", info->Action);
//
//		offset += info->NextEntryOffset;
//	}
//
//	watcher->startWatching();
//}
//
//void DirectoryWatcher::startWatching() {
//	BOOL success = ReadDirectoryChangesW(
//		dirHandle,
//		buffer,
//		DIR_WATCHER_BUF_SIZE,
//		TRUE,
//		FILE_NOTIFY_CHANGE_LAST_WRITE |
//		FILE_NOTIFY_CHANGE_FILE_NAME |
//		FILE_NOTIFY_CHANGE_DIR_NAME,
//		&bytesReturned,
//		&overlapped,
//		FileIOCompletionRoutine
//	);
//
//	if (!success)
//		syserrprint("Failed to start watching");
//}

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

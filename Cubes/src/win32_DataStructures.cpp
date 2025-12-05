#ifdef _WIN32

#include <intrin.h>
#include <windows.h>
#include "DataStructures.h"

constexpr u64 COMMIT_SIZE = 64 * 1024;
#define roundToMultiple(number, multiple) ((number + multiple - 1) / multiple) * multiple

void Arena::alloc(u64 capacity, u64 reserveCapacity) {
	this->capacity = roundToMultiple(capacity, COMMIT_SIZE); // rounding to page size
	this->size = 0;
	this->reserved = reserveCapacity;

	mem = (u8*)VirtualAlloc(0, reserveCapacity, MEM_RESERVE, PAGE_READWRITE); // reserve large chunk of memory
	VirtualAlloc(mem, this->capacity, MEM_COMMIT, PAGE_READWRITE); // commit memory
}

void Arena::release() {
	VirtualFree(mem, 0, MEM_RELEASE);
	mem = 0;
	size = capacity = 0;
}

void* _ArenaPush(Arena* arena, u64 size, u64 alignment, bool clearToZero) {
	assert(alignment && !(alignment & (alignment - 1))); // проверка на степень двойки
	
	u8* result = arena->mem + arena->size;
	s64 padding = -(s64)result & (alignment - 1); // работает только со степенями двойки

	result += padding;

	// commit memory
	if (result + size >= arena->mem + arena->capacity) {
		s64 commitSize = roundToMultiple(size + padding, COMMIT_SIZE);
		assert(arena->reserved >= arena->capacity + commitSize);

		VirtualAlloc(arena->mem + arena->capacity, commitSize, MEM_COMMIT, PAGE_READWRITE);
		arena->capacity += commitSize;
	}

	arena->size += size + padding;

	if (clearToZero)
		ZeroMemory(result, size);

	return result;
}

void Arena::clear() {
	size = 0;
}

//// NOTE: возможно придется применить CRITICAL_SECTION в функциях для потокобезопасности
//struct WorkQueue {
//	int volatile taskCount;
//	int volatile nextTask;
//	int volatile taskCompletionCount;
//	HANDLE semaphore;
//};
//
//WorkQueue* WorkQueueCreate(u32 maxSemaphore)
//{
//	WorkQueue* queue = (WorkQueue*)calloc(1, sizeof(WorkQueue));
//	queue->semaphore = CreateSemaphore(0, 0, maxSemaphore, 0);
//	return queue;
//}
//
//void WorkQueueRelease(WorkQueue* queue) {
//	CloseHandle(queue->semaphore);
//	free(queue);
//}
//
//void WorkQueueAddTask(WorkQueue* queue) {
//	_WriteBarrier();
//	_mm_sfence();
//	queue->taskCount++;
//	ReleaseSemaphore(queue->semaphore, 1, 0);
//}
//
//void WorkQueueClearTasks(WorkQueue* queue) {
//	queue->taskCount = queue->nextTask = queue->taskCompletionCount = 0;
//}
//
//int WorkQueueGetTasksCount(WorkQueue* queue)
//{
//	return queue->taskCount;
//}
//
//QueueTaskItem WorkQueueGetNextTask(WorkQueue* queue) {
//	QueueTaskItem res;
//	res.valid = false;
//
//	if (queue->nextTask < queue->taskCount) {
//		res.taskIndex = InterlockedIncrement((LONG volatile*)&queue->nextTask) - 1;
//		res.valid = true;
//		_ReadBarrier();
//	}
//
//	return res;
//}
//
//void WorkQueueSetTaskCompleted(WorkQueue* queue) {
//	InterlockedIncrement((LONG volatile*)&queue->taskCompletionCount);
//}
//
//bool WorkQueueWorkStillInProgress(WorkQueue* queue) {
//	return queue->taskCount != queue->taskCompletionCount;
//}
//
//void WorkQueueThreadWaitForNextTask(WorkQueue* queue, u32 timeMilliseconds) {
//	WaitForSingleObject(queue->semaphore, timeMilliseconds);
//}
//
//void WorkQueueWaitAndClear(WorkQueue* queue) {
//	while (queue->taskCount != queue->taskCompletionCount) {}
//	queue->taskCount = queue->nextTask = queue->taskCompletionCount = 0;
//}
//
//struct WorkingThread {
//	HANDLE handle;
//	int id;
//};
//
//WorkingThread* WorkingThreadCreate(int id, void* routine, void* argument) {
//	WorkingThread* thread = (WorkingThread*)calloc(1, sizeof(WorkingThread));
//	thread->id = id;
//	thread->handle = CreateThread(0, 0, (LPTHREAD_START_ROUTINE)routine, argument, 0, 0);
//	return thread;
//}
//
//void WorkingThreadRelease(WorkingThread* thread) {
//	// TODO: остановить поток?
//	CloseHandle(thread->handle);
//	free(thread);
//}

#endif
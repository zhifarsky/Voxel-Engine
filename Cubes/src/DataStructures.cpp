#include <stdlib.h>
#include <assert.h>
#include <intrin.h>
#include "DataStructures.h"

constexpr u64 PAGE_SIZE = 4 * 1024;
#define roundToMultiple(number, multiple) ((number + multiple - 1) / multiple) * multiple

void Arena::alloc(u64 capacity, u64 reserveCapacity) {
	this->capacity = roundToMultiple(capacity, PAGE_SIZE); // rounding to page size
	this->size = 0;

	mem = (u8*)VirtualAlloc(0, reserveCapacity, MEM_RESERVE, PAGE_READWRITE); // reserve large chunk of memory
	VirtualAlloc(mem, this->capacity, MEM_COMMIT, PAGE_READWRITE); // commit memory
}

void Arena::release() {
	VirtualFree(mem, 0, MEM_RELEASE);
	mem = 0;
	size = capacity = 0;
}

void* Arena::push(u64 size) {
	// if not enough capacity, commiting more memory
	if (this->size + size > capacity) {
		u64 newCapacity = roundToMultiple(this->size + size, PAGE_SIZE);
		VirtualAlloc((u8*)mem + capacity, newCapacity, MEM_COMMIT, PAGE_READWRITE); // commit memory
		capacity = newCapacity;
	}

	void* res = mem + this->size;
	this->size += size;
	return res;
}

void Arena::clear() {
	size = 0;
}

//WorkQueue::WorkQueue(int maxSemaphore) {
//	this->semaphore = CreateSemaphore(0, 0, maxSemaphore, 0);
//	this->taskCompletionCount = 0;
//	this->taskCount = 0;
//	this->nextTask = 0;
//}

WorkQueue WorkQueue::Create(u32 maxSemaphore)
{
	WorkQueue queue = {0};
	queue.semaphore = CreateSemaphore(0, 0, maxSemaphore, 0);
	return queue;
}

void WorkQueue::addTask() {
	_WriteBarrier();
	_mm_sfence();
	taskCount++;
	ReleaseSemaphore(semaphore, 1, 0);
}

void WorkQueue::clearTasks() {
	taskCount = nextTask = taskCompletionCount = 0;
}

QueueTaskItem WorkQueue::getNextTask() {
	QueueTaskItem res;
	res.valid = false;

	if (nextTask < taskCount) {
		res.taskIndex = InterlockedIncrement((LONG volatile*)&nextTask) - 1;
		res.valid = true;
		_ReadBarrier();
	}

	return res;
}

void WorkQueue::setTaskCompleted() {
	InterlockedIncrement((LONG volatile*)&taskCompletionCount);
}

bool WorkQueue::workStillInProgress() {
	return taskCount != taskCompletionCount;
}

void WorkQueue::waitAndClear() {
	while (taskCount != taskCompletionCount) {}
	taskCount = nextTask = taskCompletionCount = 0;
}
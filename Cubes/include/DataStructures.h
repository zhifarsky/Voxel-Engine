#pragma once
#include <windows.h>
#include "Typedefs.h"

template<typename Type>
struct DynamicArray {
	Type* items;
	u32 count, capacity;

	void append(Type item) {
		if (count >= capacity) {
			if (capacity == 0) capacity = 256;
			else capacity *= 2;
			items = (Type*)realloc(items, capacity * sizeof(Type));
		}
		items[count++] = item;
	}

	void clear() {
		count = 0;
	}

	Type operator[](int index) {
		return items[index];
	}
};

// арена, расширяющаяся при помощи коммитов ранее зарезервинованной памяти
// capacity округляется до размера страницы памяти
struct Arena {
	u8* mem;
	u64 size, capacity;

	void alloc(u64 capacity, u64 reserveCapacity = 128ULL * 1024 * 1024 * 1024);
	void release();
	void* push(u64 size);
	void clear();
};

struct QueueTaskItem {
	int taskIndex;
	bool valid;
};

struct WorkQueue {
	int volatile taskCount;
	int volatile nextTask;
	int volatile taskCompletionCount;
	HANDLE semaphore;

	WorkQueue(int maxSemaphore);

	void addTask(); // добавить задачу
	void clearTasks();

	QueueTaskItem getNextTask(); 	// получить индекс задачи, которую нужно выполнить
	void setTaskCompleted(); // выполнение задачи закончено

	void waitAndClear(); // ожидание выполнения всех задач
	bool workStillInProgress();

};
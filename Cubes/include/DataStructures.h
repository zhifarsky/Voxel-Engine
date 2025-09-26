#pragma once
#include "Typedefs.h"

template<typename T>
struct Array {
	T* items;
	u32 count, capacity;

	T& operator[](u32 i) {
		return items[i];
	}

	void alloc(u32 capacity) {
		count = 0;
		this->capacity = capacity;
		items = (T*)calloc(sizeof(T), capacity);
	}

	void release() {
		free(items);
		items = NULL;
		count = capacity = 0;
	}

	void append(T value) {
		if (count < capacity)
			items[count++] = value;
	}

	void insert(T value, u32 pos) {
		if (count < capacity) {
			for (s32 i = count; i > pos; i--)
				items[i] = items[i - 1];
			items[pos] = value;
			count++;
		}
	}

	void remove(u32 pos) {
		if (pos < count) {
			for (size_t i = pos; i < count - 1; i++)
				items[i] = items[i + 1];
			count--;
		}
	}

	void clear() {
		count = 0;
	}
};

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

#define WAIT_INFINITE 0xFFFFFFFF

struct WorkQueue;

WorkQueue* WorkQueueCreate(u32 maxSemaphore);

void WorkQueueAddTask(WorkQueue* queue); // добавить задачу
void WorkQueueClearTasks(WorkQueue* queue);

int WorkQueueGetTasksCount(WorkQueue* queue);
QueueTaskItem WorkQueueGetNextTask(WorkQueue* queue); 	// получить индекс задачи, которую нужно выполнить
void WorkQueueSetTaskCompleted(WorkQueue* queue); // выполнение задачи закончено

void WorkQueueThreadWaitForNextTask(WorkQueue* queue, u32 timeMilliseconds);
void WorkQueueWaitAndClear(WorkQueue* queue); // ожидание выполнения всех задач
bool WorkQueueWorkStillInProgress(WorkQueue* queue);

struct WorkingThread;

WorkingThread* WorkingThreadCreate(int id, void* routine, void* argument);
void WorkingThreadRelease(WorkingThread* thread);

#include "DataStructures.h"
#include "Tools.h"

void Arena::alloc(u64 capacity, u64 reserveCapacity) {
	this->capacity = roundToMultiple(capacity, GetMinCommitSize()); // rounding to page size
	this->size = 0;
	this->reserved = reserveCapacity;

	mem = (u8*)MemReserve(reserveCapacity);
	MemCommit(mem, this->capacity);
}

void Arena::release() {
	MemFree(mem);
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
		s64 commitSize = roundToMultiple(size + padding, GetMinCommitSize());
		assert(arena->reserved >= arena->capacity + commitSize);

		MemCommit(arena->mem + arena->capacity, commitSize);
		arena->capacity += commitSize;
	}

	arena->size += size + padding;

	if (clearToZero)
		memset(result, 0, size);

	return result;
}

void Arena::clear() {
	size = 0;
}
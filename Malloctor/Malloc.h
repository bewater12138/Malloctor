#pragma once

typedef void* PTR;
#ifdef WIN32
typedef __int32 INTPTR;
#else
typedef __int64 INTPTR;
#endif
typedef __int32 SIZE;
typedef char CHAR;
typedef __int32 INT;

enum HeapType
{
	HeapType_Default = 0,
};

extern "C" {
	INT ConstructHeap(PTR addr, SIZE initial_size, enum HeapType t);
	PTR _Malloc(struct Heap* heap, SIZE size);
	void _Free(struct Heap* heap, PTR ptr);
}
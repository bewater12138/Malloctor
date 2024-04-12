#pragma once

typedef void* PTR;

#ifdef _32Bit
typedef __int32 INTPTR;
typedef __int32 SIZE;
#else
typedef __int64 INTPTR;
typedef __int64 SIZE;
#endif
typedef char CHAR;
typedef __int32 INT;
typedef unsigned __int32 UINT;

enum HeapType
{
	HeapType_Default = 0,
};

extern "C" {
	INT ConstructHeap(PTR addr, SIZE initial_size, enum HeapType t);
	PTR _Malloc(struct Heap* heap, SIZE size);
	void _Free(struct Heap* heap, PTR ptr);
}
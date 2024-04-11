#include "Malloc.h"

#include <vector>
#include <iostream>

extern"C" int incr;

//分配、回收和扩容
void test1()
{
	void* mem = malloc(64 * 1024 * 1024);
	auto heap = (Heap*)mem;
	ConstructHeap(mem, 32 * 1024 * 1024, HeapType_Default);

	int i = 4033;
	while (i--)
	{
		auto p = _Malloc(heap, 1024);
		if (incr)
			_Free(heap, p);
	}


	auto p = _Malloc(heap, 40000);

	std::vector<void*> v;
	for (size_t i = 0; i < 5; i++)
	{
		v.push_back(_Malloc(heap, i * 300 + 4));
	}
	for (size_t i = 0; i < 5; i++)
	{
		_Free(heap, v[i]);
	}

	_Free(heap, p);
	_Free(heap, 0);
}

//变长内存块合并
void test2()
{
	void* mem = malloc(64 * 1024 * 1024);
	auto heap = (Heap*)mem;
	ConstructHeap(mem, 32 * 1024 * 1024, HeapType_Default);

	auto p1 = _Malloc(heap, 10000);
	auto p2 = _Malloc(heap, 10000);
	auto p3 = _Malloc(heap, 10000);
	_Free(heap, p1);
	_Free(heap, p2);
	_Free(heap, p3);
}

//变长内存块的扩容
void test3()
{
	void* mem = malloc(64 * 1024 * 1024);
	auto heap = (Heap*)mem;
	ConstructHeap(mem, 32 * 1024 * 1024, HeapType_Default);

	auto p1 = _Malloc(heap, 16000000);
	auto p2 = _Malloc(heap, 16000000);
	_Free(heap, p1);
	_Free(heap, p2);
}

int main()
{
	test1();
	return 0;
}
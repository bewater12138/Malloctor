#include "Malloc.h"

#include <vector>
#include <iostream>

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

//测试新分区策略
void test4()
{
	void* mem = malloc(64 * 1024 * 1024);
	auto heap = (Heap*)mem;
	ConstructHeap(mem, 32 * 1024 * 1024, HeapType_64B);

	heap->callback_errorFreeAddr = [](Heap* h, void* p) {
		std::cout << "error free: " << p << 
			"\n";
		};
	heap->callback_addrTransborder = [](Heap* h, void* p) {
		std::cout << "address transborder: " << p <<
			"\n";
		};

	int i = 4033;
	while (i--)
	{
		auto p = _Malloc(heap, 64);
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

struct Heap* g_h;
template<class C>
class myalloctor
{
public:
	using _From_primary = myalloctor;
	using value_type = C;
	using size_type = size_t;
	using difference_type = ptrdiff_t;

	constexpr myalloctor()noexcept {}
	constexpr myalloctor(const myalloctor&) = default;
	template<class Other>
	constexpr myalloctor(const myalloctor<Other>&)noexcept {}
	constexpr ~myalloctor()noexcept {}
	constexpr myalloctor& operator=(const myalloctor<C>&) = default;

	constexpr void deallocate(C* const _Ptr, const size_t _Count)
	{
		_Free(g_h, _Ptr);
	}
	C* allocate(const size_t _Count)
	{
		return (C*)_Malloc(g_h, _Count * sizeof(C));
	}
};

//用stl容器测试
void test5()
{
	g_h = (struct Heap*)malloc(64 * 1024 * 1024);
	ConstructHeap(g_h, 32 * 1024 * 1024, HeapType_Default);
	g_h->callback_malloc = [](Heap* h,void* p,SIZE size) {
		std::cout << "malloc size=" << size << "  addr=" << p << "\n";
		};
	g_h->callback_free = [](Heap* h,void* p,SIZE size) {
		if (size == dynamicSize)
			std::cout << "free dynamicBlock " << p << "  size=" << size <<"\n";
		else
			std::cout << "free " << p << "  size=" << size <<"\n";
		};

	{
		std::cout << "构造 string\n";
		std::basic_string<char, std::char_traits<char>, myalloctor<char>> str;
	};
	std::cout << "析构 string\n\n";

	std::basic_string<char, std::char_traits<char>, myalloctor<char>> str;
	while (true)
	{
		std::cout << "\n\n";
		std::string temp;
		std::cin >> temp;
		str += temp;
		std::cout << "str=" << str;
	}
}

int main()
{
	test5();
	return 0;
}
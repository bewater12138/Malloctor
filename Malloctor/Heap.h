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
#define _check(x,ret) {if(!x)return ret;}
#define MinHeapSize (1 * 1024 * 1024)
#define InitRegionMaxCount 32

#define INLINE_FUNC inline
#define STATIC_FUNC static
#define EXTERN_FUNC

INLINE_FUNC void Memcpy(PTR des, PTR src, SIZE size)
{
	CHAR* des_cur = (CHAR*)des;
	CHAR* des_end = (CHAR*)((INTPTR)des + size);
	CHAR* src_cur = (CHAR*)src;
	while (des_cur != des_end)
	{
		*des_cur = *src_cur;
		++des_cur;
		++src_cur;
	}
}

struct Array
{
	PTR arr;
	INT count;
	INT capacity;
};
INLINE_FUNC void MakeArray(PTR where, PTR arr, INT capacity, INT element_size)
{
	((struct Array*)where)->arr = arr;
	((struct Array*)where)->count = 0;
	((struct Array*)where)->capacity = capacity / element_size;
}
INLINE_FUNC void AddArrayElement(struct Array* arr, PTR value, SIZE element_size)
{
	PTR pos = (PTR)((INTPTR)arr->arr + arr->count * element_size);
	Memcpy(pos, value, element_size);
	++arr->count;
}
INLINE_FUNC PTR GetArrayEnd(struct Array* arr, SIZE element_size)
{
	return (INTPTR)arr->arr + element_size * arr->count;
}

//内存块节点
struct BlockNode
{
	PTR aim;
	struct BlockNode* next;
};

//变长内存块首部
struct VarBlockHead
{
	UINT used:1;
	SIZE size:sizeof(SIZE)*8 - 1;
	SIZE frontSize;	//内存上相邻的上一个内存块的大小（用以合并空闲块）
	struct VarBlockHead* last;
	struct VarBlockHead* next;
};
struct VarBlockRegionVirtualNodes
{
	struct VarBlockHead virtual_first;
	struct VarBlockHead virtual_last;
};
INLINE_FUNC void RemoveVarBlockHead_Unchecked(struct VarBlockHead* vbh)
{
	vbh->last->next = vbh->next;
	vbh->next->last = vbh->last;
}
INLINE_FUNC PTR GetBlockAim(struct VarBlockHead* vbh)
{
	return (PTR)((INTPTR)vbh + sizeof(struct VarBlockHead));
}
INLINE_FUNC struct VarBlockHead* Ptr2VarBlock(PTR ptr)
{
	return (struct VarBlockHead*)((INTPTR)ptr - sizeof(struct VarBlockHead));
}
INLINE_FUNC struct VarBlockHead* GetFrontVarBlock(struct VarBlockHead* vbh)
{
	return (INTPTR)vbh - vbh->frontSize - sizeof(struct VarBlockHead);
}
INLINE_FUNC struct VarBlockHead* GetBackVarBlock(struct VarBlockHead* vbh)
{
	return (INTPTR)vbh + vbh->size + sizeof(struct VarBlockHead);
}

//内存分区信息
struct RegionInfo
{
	SIZE blockSize;	//分区中每一块的内存大小
	union
	{
		struct BlockNode* first;
		struct VarBlockHead* vfirst;
	};
	union
	{
		struct BlockNode* last;
		struct VarBlockHead* vlast;
	};
	PTR regionBegin;	//在Heap::regions中表示所有相同块大小的分区的总大小，allBlockSize同理
	PTR blockBegin;
	SIZE regionSize;
	SIZE allBlockSize;
};
const SIZE unusedSize = 0;
const SIZE dynamicSize = ~0;

INLINE_FUNC void MakeRegionInfo(PTR where,PTR addr, SIZE block_size, SIZE region_size)
{
	((struct RegionInfo*)where)->blockSize = block_size;
	((struct RegionInfo*)where)->first = 0;
	((struct RegionInfo*)where)->last = 0;
	((struct RegionInfo*)where)->regionBegin = addr;
	((struct RegionInfo*)where)->regionSize = region_size;
}

struct Addr2Region
{
	struct RegionInfo region;	//用来记录这个区域的大小、块大小、地址等信息，空闲链表由主区域管理（构造堆时便存在的对应块大小的内存分区）
	struct RegionInfo* mainRegion;
};
INLINE_FUNC void MakeAddr2Region(PTR where, struct RegionInfo* region_ptr, struct RegionInfo* main_region)
{
	((struct Addr2Region*)where)->region = *region_ptr;
	((struct Addr2Region*)where)->mainRegion = main_region;
}
EXTERN_FUNC struct Addr2Region* Addr2HeapRegion(struct Heap* heap, PTR ptr);

typedef void(*Callback_ErrorFreeAddr)(struct Heap* heap, PTR addr);
typedef void(*Callback_AddrTransborder)(struct Heap* heap, PTR addr);
typedef void(*Callback_IncreaseHeap)(struct Heap* heap, PTR new_region_addr, SIZE new_region_size, SIZE block_size);
typedef void(*Callback_RegionTableOverflow)(struct Heap* heap);
typedef void(*Callback_FailedMalloc)(struct Heap* heap, SIZE size);
/*apply_size:申请的增量
返回值:允许的增量*/
typedef SIZE(*Callback_FailedIncreaseHeap)(struct Heap* heap, SIZE heap_size, SIZE apply_size);

/*
为堆区添加内存时，会将新的内存区构造为相应的链表，并将这个链表连接到对应的原内存分区上，
最后在size2RegionTable表中添加该区域的信息（首尾节点指针不使用，只记录内存区域的大小和位置信息）
*/
struct Heap
{
	/*
	整个堆区的大小，即 (INTPTR)heap + heap->size 等于堆区末尾
	*/
	SIZE size;

	/*
	堆区中的定长内存块区域中，最大的内存块
	*/
	SIZE maxBlockSize;

	/*
	Element Type: RegionInfo
	以内存块大小为序，存储每个内存区域的主区域
	构造时产生的区域均为主区域，所有块节点均挂载到主区域链表上
	扩容时新增的区域为子区域，只负责记录区域的地址大小等信息，不负责管理节点
	*/
	struct Array regions;

	/*
	变长内存区域中，内存块链表的虚拟首部和尾部
	*/
	struct VarBlockRegionVirtualNodes vnodes;

	/*
	Element Type: Addr2Region
	内存地址到内存区域的映射表
	以分区起始地址为序
	*/
	struct Array addr2RegionTable;

	/*
	回调函数表
	*/
	Callback_ErrorFreeAddr			callback_errorFreeAddr;				//释放错误地址
	Callback_AddrTransborder		callback_addrTransborder;			//释放地址越界
	Callback_IncreaseHeap			callback_increaseHeap;				//堆扩容
	Callback_FailedIncreaseHeap		callback_failedIncreaseHeap;		//堆扩容失败
	Callback_RegionTableOverflow	callback_regionTableOverflow;		//区域表溢出（致命错误）
	Callback_FailedMalloc			callback_failedMalloc;				//分配失败
};

enum HeapType
{
	HeapType_Default = 0,	//默认
	HeapType_64B = 1,		//定长块只有64字节区（占比1/2）
};

#define HeapSizeTooLess		1	//内存过少
#define UnknowHeapType		2	//未知类型

EXTERN_FUNC void* sbrk(SIZE increment);
EXTERN_FUNC INT ConstructHeap(PTR addr, SIZE initial_size, enum HeapType t);
EXTERN_FUNC INT IncreaseHeap(struct Heap* heap, SIZE increase_size, struct RegionInfo* region);
EXTERN_FUNC PTR _Malloc(struct Heap* heap, SIZE size);
EXTERN_FUNC void _Free(struct Heap* heap, PTR ptr);
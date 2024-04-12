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

struct Array
{
	PTR arr;
	INT count;
	INT capacity;
};
//内存块节点
struct BlockNode
{
	PTR aim;
	struct BlockNode* next;
};
//变长内存块首部
struct VarBlockHead
{
	UINT used : 1;
	SIZE size : sizeof(SIZE) * 8 - 1;
	SIZE frontSize;	//内存上相邻的上一个内存块的大小（用以合并空闲块）
	struct VarBlockHead* last;
	struct VarBlockHead* next;
};
struct VarBlockRegionVirtualNodes
{
	struct VarBlockHead virtual_first;
	struct VarBlockHead virtual_last;
};
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
struct Addr2Region
{
	struct RegionInfo region;	//用来记录这个区域的大小、块大小、地址等信息，空闲链表由主区域管理（构造堆时便存在的对应块大小的内存分区）
	struct RegionInfo* mainRegion;
};
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
	HeapType_Default = 0,
	HeapType_64B = 1,
};

#define HeapSizeTooLess		1	//内存过少
#define UnknowHeapType		2	//未知类型

extern "C" {
	INT ConstructHeap(PTR addr, SIZE initial_size, enum HeapType t);
	PTR _Malloc(struct Heap* heap, SIZE size);
	void _Free(struct Heap* heap, PTR ptr);
}
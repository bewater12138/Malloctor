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

//�ڴ��ڵ�
struct BlockNode
{
	PTR aim;
	struct BlockNode* next;
};

//�䳤�ڴ���ײ�
struct VarBlockHead
{
	UINT used:1;
	SIZE size:sizeof(SIZE)*8 - 1;
	SIZE frontSize;	//�ڴ������ڵ���һ���ڴ��Ĵ�С�����Ժϲ����п飩
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

//�ڴ������Ϣ
struct RegionInfo
{
	SIZE blockSize;	//������ÿһ����ڴ��С
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
	PTR regionBegin;	//��Heap::regions�б�ʾ������ͬ���С�ķ������ܴ�С��allBlockSizeͬ��
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
	struct RegionInfo region;	//������¼�������Ĵ�С�����С����ַ����Ϣ��������������������������ʱ����ڵĶ�Ӧ���С���ڴ������
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
/*apply_size:���������
����ֵ:���������*/
typedef SIZE(*Callback_FailedIncreaseHeap)(struct Heap* heap, SIZE heap_size, SIZE apply_size);

/*
Ϊ��������ڴ�ʱ���Ὣ�µ��ڴ�������Ϊ��Ӧ��������������������ӵ���Ӧ��ԭ�ڴ�����ϣ�
�����size2RegionTable������Ӹ��������Ϣ����β�ڵ�ָ�벻ʹ�ã�ֻ��¼�ڴ�����Ĵ�С��λ����Ϣ��
*/
struct Heap
{
	/*
	���������Ĵ�С���� (INTPTR)heap + heap->size ���ڶ���ĩβ
	*/
	SIZE size;

	/*
	�����еĶ����ڴ�������У������ڴ��
	*/
	SIZE maxBlockSize;

	/*
	Element Type: RegionInfo
	���ڴ���СΪ�򣬴洢ÿ���ڴ������������
	����ʱ�����������Ϊ���������п�ڵ�����ص�������������
	����ʱ����������Ϊ������ֻ�����¼����ĵ�ַ��С����Ϣ�����������ڵ�
	*/
	struct Array regions;

	/*
	�䳤�ڴ������У��ڴ������������ײ���β��
	*/
	struct VarBlockRegionVirtualNodes vnodes;

	/*
	Element Type: Addr2Region
	�ڴ��ַ���ڴ������ӳ���
	�Է�����ʼ��ַΪ��
	*/
	struct Array addr2RegionTable;

	/*
	�ص�������
	*/
	Callback_ErrorFreeAddr			callback_errorFreeAddr;				//�ͷŴ����ַ
	Callback_AddrTransborder		callback_addrTransborder;			//�ͷŵ�ַԽ��
	Callback_IncreaseHeap			callback_increaseHeap;				//������
	Callback_FailedIncreaseHeap		callback_failedIncreaseHeap;		//������ʧ��
	Callback_RegionTableOverflow	callback_regionTableOverflow;		//������������������
	Callback_FailedMalloc			callback_failedMalloc;				//����ʧ��
};

enum HeapType
{
	HeapType_Default = 0,	//Ĭ��
	HeapType_64B = 1,		//������ֻ��64�ֽ�����ռ��1/2��
};

#define HeapSizeTooLess		1	//�ڴ����
#define UnknowHeapType		2	//δ֪����

EXTERN_FUNC void* sbrk(SIZE increment);
EXTERN_FUNC INT ConstructHeap(PTR addr, SIZE initial_size, enum HeapType t);
EXTERN_FUNC INT IncreaseHeap(struct Heap* heap, SIZE increase_size, struct RegionInfo* region);
EXTERN_FUNC PTR _Malloc(struct Heap* heap, SIZE size);
EXTERN_FUNC void _Free(struct Heap* heap, PTR ptr);
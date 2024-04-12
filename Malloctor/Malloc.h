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
//�ڴ��ڵ�
struct BlockNode
{
	PTR aim;
	struct BlockNode* next;
};
//�䳤�ڴ���ײ�
struct VarBlockHead
{
	UINT used : 1;
	SIZE size : sizeof(SIZE) * 8 - 1;
	SIZE frontSize;	//�ڴ������ڵ���һ���ڴ��Ĵ�С�����Ժϲ����п飩
	struct VarBlockHead* last;
	struct VarBlockHead* next;
};
struct VarBlockRegionVirtualNodes
{
	struct VarBlockHead virtual_first;
	struct VarBlockHead virtual_last;
};
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
struct Addr2Region
{
	struct RegionInfo region;	//������¼�������Ĵ�С�����С����ַ����Ϣ��������������������������ʱ����ڵĶ�Ӧ���С���ڴ������
	struct RegionInfo* mainRegion;
};
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
	HeapType_Default = 0,
	HeapType_64B = 1,
};

#define HeapSizeTooLess		1	//�ڴ����
#define UnknowHeapType		2	//δ֪����

extern "C" {
	INT ConstructHeap(PTR addr, SIZE initial_size, enum HeapType t);
	PTR _Malloc(struct Heap* heap, SIZE size);
	void _Free(struct Heap* heap, PTR ptr);
}
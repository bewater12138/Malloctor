#include "Heap.h"

const SIZE heapRegionsSize = sizeof(struct RegionInfo) * InitRegionMaxCount;
const SIZE heapSize2RegionTableSize = sizeof(struct Addr2Region) * InitRegionMaxCount;

STATIC_FUNC void Default_Callback_ErrorFreeAddr(struct Heap* heap, PTR addr) {}
STATIC_FUNC void Default_Callback_AddrTransborder(struct Heap* heap, PTR addr) {}
STATIC_FUNC void Default_Callback_IncreaseHeap(struct Heap* heap, PTR new_region_addr, SIZE new_region_size, SIZE block_size) {}
STATIC_FUNC void Default_Callback_RegionTableOverflow(struct Heap* heap) {}
STATIC_FUNC void Default_Callback_FailedMalloc(struct Heap* heap, SIZE size) {}
STATIC_FUNC SIZE Default_Callback_FailedIncreaseHeap(struct Heap* heap, SIZE heap_size, SIZE apply_size) { return 0; }

STATIC_FUNC INT MemIn(PTR region, SIZE region_size, PTR where)
{
	return region <= where && (INTPTR)region + region_size > where;
}

EXTERN_FUNC void* sbrk(SIZE increment)
{
	return 0;
}

STATIC_FUNC void MakeRegionList(struct RegionInfo* region)
{
	INT node_count = region->regionSize / (sizeof(struct BlockNode) + region->blockSize);

	//内存链表的大小
	SIZE region_list_size = sizeof(struct BlockNode) * node_count;

	struct BlockNode virtual_node = { .aim = 0,.next = 0 };
	struct BlockNode* curr = &virtual_node;

	//当前节点的地址
	PTR curr_node_mem = region->regionBegin;
	//当前块的地址
	PTR curr_mem = (INTPTR)region->regionBegin + region_list_size;
	region->blockBegin = curr_mem;
	region->allBlockSize = region->regionSize - region_list_size;

	for (INT j = 0; j < node_count; j++)
	{
		struct BlockNode* new_node = (struct BlockNode*)curr_node_mem;
		if (j == 0)
			region->first = new_node;

		new_node->aim = curr_mem;
		new_node->next = 0;
		curr->next = new_node;
		region->last = new_node;
		curr = new_node;

		curr_node_mem = (INTPTR)curr_node_mem + sizeof(struct BlockNode);
		curr_mem = (INTPTR)curr_mem + region->blockSize;
	}
}

EXTERN_FUNC INT ConstructHeap(PTR addr, SIZE heap_size, enum HeapType t)
{
	_check(heap_size >= MinHeapSize, HeapSizeTooLess);

	struct Heap* h = (struct Heap*)addr;
	h->size = heap_size;
	h->callback_addrTransborder = Default_Callback_AddrTransborder;
	h->callback_errorFreeAddr = Default_Callback_ErrorFreeAddr;
	h->callback_failedIncreaseHeap = Default_Callback_FailedIncreaseHeap;
	h->callback_failedMalloc = Default_Callback_FailedMalloc;
	h->callback_increaseHeap = Default_Callback_IncreaseHeap;
	h->callback_regionTableOverflow = Default_Callback_RegionTableOverflow;

	//初始化数组
	PTR space_mem = (INTPTR)addr + sizeof(struct Heap);
	MakeArray(&h->regions, space_mem, heapRegionsSize, sizeof(struct RegionInfo));
	space_mem = (INTPTR)space_mem + heapRegionsSize;
	MakeArray(&h->addr2RegionTable, space_mem, heapSize2RegionTableSize, sizeof(struct Addr2Region));
	space_mem = (INTPTR)space_mem + heapSize2RegionTableSize;

	INT region_count;
	SIZE every_region_size[InitRegionMaxCount];
	SIZE every_region_block_size[InitRegionMaxCount];

	//划分区域
	switch (t)
	{
		/*
		(随便设置的数值)
		默认划分方式：
				块大小		区域占比		当前共占
			1	8			1/16		1/16
			2	32			1/16		2/16
			3	64			1/16		3/16
			4	128			1/16		4/16
			5	256			1/16		5/16
			6	512			1/16		6/16
			7	1024		1/8			8/16
			8	大于1024		剩余			1
		变长内存区采用遍历的方式管理
		*/
	case HeapType_Default: {
		region_count = 8;
		h->maxBlockSize = 1024;
		SIZE left_size = (INTPTR)addr + heap_size - (INTPTR)space_mem;
		every_region_size[0] = left_size / 16;
		every_region_size[1] = left_size / 16;
		every_region_size[2] = left_size / 16;
		every_region_size[3] = left_size / 16;
		every_region_size[4] = left_size / 16;
		every_region_size[5] = left_size / 16;
		every_region_size[6] = left_size / 8;
		every_region_size[7] = left_size;
		for (INT i = 0; i < 7; ++i)
		{
			every_region_size[7] -= every_region_size[i];
		}

		every_region_block_size[0] = 8;
		every_region_block_size[1] = 32;
		every_region_block_size[2] = 64;
		every_region_block_size[3] = 128;
		every_region_block_size[4] = 256;
		every_region_block_size[5] = 512;
		every_region_block_size[6] = 1024;
		every_region_block_size[7] = dynamicSize;
	}
		break;
	default:return UnknowHeapType;
	}

	//设置每一个分区的区域信息
	h->regions.count = region_count;
	h->addr2RegionTable.count = region_count;
	PTR current_ptr = space_mem;
	for (INT i = 0; i < region_count; ++i)
	{
		MakeRegionInfo(((struct RegionInfo*)h->regions.arr) + i, current_ptr, every_region_block_size[i], every_region_size[i]);
		current_ptr = (INTPTR)current_ptr + every_region_size[i];
	}

	//构造每一个块定长内存分区
	for (INT i = 0; i < region_count - 1; ++i)
	{
		struct RegionInfo* rinfo = &((struct RegionInfo*)h->regions.arr)[i];
		MakeRegionList(rinfo);
		MakeAddr2Region(((struct Addr2Region*)h->addr2RegionTable.arr) + i, ((struct RegionInfo*)h->regions.arr) + i, ((struct RegionInfo*)h->regions.arr) + i);
	}

	//构造块变长内存分区，初始只有一个大块
	struct RegionInfo* rinfo = &((struct RegionInfo*)h->regions.arr)[region_count - 1];
	rinfo->blockBegin = rinfo->regionBegin;
	rinfo->vfirst = rinfo->regionBegin;
	rinfo->vlast = rinfo->vfirst;
	rinfo->allBlockSize = rinfo->regionSize;

	rinfo->vfirst->frontSize = 0;
	rinfo->vfirst->used = 0;
	rinfo->vfirst->size = rinfo->regionSize - sizeof(struct VarBlockHead);
	rinfo->vfirst->last = 0;
	rinfo->vfirst->next = 0;

	MakeAddr2Region(((struct Addr2Region*)h->addr2RegionTable.arr) + region_count - 1,  ((struct RegionInfo*)h->regions.arr) + region_count - 1, ((struct RegionInfo*)h->regions.arr) + region_count - 1);

	return 0;
}

EXTERN_FUNC INT IncreaseHeap(struct Heap* heap, SIZE increase_size, struct RegionInfo* region)
{
	if (heap->addr2RegionTable.count == heap->addr2RegionTable.capacity)
	{
		heap->callback_regionTableOverflow(heap);
		return 1;
	}

	//调用sbrk函数
	sbrk(increase_size);

	PTR new_region_beg = (INTPTR)heap + heap->size;
	heap->size += increase_size;

	if (region->blockSize == dynamicSize)
	{
		//构造块变长内存分区，初始只有一个大块
		struct RegionInfo new_region;
		new_region.blockSize = dynamicSize;
		new_region.regionBegin = new_region_beg;
		new_region.regionSize = increase_size;

		new_region.blockBegin = new_region.regionBegin;
		new_region.vfirst = new_region.regionBegin;
		new_region.vlast = new_region.vfirst;
		new_region.allBlockSize = new_region.regionSize;

		new_region.vfirst->frontSize = 0;
		new_region.vfirst->used = 0;
		new_region.vfirst->size = new_region.regionSize - sizeof(struct VarBlockHead);
		new_region.vfirst->last = 0;
		new_region.vfirst->next = 0;

		//与主区域链表合并
		struct RegionInfo* main_region = ((struct RegionInfo*)heap->regions.arr) + heap->regions.count - 1;
		new_region.vfirst->next = main_region->vfirst;
		if (main_region->vfirst)
			main_region->vfirst->last = new_region.vlast;
		main_region->vfirst = new_region.vfirst;

		struct Addr2Region addr2region;
		MakeAddr2Region(&addr2region, &new_region, main_region);
		AddArrayElement(&heap->addr2RegionTable, &addr2region, sizeof(addr2region));

		heap->callback_increaseHeap(heap, new_region_beg, new_region.regionSize, new_region.blockSize);
	}
	else
	{
		struct RegionInfo new_region;
		new_region.regionBegin = new_region_beg;
		new_region.regionSize = increase_size;
		new_region.blockSize = region->blockSize;
		MakeRegionList(&new_region);
		region->regionSize += new_region.regionSize;
		region->allBlockSize += new_region.allBlockSize;

		//挂在到主区域链表中
		if (region->first == 0)
		{
			region->first = new_region.first;
			region->last = new_region.last;
		}
		else
		{
			region->last->next = new_region.first;
			region->last = new_region.last;
		}


		//添加映射表
		MakeAddr2Region(GetArrayEnd(&heap->addr2RegionTable, sizeof(struct Addr2Region)), &new_region, region);
		heap->addr2RegionTable.count++;

		heap->callback_increaseHeap(heap, new_region_beg, new_region.regionSize, new_region.blockSize);
	}

	return 0;
}

EXTERN_FUNC PTR _Malloc(struct Heap* heap, SIZE size)
{
	INT left = 0;
	INT right = heap->regions.count - 1;
	INT mid;
	PTR ret = 0;
	struct RegionInfo* aim_region = 0;

	struct RegionInfo* max_region = &((struct RegionInfo*)heap->regions.arr)[heap->regions.count - 2];
	if (size > max_region->blockSize)
	{
		ALLOC:
		//大内存块，从变长内存区中分配
		aim_region = max_region + 1;

		struct VarBlockHead* cur = aim_region->vfirst; 
		while (cur)
		{
			//内存块大到足以继续分割
			if (cur->size > size + sizeof(struct VarBlockHead))
			{
				ret = GetBlockAim(cur);

				//分割内存块
				struct VarBlockHead* split_block_node = (INTPTR)ret + size;
				split_block_node->used = 0;
				split_block_node->frontSize = size;
				split_block_node->size = cur->size - size - sizeof(struct VarBlockHead);
				split_block_node->next = cur->next;
				split_block_node->last = cur->last;

				cur->used = 1;
				cur->size = size;
				if (cur->last)
				{
					cur->last->next = split_block_node;
				}
				else  //是头节点的话，就要直接修改区域信息中的头指针
				{
					aim_region->vfirst = split_block_node;
				}

				if (cur->next)
				{
					split_block_node->next->last = split_block_node;
				}

				//修改内存上相邻的下一个内存块的首部
				struct VarBlockHead* back_block = GetBackVarBlock(split_block_node);
				struct Addr2Region* addr2region = Addr2HeapRegion(heap, back_block);
				if (addr2region && MemIn(addr2region->region.blockBegin, addr2region->region.allBlockSize, back_block))
				{
					back_block->frontSize = split_block_node->size;
				}

			}
			//内存块足够分配
			else if (cur->size >= size)
			{
				ret = GetBlockAim(cur);
				cur->used = 1;

				//修改链表
				if (cur->last)
				{
					RemoveVarBlockHead_Unchecked(cur);
				}
				else
				{
					aim_region->vfirst = cur->next;
				}
			}
			if (ret)break;

			cur = cur->next;
		}


		if (ret)
		{
			aim_region->vfirst->last = 0;
		}
		else
		{	//分配失败，执行后续操作
			//没有足够的空间，扩容堆区
			IncreaseHeap(heap, aim_region->regionSize >= size + sizeof(struct VarBlockHead) ? aim_region->regionSize : size + sizeof(struct VarBlockHead), aim_region);
			goto ALLOC;
		}

	}
	else
	{
		ALLOC2:
		//二分查找块大小最接近的内存区域
		while (1)
		{
			mid = (left + right) / 2;
			struct RegionInfo* mid_region = (struct RegionInfo*)heap->regions.arr + mid;
			if (mid_region->blockSize == size)
			{
				aim_region = mid_region;
				break;
			}
			else if (mid_region->blockSize < size)
			{
				left = mid + 1;
			}
			else
			{
				struct RegionInfo* last_rinfo = mid_region - 1;
				if (last_rinfo < heap->regions.arr || last_rinfo->blockSize < size)
				{
					aim_region = mid_region;
					break;
				}

				right = mid - 1;
			}
		}

		//扩容堆区
		if (aim_region->first == 0)
		{
			INT error = IncreaseHeap(heap, aim_region->regionSize, aim_region);
			if (error)
			{
				heap->callback_failedMalloc(heap, size);
				return 0;
			}
			goto ALLOC2;
		}

		ret = aim_region->first->aim;
		aim_region->first->aim = 0;	//分配后的节点，将指向设为0，表示已经被分配
		aim_region->first = aim_region->first->next;
	}

	return ret;
}

EXTERN_FUNC struct Addr2Region* Addr2HeapRegion(struct Heap* heap, PTR ptr)
{
	//将地址映射到内存区
	INT left = 0;
	INT right = heap->addr2RegionTable.count - 1;
	INT mid;
	struct Addr2Region* aim = 0;

	while (1)
	{
		mid = (left + right) / 2;
		struct Addr2Region* mid_size2region = (struct Addr2Region*)heap->addr2RegionTable.arr + mid;

		//目标地址在该内存区中
		if (MemIn(mid_size2region->region.blockBegin, mid_size2region->region.allBlockSize, ptr))
		{
			aim = mid_size2region;
			break;
		}
		else if (mid_size2region->region.blockBegin > ptr)
		{
			right = mid - 1;
		}
		else
		{
			left = mid + 1;
		}

		//如果左右指针已经相遇，则进行最后一次比较
		if ((left == right && left == 0)
			|| (left == right && left == heap->addr2RegionTable.count - 1))
		{
			mid_size2region = (struct Addr2Region*)heap->addr2RegionTable.arr + left;
			if (MemIn(mid_size2region->region.blockBegin, mid_size2region->region.allBlockSize, ptr))
			{
				aim = mid_size2region;
			}

			break;
		}
	}
	return aim;
}

EXTERN_FUNC void _Free(struct Heap* heap, PTR ptr)
{
	//将地址映射到内存区
	struct Addr2Region* aim_addr2region = Addr2HeapRegion(heap, ptr);

	if (!aim_addr2region)
	{	//不是堆区中的地址
		heap->callback_addrTransborder(heap, ptr);
		return;
	}

	if (aim_addr2region->region.blockSize == dynamicSize)
	{
		struct RegionInfo* var_region = aim_addr2region->mainRegion;

		struct VarBlockHead* aim_node = Ptr2VarBlock(ptr);
		SIZE size = aim_node->size;
		if (!aim_node->used)
		{
			return;
		}

		//合并前后空闲内存块
		struct VarBlockHead* back_node =GetBackVarBlock(aim_node);
		INT have_back_node = MemIn(aim_addr2region->region.blockBegin, aim_addr2region->region.allBlockSize, back_node);
		INT back_node_free = have_back_node && !back_node->used;
		struct VarBlockHead* front_node = 0;
		INT front_node_free = 0;
		if (aim_node->frontSize != 0)
		{
			front_node = GetFrontVarBlock(aim_node);
			front_node_free = !front_node->used;
		}

		//如果相邻内存区域的前面是空闲内存块
		if (front_node_free)
		{
			if (back_node_free)
			{
				//前后都为空闲，将三个内存块合并，合并后为front_node
				front_node->size += (aim_node->size + back_node->size + (sizeof(struct VarBlockHead) << 1));

				//从链表移除back_node
				if (back_node->last)
				{
					if(back_node->next)
						RemoveVarBlockHead_Unchecked(back_node);
					else
					{
						back_node->last->next = 0;
					}
				}
				else
				{
					var_region->vfirst = back_node->next;
					var_region->vfirst->last = 0;
				}

				//修改合并后，相邻的下一个内存块
				back_node = GetBackVarBlock(front_node);
				if (MemIn(aim_addr2region->region.blockBegin, aim_addr2region->region.allBlockSize, back_node))
				{
					back_node->frontSize = front_node->size;
				}

				return;
			}
			else
			{
				front_node->size += (aim_node->size + sizeof(struct VarBlockHead));
				//修改合并后，相邻的下一个内存块
				back_node->frontSize = front_node->size;

				return;
			}
		}
		else if(back_node_free)
		{
			//与相邻节点合并
			aim_node->size += (back_node->size + sizeof(struct VarBlockHead));
			aim_node->next = back_node->next;
			aim_node->last = back_node->last;
			aim_node->used = 0;

			//移除内存相邻节点
			if (aim_node->next)
			{
				aim_node->next->last = aim_node;
				if (aim_node->last)
				{
					aim_node->last->next = aim_node;
					RemoveVarBlockHead_Unchecked(back_node);
				}
				else
				{
					var_region->vfirst = aim_node;
					var_region->vfirst->last = 0;
				}
			}
			else
			{
				if (aim_node->last)
				{
					aim_node->last->next = aim_node;
				}
				else
				{
					var_region->vfirst = aim_node;
					var_region->vfirst->last = 0;
				}
			}

			//修改合并后，相邻的下一个内存块
			back_node = GetBackVarBlock(aim_node);
			if (MemIn(aim_addr2region->region.blockBegin, aim_addr2region->region.allBlockSize, back_node))
			{
				back_node->frontSize = aim_node->size;
			}

			return;
		}

		//添加到空闲链表
		if (!aim_node->used)
		{
			return;
		}

		size = aim_node->size;
		aim_node->used = 0;
		aim_node->next = var_region->vfirst;
		if(var_region->vfirst)
			var_region->vfirst->last = aim_node;
		var_region->vfirst = aim_node;

		return;
	}
	else
	{
		SIZE offset = (INTPTR)ptr - (INTPTR)aim_addr2region->region.blockBegin;

		//如果模数不为0，则不是正确的堆地址
		if (offset % aim_addr2region->region.blockSize)
		{
			heap->callback_errorFreeAddr(heap, ptr);
		}
		else
		{
			//根据偏移计算出对应的节点位置
			INT node_pos = offset / aim_addr2region->region.blockSize;
			struct BlockNode* aim_node = (struct BlockNode*)aim_addr2region->region.regionBegin + node_pos;

			if (aim_node->aim != 0)
			{
				return;
			}
			aim_node->aim = ptr;
			aim_node->next = aim_addr2region->mainRegion->first;	//添加到空闲节点链表的头部
			aim_addr2region->mainRegion->first = aim_node;


			return;
		}
	}

}

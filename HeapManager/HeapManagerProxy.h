#pragma once

#include <assert.h>
#include <new>
#include "Log.h"

namespace HeapManagerProxy
{
	struct MemoryBlock
	{
		void* pBaseAddress;
		size_t BlockSize;
		struct MemoryBlock* pNextBlock;
	};

	class HeapManager
	{
	private:
		struct MemoryBlock* pFreeList = nullptr;
		struct MemoryBlock* pOutstandingAllocations = nullptr;

		void* pHeapStartAddress = nullptr;
		void* pHeapEndAddress = nullptr;

	public:
		static size_t s_MinumumToLeave;

		HeapManager() = delete; // remove default constructor
		HeapManager(void* pHeapAllocatorMemory, const size_t sizeHeap);
		virtual ~HeapManager() {}

		// allocate a block of memeory
		void* alloc(const size_t sizeAlloc, const unsigned int alignment = 4);

		bool free(const void* pPtr);

		// garbage collect, merge empty block
		void Collect();

		bool Contains(const void* pPtr);

		bool IsAllocated(const void* pPtr);	

		void ShowFreeBlocks();

		void ShowOutstandingAllocations();

		void Destroy();

		MemoryBlock* FindFirstFittingFreeBlock(size_t i_size);

		MemoryBlock* FindBestFittingFreeBlock(size_t i_size);

		MemoryBlock* GetFreeMemoryBlock();

		void ReturnMemoryBlock(MemoryBlock* i_pFreeBlock);

		size_t GetLargestFreeBlock();
	};

	// create a HeapManager
	HeapManager* CreateHeapManager(void* pHeapMemory, const size_t sizeHeap, const unsigned int numDescriptors);
}
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

		void* pHeapLeft = nullptr;
		void* pHeapRight = nullptr;

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

		size_t GetLargestFreeBlock(const unsigned int alignment = 4);

		void Destroy();

	private:
		MemoryBlock* FindFirstFittingFreeBlock(const size_t i_size, const unsigned int alignment = 4);

		MemoryBlock* FindBestFittingFreeBlock(const size_t i_size, const unsigned int alignment = 4);

		MemoryBlock* GetFreeMemoryBlock(bool bBreakConnect = true);

		MemoryBlock* CreateFreeMemoryBlock();

		void ReturnMemoryBlock(MemoryBlock* i_pFreeBlock);

		MemoryBlock* FindPrevFreeBlock(MemoryBlock* i_pBlock);
	};

	// create a HeapManager
	HeapManager* CreateHeapManager(void* pHeapMemory, const size_t sizeHeap, const unsigned int numDescriptors);

	inline bool IsPowerOfTwo(unsigned int value)
	{
		return !(value == 0) && !(value & (value - 1));
	}

	inline unsigned int AlignDown(unsigned int i_value, unsigned int i_align)
	{
		assert(i_align);
		assert(IsPowerOfTwo(i_align));

		return i_value & ~(i_align - 1);
	}

	inline unsigned int AlignUp(unsigned int i_value, unsigned int i_align)
	{
		assert(i_align);
		assert(IsPowerOfTwo(i_align));

		return (i_value + (i_align - 1)) & ~(i_align - 1);
	}

	inline void* AlignDownAddress(void* i_pAddr, unsigned int i_align = 4)
	{
		return reinterpret_cast<void*>(AlignDown(reinterpret_cast<uintptr_t>(i_pAddr), i_align));
	}

	inline void* AlignUpAddress(void* i_pAddr, unsigned int i_align = 4)
	{
		return reinterpret_cast<void*>(AlignUp(reinterpret_cast<uintptr_t>(i_pAddr), i_align));
	}
}
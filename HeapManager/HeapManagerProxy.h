#pragma once

#include <assert.h>
#include <new>
#include "Log.h"

namespace HeapManagerProxy
{
	struct MemoryBlock
	{
		void* pBaseAddress;
		struct MemoryBlock* pNextBlock;
		size_t BlockSize;
		MemoryBlock(void* i_pBaseAddress, MemoryBlock* i_pNextBlock, size_t i_BlockSize) :
			pBaseAddress(i_pBaseAddress),
			pNextBlock(i_pNextBlock),
			BlockSize(i_BlockSize) {}
	};

	class HeapManager
	{
	private:
		struct MemoryBlock* pFreeList = nullptr;
		struct MemoryBlock* pOutstandingAllocations = nullptr;

		void* pHeapStartAddress = nullptr;
		void* pHeapEndAddress = nullptr;

		void* pHeapAllocedEndAddress = nullptr;

	public:
		static size_t s_MinumumToLeave;

		static unsigned char _bNoMansLandFill;
		static unsigned char _bAlignLandFill;
		static unsigned char _bDeadLandFill;
		static unsigned char _bCleanLandFill;

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

		bool IsEmpty() const { return pOutstandingAllocations == nullptr; }

	private:
		MemoryBlock* FindFirstFittingFreeBlock(const size_t i_size, const unsigned int alignment = 4);

		MemoryBlock* FindBestFittingFreeBlock(const size_t i_size, const unsigned int alignment = 4);

		MemoryBlock* GetFreeMemoryBlockDescriptor();

		MemoryBlock* CreateFreeMemoryBlockDescriptor();

		void ReturnMemoryBlockDescriptor(MemoryBlock* i_pFreeBlock);
	};

	// create a HeapManager
	HeapManager* CreateHeapManager(const size_t sizeHeap, const unsigned int numDescriptors, void* pHeapMemory = nullptr);

	void Destroy(HeapManager* pHeapManager);

	inline bool IsPowerOfTwo(size_t value)
	{
		return !(value == 0) && !(value & (value - 1));
	}

	inline uintptr_t AlignDown(uintptr_t i_value, uintptr_t i_align)
	{
		assert(i_align);
		assert(IsPowerOfTwo(i_align));

		return i_value & ~(i_align - 1);
	}

	inline uintptr_t AlignUp(uintptr_t i_value, uintptr_t i_align)
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
#pragma once
#include "IAllocator.h"

namespace HeapManagerProxy
{
	typedef struct MemoryBlock {
		void* pBaseAddress;
		MemoryBlock* pNextBlock;
		size_t BlockSize;
		
		MemoryBlock(void* i_pBaseAddress, MemoryBlock* i_pNextBlock, size_t i_BlockSize) :
			pBaseAddress(i_pBaseAddress),
			pNextBlock(i_pNextBlock),
			BlockSize(i_BlockSize) {}
	} MemoryBlock;

	class HeapAllocator: public IAllocator
	{
	public:
		HeapAllocator() = delete; // remove default constructor
		HeapAllocator(void* i_pAllocatorMemory, const size_t sizeHeap);
		virtual ~HeapAllocator();

		// allocate a block of memory
		virtual void* alloc(const size_t sizeAlloc, const unsigned int alignment = 4) override;

		virtual bool free(const void* pPtr) override;

		// garbage collect, merge empty block
		virtual void Collect() override;

		virtual bool Contains(const void* pPtr) override;

		virtual bool IsAllocated(const void* pPtr) override;

		virtual void ShowFreeBlocks() override;

		virtual void ShowOutstandingAllocations() override;

		virtual void Destroy() override;

		virtual bool IsEmpty() override { return pOutstandingAllocations == nullptr; }

		size_t GetLargestFreeBlock(const unsigned int alignment = 4);

		static size_t s_MinumumToLeave;

	private:
		MemoryBlock* pFreeList = nullptr;
		MemoryBlock* pOutstandingAllocations = nullptr;

		void* pHeapStartAddress = nullptr;
		void* pHeapEndAddress = nullptr;

		void* pHeapAllocedEndAddress = nullptr;

		MemoryBlock* FindFirstFittingFreeBlock(const size_t i_size,
			const unsigned int alignment = 4);

		MemoryBlock* FindBestFittingFreeBlock(const size_t i_size,
			const unsigned int alignment = 4);

		MemoryBlock* GetFreeMemoryBlockDescriptor();

		MemoryBlock* CreateFreeMemoryBlockDescriptor();

		void ReturnMemoryBlockDescriptor(MemoryBlock* i_pFreeBlock);
	};
}


#include "HeapAllocator.h"
#include "string.h"
#include <assert.h>
#include <new>
#include "stdio.h"
#include "Utils.h"

namespace HeapManagerProxy
{
	size_t HeapAllocator::s_MinumumToLeave = sizeof(MemoryBlock);

	HeapAllocator::HeapAllocator(void* i_pAllocatorMemory, const size_t sizeHeap)
	{
		pFreeList = nullptr;
		pOutstandingAllocations = nullptr;

		pHeapStartAddress = i_pAllocatorMemory;
		pHeapEndAddress = static_cast<char*>(pHeapStartAddress) + sizeHeap;

		pHeapAllocedEndAddress = pHeapEndAddress;
		memset(pHeapStartAddress, _bDeadLandFill, sizeHeap);
	}

	HeapAllocator::~HeapAllocator()
	{
		assert(pOutstandingAllocations == nullptr);
	}

	void* HeapAllocator::alloc(const size_t sizeAlloc, const unsigned int alignment /*= 4*/)
	{
		// GUARD_BAND only exist in _DEBUG
		MemoryBlock* pBlockDescriptor = FindFirstFittingFreeBlock(GUARD_BAND_SIZE + sizeAlloc + GUARD_BAND_SIZE, alignment);
		pBlockDescriptor = pBlockDescriptor == nullptr ? GetFreeMemoryBlockDescriptor() : pBlockDescriptor;
		if (pBlockDescriptor == nullptr)
			return nullptr;

		size_t maxCapacity = 0;
		void* pAvailableStart = Utils::AlignUpAddress(static_cast<char*>(pHeapStartAddress) + s_MinumumToLeave + GUARD_BAND_SIZE, alignment);
		void* pAvailableEnd = pHeapEndAddress;

		if (pAvailableEnd > pAvailableStart)
			maxCapacity = static_cast<char*>(pAvailableEnd) - static_cast<char*>(pAvailableStart);

		if (maxCapacity < sizeAlloc + GUARD_BAND_SIZE) // left memory not enough
		{
			ReturnMemoryBlockDescriptor(pBlockDescriptor);
			return nullptr;
		}

		// the alignment is for user memory start point
		char* pBlockStartAddress = static_cast<char*>(Utils::AlignDownAddress(static_cast<char*>(pHeapEndAddress) - GUARD_BAND_SIZE - sizeAlloc, alignment));

		pBlockDescriptor->pBaseAddress = pBlockStartAddress - GUARD_BAND_SIZE;
		pBlockDescriptor->BlockSize = static_cast<char*>(pHeapEndAddress) - (pBlockStartAddress - GUARD_BAND_SIZE);

		pHeapEndAddress = pBlockDescriptor->pBaseAddress;

		assert(pHeapStartAddress <= pHeapEndAddress);

		pBlockDescriptor->pNextBlock = pOutstandingAllocations;
		pOutstandingAllocations = pBlockDescriptor;

		char* pUserMemory = static_cast<char*>(pBlockDescriptor->pBaseAddress) + GUARD_BAND_SIZE;
		memset(pUserMemory - GUARD_BAND_SIZE, _bNoMansLandFill, GUARD_BAND_SIZE);	// head guard
		memset(pUserMemory, _bCleanLandFill, sizeAlloc); // user alloc memory
		memset(pUserMemory + sizeAlloc, _bNoMansLandFill, GUARD_BAND_SIZE); // tail guard
		memset(pUserMemory + sizeAlloc + GUARD_BAND_SIZE, _bAlignLandFill, pBlockDescriptor->BlockSize - (GUARD_BAND_SIZE + sizeAlloc + GUARD_BAND_SIZE)); // alignment

		// printf("allocated memory %p\n", pUserMemory - GUARD_BAND_SIZE);
		return pUserMemory;
	}

	bool HeapAllocator::free(const void* pPtr)
	{
		// assert(Contains(pPtr));
		if (Contains(pPtr) == false)
		{
			printf("Default Heap does not contains %p\n", pPtr);
			return false;
		}

		// printf("start free %p\n", pPtr);

		MemoryBlock* pCurBlock = pOutstandingAllocations;
		MemoryBlock* pPrevBlock = nullptr;
		while (pCurBlock)
		{
			if (static_cast<char*>(pCurBlock->pBaseAddress) + GUARD_BAND_SIZE == pPtr)
			{
				if (pPrevBlock)
					pPrevBlock->pNextBlock = pCurBlock->pNextBlock;
				else
					pOutstandingAllocations = pCurBlock->pNextBlock;
				pCurBlock->pNextBlock = nullptr;
				ReturnMemoryBlockDescriptor(pCurBlock);
				break;
			}

			pPrevBlock = pCurBlock;
			pCurBlock = pCurBlock->pNextBlock;
		}

		return (pCurBlock != nullptr);
	}

	void HeapAllocator::Collect()
	{
		if (pFreeList == nullptr)
			return;

		MemoryBlock* pCurBlock = pFreeList;
		MemoryBlock* pNextBlock = pCurBlock->pNextBlock;

		while (pCurBlock && pNextBlock)
		{
			// ship all empty block descriptor
			if (pCurBlock->BlockSize > 0)
			{
				if (static_cast<char*>(pCurBlock->pBaseAddress) + pCurBlock->BlockSize == pNextBlock->pBaseAddress)
				{
					pCurBlock->pNextBlock = pNextBlock->pNextBlock;
					pCurBlock->BlockSize += pNextBlock->BlockSize;

					pNextBlock->BlockSize = 0;
					pNextBlock->pBaseAddress = nullptr;

					pNextBlock->pNextBlock = pFreeList;
					pFreeList = pNextBlock;

					// if merged, not move current block point
					pNextBlock = pCurBlock->pNextBlock;
				}
			}
			else
			{ 
				pCurBlock = pNextBlock;
				pNextBlock = pNextBlock->pNextBlock;
			}
		}

		// check the last
		if (pCurBlock)
		{
			if (static_cast<char*>(pCurBlock->pBaseAddress) + pCurBlock->BlockSize == pHeapStartAddress)
			{
				pHeapStartAddress = pCurBlock->pBaseAddress;
				pCurBlock->BlockSize = 0;
				pCurBlock->pBaseAddress = nullptr;

				pCurBlock->pNextBlock = pFreeList;
				pFreeList = pCurBlock;
			}
		}
	}

	bool HeapAllocator::Contains(const void* pPtr)
	{
		return (pPtr >= pHeapStartAddress && pPtr <= pHeapAllocedEndAddress);
	}

	bool HeapAllocator::IsAllocated(const void* pPtr)
	{
		MemoryBlock* pBlock = pOutstandingAllocations;
		while (pBlock)
		{
			if (pBlock->BlockSize > 0 && (static_cast<char*>(pBlock->pBaseAddress) + GUARD_BAND_SIZE == pPtr))
				break;

			pBlock = pBlock->pNextBlock;
		}

		return (pBlock != nullptr);
	}

	void HeapAllocator::ShowFreeBlocks()
	{
		printf("Free Blocks:\n");
		printf("Start\t Address\tEnd\t Address\tSize\t\n");
		if (pHeapEndAddress > pHeapStartAddress)
			printf("0x%p\t0x%p\t%zu\n", pHeapStartAddress,
				pHeapEndAddress, static_cast<char*>(pHeapEndAddress) - static_cast<char*>(pHeapStartAddress));

		MemoryBlock* pCurBlock = pFreeList;
		while (pCurBlock)
		{
			if (pCurBlock->BlockSize > 0)
			{
				void* endPoint = static_cast<char*>(pCurBlock->pBaseAddress) + pCurBlock->BlockSize;
				printf("0x%p\t0x%p\t%zu\n", pCurBlock->pBaseAddress, endPoint, pCurBlock->BlockSize);
			}

			pCurBlock = pCurBlock->pNextBlock;
		}
	}

	void HeapAllocator::ShowOutstandingAllocations()
	{
		printf("Allocated Blocks:\n");
		printf("Start\t Address\tEnd\t Address\tSize\t\n");
		MemoryBlock* pCurBlock = pOutstandingAllocations;
		while (pCurBlock)
		{
			if (pCurBlock->BlockSize > 0)
			{
				void* endPoint = static_cast<char*>(pCurBlock->pBaseAddress) + pCurBlock->BlockSize;
				printf("0x%p\t0x%p\t%zu\n", pCurBlock->pBaseAddress, endPoint, pCurBlock->BlockSize);
			}

			pCurBlock = pCurBlock->pNextBlock;
		}
	}

	void HeapAllocator::Destroy()
	{

	}

	// the largest real size for user memory
	size_t HeapAllocator::GetLargestFreeBlock(const unsigned int alignment /*=4*/)
	{

		size_t iMaxCapacity = 0;

		char* pMaxUserMemoryEnd = static_cast<char*>(pHeapEndAddress) - GUARD_BAND_SIZE;
		char* pMaxUserMemoryStart = static_cast<char*>(Utils::AlignUpAddress(static_cast<char*>(pHeapStartAddress) + s_MinumumToLeave + GUARD_BAND_SIZE, alignment));

		if (pMaxUserMemoryStart < pMaxUserMemoryEnd)
			iMaxCapacity = pMaxUserMemoryEnd - pMaxUserMemoryStart;
	
		MemoryBlock* pCurBlock = pFreeList;

		while (pCurBlock)
		{

			if (pCurBlock->BlockSize > 0)
			{
				void* start = Utils::AlignUpAddress(static_cast<char*>(pCurBlock->pBaseAddress) + GUARD_BAND_SIZE, alignment);
				void* end = static_cast<char*>(pCurBlock->pBaseAddress) + pCurBlock->BlockSize;

				size_t capacity = static_cast<char*>(end) - static_cast<char*>(start) - GUARD_BAND_SIZE;
				iMaxCapacity = capacity > iMaxCapacity ? capacity : iMaxCapacity;
			}

			pCurBlock = pCurBlock->pNextBlock;
		}

		return iMaxCapacity;
	}

	// required size i_size is with header and tail guard band
	// the real user memory size is i_size - GUARD_BAND_SIZE - GUARD_BAND_SIZE
	MemoryBlock* HeapAllocator::FindFirstFittingFreeBlock(const size_t i_size, const unsigned int alignment /*= 4*/)
	{
		MemoryBlock* pCurBlock = pFreeList;
		MemoryBlock* pPrevBlock = nullptr;

		char* pCurBlockEndAddress = nullptr;
		char* pUserMemory = nullptr; // user memory address

		while (pCurBlock)
		{
			if (pCurBlock->BlockSize >= i_size) // quick pre-filter
			{
				pCurBlockEndAddress = static_cast<char*>(pCurBlock->pBaseAddress) + pCurBlock->BlockSize;
				// the alignment is for user memory
				pUserMemory = static_cast<char*>(Utils::AlignDownAddress(static_cast<char*>(pCurBlockEndAddress) - i_size + GUARD_BAND_SIZE, alignment));

				// this block is large enough after alignment and guard
				if (pUserMemory - GUARD_BAND_SIZE >= pCurBlock->pBaseAddress)
					break;
			}

			pPrevBlock = pCurBlock;
			pCurBlock = pCurBlock->pNextBlock;
		}

		if (pCurBlock)
		{
			if (pUserMemory - GUARD_BAND_SIZE == pCurBlock->pBaseAddress) // use the whole block
			{
				if (pPrevBlock)
					pPrevBlock->pNextBlock = pCurBlock->pNextBlock;
				else
					pFreeList = pCurBlock->pNextBlock;

				// disconnect from free list
				pCurBlock->pNextBlock = nullptr;
			}
			else // split into used and free blocks
			{
				MemoryBlock* pNewBlock = GetFreeMemoryBlockDescriptor();

				if (!pNewBlock) // failed to create new memory block descriptor
					return nullptr;

				pNewBlock->pBaseAddress = pCurBlock->pBaseAddress;
				pNewBlock->BlockSize = pUserMemory - GUARD_BAND_SIZE - static_cast<char*>(pCurBlock->pBaseAddress);

				// disconnect from free list
				pCurBlock->pBaseAddress = pUserMemory - GUARD_BAND_SIZE;
				pCurBlock->BlockSize = pCurBlock->BlockSize - pNewBlock->BlockSize;
				pCurBlock->pNextBlock = nullptr;

				ReturnMemoryBlockDescriptor(pNewBlock);
			}
		}

		return pCurBlock;
	}

	MemoryBlock* HeapAllocator::FindBestFittingFreeBlock(const size_t i_size, const unsigned int alignment /*= 4*/)
	{
		return nullptr;
	}

	MemoryBlock* HeapAllocator::GetFreeMemoryBlockDescriptor()
	{
		MemoryBlock* pCurBlock = pFreeList;
		MemoryBlock* pPrevBlock = nullptr;

		while (pCurBlock)
		{
			if (pCurBlock->BlockSize == 0) // get a free memory block descriptor
			{
				if (pPrevBlock)
					pPrevBlock->pNextBlock = pCurBlock->pNextBlock;
				else
					pFreeList = pCurBlock->pNextBlock;

				break;
			}

			pPrevBlock = pCurBlock;
			pCurBlock = pCurBlock->pNextBlock;
		}

		if (pCurBlock == nullptr)// no free memory block descriptor left
			pCurBlock = CreateFreeMemoryBlockDescriptor();

		return pCurBlock;
	}

	MemoryBlock* HeapAllocator::CreateFreeMemoryBlockDescriptor()
	{
		if (static_cast<char*>(pHeapStartAddress) + sizeof(MemoryBlock) > pHeapEndAddress)
			return nullptr; // out of all memory

		MemoryBlock* pNewBlock = new (pHeapStartAddress) MemoryBlock(nullptr, nullptr, 0);

		pHeapStartAddress = pNewBlock + 1;

		return pNewBlock;
	}

	void HeapAllocator::ReturnMemoryBlockDescriptor(MemoryBlock* i_pFreeBlock)
	{
		if (pFreeList == nullptr)
		{
			pHeapEndAddress = static_cast<char*>(pHeapEndAddress) + i_pFreeBlock->BlockSize;
			pHeapStartAddress = static_cast<char*>(pHeapStartAddress) - sizeof(MemoryBlock);
		}
		else if (i_pFreeBlock->BlockSize == 0)
		{
			i_pFreeBlock->pNextBlock = pFreeList;
			pFreeList = i_pFreeBlock;
		}
		else
		{
			MemoryBlock* pCurBlock = pFreeList;
			MemoryBlock* pPrevBlock = nullptr;
			while(pCurBlock)
			{
				if (pCurBlock->BlockSize > 0 && static_cast<char*>(i_pFreeBlock->pBaseAddress) + i_pFreeBlock->BlockSize < pCurBlock->pBaseAddress)
					break;

				pPrevBlock = pCurBlock;
				pCurBlock = pCurBlock->pNextBlock;
			}

			if (pCurBlock)
			{
				// neighbor block. merge
				if (static_cast<char*>(i_pFreeBlock->pBaseAddress) + i_pFreeBlock->BlockSize == pCurBlock->pBaseAddress)
				{
					pCurBlock->BlockSize += i_pFreeBlock->BlockSize;
					pCurBlock->pBaseAddress = i_pFreeBlock->pBaseAddress;

					i_pFreeBlock->BlockSize = 0;
					i_pFreeBlock->pBaseAddress = nullptr;

					i_pFreeBlock->pNextBlock = pFreeList;
					pFreeList = i_pFreeBlock;

					if (pPrevBlock && static_cast<char*>(pPrevBlock->pBaseAddress) + pPrevBlock->BlockSize == pCurBlock->pBaseAddress)
					{
						pPrevBlock->BlockSize += pCurBlock->BlockSize;
						pPrevBlock->pNextBlock = pCurBlock->pNextBlock;

						pCurBlock->BlockSize = 0;
						pCurBlock->pBaseAddress = nullptr;

						pCurBlock->pNextBlock = pFreeList;
						pFreeList = pCurBlock;
					}
				}
				else
				{
					if (pPrevBlock)
						pPrevBlock->pNextBlock = i_pFreeBlock;
					else
						pFreeList = i_pFreeBlock;

					i_pFreeBlock->pNextBlock = pCurBlock;
				}
			}
			else
			{
				if (static_cast<char*>(pPrevBlock->pBaseAddress) + pPrevBlock->BlockSize == i_pFreeBlock->pBaseAddress)
				{
					pPrevBlock->BlockSize += i_pFreeBlock->BlockSize;
					pPrevBlock->pNextBlock = nullptr;

					i_pFreeBlock->BlockSize = 0;
					i_pFreeBlock->pBaseAddress = nullptr;

					i_pFreeBlock->pNextBlock = pFreeList;
					pFreeList = i_pFreeBlock;
				}
				else
				{
					pPrevBlock->pNextBlock = i_pFreeBlock;
					i_pFreeBlock->pNextBlock = nullptr;
				}
			}
		}
	}
}
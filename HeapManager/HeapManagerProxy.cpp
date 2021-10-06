#include "HeapManagerProxy.h"
#include <stdio.h>

namespace HeapManagerProxy
{
	size_t HeapManager::s_MinumumToLeave = 0;// sizeof(MemoryBlock);

	HeapManager* CreateHeapManager(void* pHeapMemory, const size_t sizeHeap, const unsigned int numDescriptors)
	{
		assert((pHeapMemory != nullptr) && (sizeHeap > 0));

		HeapManager* pManager = reinterpret_cast<HeapManager*>(pHeapMemory);

		void* pHeapAllocatorMemory = pManager + 1;

		return new (pHeapMemory) HeapManager(pHeapAllocatorMemory, sizeHeap - sizeof(HeapManager));
	}

	HeapManager::HeapManager(void* pHeapAllocatorMemory, const size_t sizeHeap)
	{
		assert((pHeapAllocatorMemory != nullptr) && (sizeHeap > sizeof(MemoryBlock)));

		pHeapStartAddress = pHeapAllocatorMemory;
		pHeapEndAddress = static_cast<char*>(pHeapAllocatorMemory) + sizeHeap - sizeof(MemoryBlock);

		pFreeList = reinterpret_cast<MemoryBlock*>(pHeapEndAddress);

		pFreeList->pBaseAddress = pHeapAllocatorMemory;
		pFreeList->BlockSize = 0;
		pFreeList->pNextBlock = nullptr;

		pHeapAllocatorMemory = nullptr;
	}

	// allocate a block of memeory
	void* HeapManager::alloc(const size_t sizeAlloc, const unsigned int alignment)
	{
		MemoryBlock* pBlock = FindFirstFittingFreeBlock(sizeAlloc);

		// no free block fit the size
		if (pBlock == nullptr && 
			(static_cast<char*>(pHeapEndAddress) - sizeAlloc) >= static_cast<char*>(pHeapStartAddress))
		{
			pBlock = GetFreeMemoryBlock();
			pBlock->pBaseAddress = pHeapStartAddress;
			pBlock->BlockSize = sizeAlloc;

			pHeapStartAddress = static_cast<char*>(pHeapStartAddress) + sizeAlloc;

			assert(pHeapStartAddress <= pHeapEndAddress);
		}

		if (pBlock)
		{
			pBlock->pNextBlock = pOutstandingAllocations;
			pOutstandingAllocations = pBlock;
		}

		return pBlock ? pBlock->pBaseAddress : nullptr;
	}

	bool HeapManager::free(const void* pPtr)
	{
		assert(pOutstandingAllocations != nullptr);

		MemoryBlock* pAllocatedBlock = pOutstandingAllocations;
		MemoryBlock* pPrevBlock = nullptr;
		while (pAllocatedBlock)
		{
			if (pAllocatedBlock->pBaseAddress == pPtr)
			{
				if (pPrevBlock)
					pPrevBlock->pNextBlock = pAllocatedBlock->pNextBlock;
				else
					pOutstandingAllocations = pAllocatedBlock->pNextBlock;

				ReturnMemoryBlock(pAllocatedBlock);
				return true;
			}
			else
			{
				pPrevBlock = pAllocatedBlock;
				pAllocatedBlock = pAllocatedBlock->pNextBlock;
			}
		}

		return false;
	}

	// garbage collect, merge empty block
	void HeapManager::Collect()
	{
		MemoryBlock* pBlockStartAddr = reinterpret_cast<MemoryBlock*>(pHeapEndAddress);

	}

	bool HeapManager::Contains(const void* pPtr)
	{
		return (pPtr < pHeapEndAddress);
	}

	bool HeapManager::IsAllocated(const void* pPtr)
	{
		assert(pOutstandingAllocations != nullptr);

		MemoryBlock* pAllocatedBlock = pOutstandingAllocations;
		while (pAllocatedBlock)
		{
			if (pAllocatedBlock->pBaseAddress == pPtr)
				return true;

			pAllocatedBlock = pAllocatedBlock->pNextBlock;
		}

		return false;
	}

	void HeapManager::ShowFreeBlocks()
	{
		MemoryBlock* pCurBlock = pFreeList;
		while (pCurBlock)
		{
			if (pCurBlock->BlockSize > 0)
			{
				void* endPoint = static_cast<char*>(pCurBlock->pBaseAddress) + pCurBlock->BlockSize;
				printf("Free block start from %p to %p, size %u.\n", pCurBlock->pBaseAddress, endPoint, pCurBlock->BlockSize);
			}

			pCurBlock = pCurBlock->pNextBlock;
		}

		if (pHeapEndAddress > pHeapStartAddress)
			printf("Free block start from %p to %p, size %u.\n", pHeapStartAddress, 
				pHeapEndAddress, static_cast<char*>(pHeapEndAddress) - static_cast<char*>(pHeapStartAddress));
	}

	void HeapManager::ShowOutstandingAllocations()
	{
		MemoryBlock* pCurBlock = pOutstandingAllocations;
		while (pCurBlock)
		{
			if (pCurBlock->BlockSize > 0)
			{
				void* endPoint = static_cast<char*>(pCurBlock->pBaseAddress) + pCurBlock->BlockSize;
				printf("Allocated block start from %p to %p, size %u.\n", pCurBlock->pBaseAddress, endPoint, pCurBlock->BlockSize);
			}

			pCurBlock = pCurBlock->pNextBlock;
		}
	}

	void HeapManager::Destroy()
	{

	}

	MemoryBlock* HeapManager::FindFirstFittingFreeBlock(size_t i_size)
	{
		MemoryBlock* pCurBlock = pFreeList;
		MemoryBlock* pPrevBlock = nullptr;
		while (pCurBlock)
		{
			if (pCurBlock->BlockSize >= i_size)
			{
				break;
			}
			else
			{
				pPrevBlock = pCurBlock;
				pCurBlock = pCurBlock->pNextBlock;
			}
		}

		if (pCurBlock)
		{
			MemoryBlock* pNewBlock = pCurBlock->BlockSize > i_size ? GetFreeMemoryBlock() : nullptr;
			if (pNewBlock)
			{
				pNewBlock->pBaseAddress = static_cast<char*>(pCurBlock->pBaseAddress) + i_size;
				pNewBlock->BlockSize = pCurBlock->BlockSize - i_size;
				pNewBlock->pNextBlock = pCurBlock->pNextBlock;

				pCurBlock->BlockSize = i_size;
			}

			if (pPrevBlock)
				pPrevBlock->pNextBlock = pNewBlock ? pNewBlock : pCurBlock->pNextBlock;
			else
				pFreeList = pNewBlock;
		}
		return pCurBlock;
	}

	MemoryBlock* HeapManager::FindBestFittingFreeBlock(size_t i_size)
	{
		return nullptr;
	}

	MemoryBlock* HeapManager::GetFreeMemoryBlock()
	{
		MemoryBlock* pFreeBlock = pFreeList;
		while (pFreeBlock)
		{
			if(pFreeBlock->BlockSize == 0)
				return pFreeBlock;

			pFreeBlock = pFreeBlock->pNextBlock;
		}

		if (pFreeBlock == nullptr && static_cast<char*>(pHeapStartAddress) + sizeof(MemoryBlock) < pHeapEndAddress)
		{
			pHeapEndAddress = static_cast<char*>(pHeapEndAddress) - sizeof(MemoryBlock);
			pFreeBlock = reinterpret_cast<MemoryBlock*>(pHeapEndAddress);

			pFreeBlock->BlockSize = 0;
			pFreeBlock->pNextBlock = nullptr;
			pFreeBlock->pBaseAddress = nullptr;
		}

		return pFreeBlock;
	}

	void HeapManager::ReturnMemoryBlock(MemoryBlock* i_pFreeBlock)
	{
		assert(i_pFreeBlock != nullptr);

		if (pFreeList == nullptr)
		{
			i_pFreeBlock->pNextBlock = pFreeList;
			pFreeList = i_pFreeBlock;
			return;
		}

		MemoryBlock* pCurFreeBlock = pFreeList;
		MemoryBlock* pPrevBlock = nullptr;
		while (pCurFreeBlock)
		{
			// skip all free block
			if(pCurFreeBlock->BlockSize == 0)
				continue;

			void* pFreeBlockEndAddress = static_cast<char*>(i_pFreeBlock->pBaseAddress) + i_pFreeBlock->BlockSize;
			void* pPrevBlockEndAddress = pPrevBlock? static_cast<char*>(pPrevBlock->pBaseAddress) + pPrevBlock->BlockSize : nullptr;
			if (pFreeBlockEndAddress <= pCurFreeBlock->pBaseAddress)
			{
				// insert in order
				// merge begind
				if (pFreeBlockEndAddress == pCurFreeBlock->pBaseAddress)
				{
					pCurFreeBlock->pBaseAddress = i_pFreeBlock->pBaseAddress;
					pCurFreeBlock->BlockSize += i_pFreeBlock->BlockSize;

					// should also connect to the link
					i_pFreeBlock->BlockSize = 0;
					i_pFreeBlock->pNextBlock = pCurFreeBlock->pNextBlock;
					pCurFreeBlock->pNextBlock = i_pFreeBlock;
				}

				// merge front
				if (pPrevBlockEndAddress && pPrevBlockEndAddress == pCurFreeBlock->pBaseAddress)
				{
					pPrevBlock->BlockSize += pCurFreeBlock->BlockSize;

					// do not break the link
					pCurFreeBlock->BlockSize = 0;
				}

				return;
			}
			else
			{
				pPrevBlock = pCurFreeBlock;
				pCurFreeBlock = pCurFreeBlock->pNextBlock;
			}
		}

		assert(pPrevBlock != nullptr);
		pPrevBlock->pNextBlock = i_pFreeBlock;
		i_pFreeBlock->pNextBlock = nullptr;
	}

	size_t HeapManager::GetLargestFreeBlock()
	{
		size_t iMaxCapacity = static_cast<char*>(pHeapEndAddress) - static_cast<char*>(pHeapStartAddress);
		MemoryBlock* pCurBlock = pFreeList;
		bool bHasFreeBlock = false;
		while (pCurBlock)
		{
			iMaxCapacity = pCurBlock->BlockSize > iMaxCapacity ? pCurBlock->BlockSize : iMaxCapacity;
			bHasFreeBlock = pCurBlock->BlockSize == 0 ? true : bHasFreeBlock;

			pCurBlock = pCurBlock->pNextBlock;
		}

		if (iMaxCapacity == static_cast<char*>(pHeapEndAddress) - static_cast<char*>(pHeapStartAddress) && bHasFreeBlock)
			return iMaxCapacity;
		else
			return bHasFreeBlock ? iMaxCapacity : iMaxCapacity - sizeof(MemoryBlock);
	}
}
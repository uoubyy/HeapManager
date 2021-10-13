#include "HeapManagerProxy.h"
#include <stdio.h>

namespace HeapManagerProxy
{
	size_t HeapManager::s_MinumumToLeave = sizeof(MemoryBlock);

	HeapManager* CreateHeapManager(void* pHeapMemory, const size_t sizeHeap, const unsigned int numDescriptors)
	{
		assert((pHeapMemory != nullptr) && (sizeHeap > 0));

		HeapManager* pManager = reinterpret_cast<HeapManager*>(pHeapMemory);

		void* pHeapAllocatorMemory = pManager + 1;

		return new (pHeapMemory) HeapManager(pHeapAllocatorMemory, sizeHeap - sizeof(HeapManager));
	}

	HeapManager::HeapManager(void* pHeapAllocatorMemory, const size_t sizeHeap)
	{
		pFreeList = nullptr;
		pOutstandingAllocations = nullptr;

		pHeapStartAddress = pHeapAllocatorMemory;
		pHeapEndAddress = static_cast<char*>(pHeapStartAddress) + sizeHeap;
	}

	void* HeapManager::alloc(const size_t sizeAlloc, const unsigned int alignment)
	{
		MemoryBlock* pBlockNode = FindFirstFittingFreeBlock(sizeAlloc, alignment);

		void* add1 = pHeapStartAddress;
		void* add2 = pHeapEndAddress;

		if (pBlockNode == nullptr)
		{
			size_t maxCapacity = static_cast<char*>(AlignDownAddress(pHeapEndAddress, alignment)) - static_cast<char*>(AlignUpAddress(pHeapStartAddress, alignment)) - s_MinumumToLeave;
			if (maxCapacity < sizeAlloc) // left memory not enough
				return nullptr;

			pBlockNode = pBlockNode ? pBlockNode : GetFreeMemoryBlock();
			pBlockNode = pBlockNode ? pBlockNode : CreateFreeMemoryBlock();

			if (pBlockNode == nullptr) // out of all memory
				return nullptr;

			void* pBlockStartAddress = AlignDownAddress(static_cast<char*>(pHeapEndAddress) - sizeAlloc, alignment);

			pBlockNode->pBaseAddress = pBlockStartAddress;
			pBlockNode->BlockSize = static_cast<char*>(pHeapEndAddress) - static_cast<char*>(pBlockStartAddress);

			pHeapEndAddress = pBlockStartAddress;
		}

		assert(pBlockNode && pHeapStartAddress <= pHeapEndAddress);

		pBlockNode->pNextBlock = pOutstandingAllocations;
		pOutstandingAllocations = pBlockNode;

		return pBlockNode->pBaseAddress;
	}

	bool HeapManager::free(const void* pPtr)
	{
		MemoryBlock* pCurBlock = pOutstandingAllocations;
		MemoryBlock* pPrevBlock = nullptr;
		while (pCurBlock)
		{
			if (pCurBlock->pBaseAddress == pPtr)
			{
				if (pPrevBlock)
					pPrevBlock->pNextBlock = pCurBlock->pNextBlock;
				else
					pOutstandingAllocations = pCurBlock->pNextBlock;
				pCurBlock->pNextBlock = nullptr;

				ReturnMemoryBlock(pCurBlock);
				break;
			}

			pPrevBlock = pCurBlock;
			pCurBlock = pCurBlock->pNextBlock;
		}

		return (pCurBlock != nullptr);
	}

	void HeapManager::Collect()
	{
		MemoryBlock* pCurBlock = pFreeList;
		MemoryBlock* pPrevBlock = nullptr;
		MemoryBlock* pEmptyBlocks = nullptr;

		// remove all empty block first
		while (pCurBlock)
		{
			if (pCurBlock->BlockSize == 0)
			{
				if (pEmptyBlocks)
					pEmptyBlocks->pNextBlock = pCurBlock;
				else
					pEmptyBlocks = pCurBlock;

				if (pPrevBlock)
					pPrevBlock->pNextBlock = pCurBlock->pNextBlock;
				else
					pFreeList = pCurBlock->pNextBlock;

				MemoryBlock* pNext = pCurBlock->pNextBlock;
				pCurBlock->pNextBlock = nullptr;
				pCurBlock = pNext;
			}
			else
			{
				pPrevBlock = pCurBlock;
				pCurBlock = pCurBlock->pNextBlock;
			}
		}

		pCurBlock = pFreeList;
		pPrevBlock = nullptr;
		while (pCurBlock)
		{
			void* pPrevBlockEndAddress = pPrevBlock ? static_cast<char*>(pPrevBlock->pBaseAddress) + pPrevBlock->BlockSize : nullptr;
			if (pPrevBlock && pPrevBlockEndAddress == pCurBlock->pBaseAddress)
			{
				// start merge
				pPrevBlock->BlockSize += pCurBlock->BlockSize;
				pCurBlock->BlockSize = 0;
				// move to empty blocks
				if (pEmptyBlocks)
					pEmptyBlocks->pNextBlock = pCurBlock;
				else
					pEmptyBlocks = pCurBlock;

				MemoryBlock* pNext = pCurBlock->pNextBlock;
				pCurBlock->pNextBlock = nullptr;
				pCurBlock = pNext;

				pPrevBlock->pNextBlock = pCurBlock;
			}
			else
			{
				pPrevBlock = pCurBlock;
				pCurBlock = pCurBlock->pNextBlock;
			}
		}

		if (pPrevBlock)
			pPrevBlock->pNextBlock = pEmptyBlocks;

	}

	bool HeapManager::Contains(const void* pPtr)
	{
		// TODO
		return true;
	}

	bool HeapManager::IsAllocated(const void* pPtr)
	{
		MemoryBlock* pBlock = pOutstandingAllocations;
		while (pBlock)
		{
			if (pBlock->BlockSize > 0 && pBlock->pBaseAddress == pPtr)
				break;

			pBlock = pBlock->pNextBlock;
		}

		return (pBlock != nullptr);
	}

	void HeapManager::ShowFreeBlocks()
	{
		printf("Free Blocks:\n");
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
		printf("Allocated Blocks:\n");
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

	MemoryBlock* HeapManager::FindFirstFittingFreeBlock(const size_t i_size, const unsigned int alignment)
	{
		MemoryBlock* pCurBlock = pFreeList;
		MemoryBlock* pPrevBlock = nullptr;

		void* pCurBlockEndAddress = nullptr;
		void* pBlockRequiredStartAddress = nullptr;

		while (pCurBlock)
		{
			if (pCurBlock->BlockSize > AlignUp(i_size, alignment))
			{
				pCurBlockEndAddress = static_cast<char*>(pCurBlock->pBaseAddress) + pCurBlock->BlockSize;
				pBlockRequiredStartAddress = AlignDownAddress(static_cast<char*>(pCurBlockEndAddress) - i_size, alignment);

				if (pBlockRequiredStartAddress >= pCurBlock->pBaseAddress)
					break;
			}

			pPrevBlock = pCurBlock;
			pCurBlock = pCurBlock->pNextBlock;
		}

		if (pCurBlock)
		{
			if (pBlockRequiredStartAddress == pCurBlock->pBaseAddress)
			{
				if (pPrevBlock)
					pPrevBlock->pNextBlock = pCurBlock->pNextBlock;
				else
					pFreeList = pCurBlock->pNextBlock;

				// break connect
				pCurBlock->pNextBlock = nullptr;
			}
			else
			{
				MemoryBlock* pNewBlock = GetFreeMemoryBlock(false); // do not break connect
				pNewBlock = pNewBlock ? pNewBlock : CreateFreeMemoryBlock();

				assert(pNewBlock);
				pNewBlock->pBaseAddress = pCurBlock->pBaseAddress;
				pNewBlock->BlockSize = static_cast<char*>(pBlockRequiredStartAddress) - static_cast<char*>(pCurBlock->pBaseAddress);

				// pNewBlock can be pCurBlock's preview or behind node
				if (pNewBlock == pCurBlock->pNextBlock)
				{
					if (pPrevBlock)
						pPrevBlock->pNextBlock = pNewBlock;
					else
						pFreeList = pNewBlock;
				}
				else if(pNewBlock == pPrevBlock)
				{
					pNewBlock->pNextBlock = pCurBlock->pNextBlock;
				}
				else
				{
					MemoryBlock* pNewBlockPrevBlock = FindPrevFreeBlock(pNewBlock);
					// break old connect first
					if (pNewBlockPrevBlock)
						pNewBlockPrevBlock->pNextBlock = pNewBlock->pNextBlock;

					// connect
					if (pPrevBlock)
						pPrevBlock->pNextBlock = pNewBlock;
					else
						pFreeList = pNewBlock;
					pNewBlock->pNextBlock = pCurBlock->pNextBlock;
				}

				// break connect
				pCurBlock->pBaseAddress = pBlockRequiredStartAddress;
				pCurBlock->BlockSize = pCurBlock->BlockSize - pNewBlock->BlockSize;
				pCurBlock->pNextBlock = nullptr;
			}
		}

		return pCurBlock;
	}

	MemoryBlock* HeapManager::FindBestFittingFreeBlock(const size_t i_size, const unsigned int alignment)
	{
		// TODO
		return nullptr;
	}

	MemoryBlock* HeapManager::FindPrevFreeBlock(MemoryBlock* i_pBlock)
	{
		MemoryBlock* pCurBlock = pFreeList;
		MemoryBlock* pPrevBlock = nullptr;

		while (pCurBlock)
		{
			if(pCurBlock == i_pBlock)
				break;

			pPrevBlock = pCurBlock;
			pCurBlock = pCurBlock->pNextBlock;
		}

		return pPrevBlock;
	}

	MemoryBlock* HeapManager::GetFreeMemoryBlock(bool bBreakConnect)
	{
		MemoryBlock* pCurBlock = pFreeList;
		MemoryBlock* pPrevBlock = nullptr;

		while (pCurBlock)
		{
			if (pCurBlock->BlockSize == 0)
			{
				// if break connect automatic
				if (bBreakConnect)
				{
					if (pPrevBlock)
						pPrevBlock->pNextBlock = pCurBlock->pNextBlock;
					else
						pFreeList = pCurBlock->pNextBlock;
				}

				pCurBlock->pBaseAddress = nullptr;

				if(bBreakConnect)
					pCurBlock->pNextBlock = nullptr;

				break;
			}

			pPrevBlock = pCurBlock;
			pCurBlock = pCurBlock->pNextBlock;
		}

		return pCurBlock;
	}

	MemoryBlock* HeapManager::CreateFreeMemoryBlock()
	{
		if (static_cast<char*>(pHeapStartAddress) + sizeof(MemoryBlock) > pHeapEndAddress)
			return nullptr; // out of all memory

		MemoryBlock* pNewBlock = reinterpret_cast<MemoryBlock*>(pHeapStartAddress);
		pNewBlock->BlockSize = 0;
		pNewBlock->pBaseAddress = nullptr;
		pNewBlock->pNextBlock = nullptr;

		pHeapStartAddress = pNewBlock + 1;

		return pNewBlock;
	}

	void HeapManager::ReturnMemoryBlock(MemoryBlock* i_pFreeBlock)
	{
		if (pFreeList == nullptr)
		{
			i_pFreeBlock->pNextBlock = pFreeList;
			pFreeList = i_pFreeBlock;

			if (pHeapStartAddress == pHeapEndAddress)
			{
				pHeapEndAddress = static_cast<char*>(pHeapStartAddress) + i_pFreeBlock->BlockSize;
				pFreeList->BlockSize = 0;
				pFreeList->pBaseAddress = nullptr;
			}

			return;
		}

		// insert according pBaseAddress in order
		MemoryBlock* pCurBlock = pFreeList;
		MemoryBlock* pPrevBlock = nullptr;

		void* pFreeBlockEndAddress = static_cast<char*>(i_pFreeBlock->pBaseAddress) + i_pFreeBlock->BlockSize;

		while (pCurBlock)
		{
			if (pCurBlock->BlockSize > 0 && pCurBlock->pBaseAddress >= pFreeBlockEndAddress)
				break;

			pPrevBlock = pCurBlock;
			pCurBlock = pCurBlock->pNextBlock;
		}

		if (pCurBlock)
		{
			// insert in order
			i_pFreeBlock->pNextBlock = pCurBlock;
			if (pPrevBlock)
				pPrevBlock->pNextBlock = i_pFreeBlock;
			else
				pFreeList = i_pFreeBlock;
		}
		else
		{
			pPrevBlock->pNextBlock = i_pFreeBlock;
			i_pFreeBlock->pNextBlock = nullptr;
		}
	}

	size_t HeapManager::GetLargestFreeBlock(const unsigned int alignment)
	{
		size_t iMaxCapacity = static_cast<char*>(AlignDownAddress(pHeapEndAddress, alignment)) - static_cast<char*>(AlignUpAddress(pHeapStartAddress, alignment));
		MemoryBlock* pCurBlock = pFreeList;

		while (pCurBlock)
		{

			if (pCurBlock->BlockSize > 0)
			{
				void* start = AlignUpAddress(pCurBlock->pBaseAddress, alignment);
				void* end = AlignDownAddress(static_cast<char*>(pCurBlock->pBaseAddress) + pCurBlock->BlockSize, alignment);
				size_t capacity = static_cast<char*>(end) - static_cast<char*>(start);
				iMaxCapacity = capacity > iMaxCapacity ? capacity : iMaxCapacity;
			}

			pCurBlock = pCurBlock->pNextBlock;
		}

		return iMaxCapacity;
	}
}
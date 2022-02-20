#include "FixedSizeAllocator.h"
#include "BitArray.h"
#include "Utils.h"
#include "string.h"
#include "stdio.h"

namespace HeapManagerProxy
{
	FixedSizeAllocator::FixedSizeAllocator(void* i_pAllocatorMemory, void* i_pAvailableBlocks, const size_t sizeBlock, const size_t numBlocks) 
		: m_pAllocatorMemory(i_pAllocatorMemory),
		m_initData(sizeBlock, numBlocks)
	{
		m_pAvailableBlocks = static_cast<BitArray*>(i_pAvailableBlocks);
		memset(m_pAllocatorMemory, _bDeadLandFill, sizeBlock * numBlocks); // initial free all
	}

	void* FixedSizeAllocator::alloc(const size_t sizeAlloc, const unsigned int alignment /*= 4*/)
	{
		size_t realAllocSize = GUARD_BAND_SIZE + sizeAlloc + GUARD_BAND_SIZE;

		if (realAllocSize > m_initData.sizeBlocks)
			return nullptr;

		char* pUserMemory = nullptr;
		size_t i_firstAvailable;

		if (m_pAvailableBlocks->GetFirstSetBit(i_firstAvailable))
		{

			char* pBlockStartAddr = static_cast<char*>(m_pAllocatorMemory) + (i_firstAvailable * m_initData.sizeBlocks); // fixed block start address
			char* pBlockEndAddr = pBlockStartAddr + m_initData.sizeBlocks; // fixed block end address

			// user memory start address after header guard and align up
			pUserMemory = static_cast<char*>(Utils::AlignUpAddress(pBlockStartAddr + GUARD_BAND_SIZE, alignment));

			if (pUserMemory + sizeAlloc + GUARD_BAND_SIZE > pBlockEndAddr)
				return nullptr;

			m_pAvailableBlocks->ClearBit(i_firstAvailable);

			memset(pBlockStartAddr, _bAlignLandFill, pUserMemory - pBlockStartAddr);		// align
			memset(pUserMemory - GUARD_BAND_SIZE, _bNoMansLandFill, GUARD_BAND_SIZE);		// header guard
			memset(pUserMemory, _bCleanLandFill, sizeAlloc);								// user memory
			memset(pUserMemory + sizeAlloc, _bNoMansLandFill, GUARD_BAND_SIZE);				// tail guard
			printf("allocated memory %p from %zuKB fixed-size heap bit id %zu\n", pBlockStartAddr, m_initData.sizeBlocks, i_firstAvailable);
		}

		return pUserMemory;
	}

	bool FixedSizeAllocator::free(const void* pPtr)
	{
		if (!Contains(pPtr))
			return false;

		size_t offset = static_cast<char*>(const_cast<void*>(pPtr)) - static_cast<char*>(m_pAllocatorMemory);
		size_t i_bitNumber = offset / m_initData.sizeBlocks;

		char* m_pBlockStartAddr = static_cast<char*>(m_pAllocatorMemory) + (i_bitNumber * m_initData.sizeBlocks);
		memset(m_pBlockStartAddr, _bDeadLandFill, m_initData.sizeBlocks);	// free memory

		m_pAvailableBlocks->SetBit(i_bitNumber);
		return true;
	}

	bool FixedSizeAllocator::Contains(const void* pPtr)
	{
		char* m_pMemoryEnd = static_cast<char*>(m_pAllocatorMemory) + m_initData.numBlocks * m_initData.sizeBlocks;

		char* pAddr = static_cast<char*>(const_cast<void*>(pPtr));

		return (pAddr >= m_pAllocatorMemory && pAddr < m_pMemoryEnd);
	}

	bool FixedSizeAllocator::IsAllocated(const void* pPtr)
	{
		return Contains(pPtr);
	}

	void FixedSizeAllocator::ShowFreeBlocks()
	{
		return;
		printf("Free Blocks in %zuKB heap:\n", m_initData.sizeBlocks);
		printf("Start\tEnd\tSize\tStatus\n");
		for (size_t iBit = 0; iBit < m_pAvailableBlocks->GetBitsNum(); ++iBit)
		{
			if ((*m_pAvailableBlocks)[iBit])
			{
				char* pBlockStart = static_cast<char*>(m_pAllocatorMemory) + (iBit * m_initData.sizeBlocks);
				printf("%p\t%p\t%zu\tFree\n", pBlockStart, pBlockStart + m_initData.sizeBlocks, m_initData.sizeBlocks);
			}
		}
	}

	void FixedSizeAllocator::ShowOutstandingAllocations()
	{
		printf("Allocated Blocks in %zuKB heap:\n", m_initData.sizeBlocks);
		printf("Start\tEnd\tSize\tStatus\n");
		for (size_t iBit = 0; iBit < m_pAvailableBlocks->GetBitsNum(); ++iBit)
		{
			if ((*m_pAvailableBlocks)[iBit] == false)
			{
				char* pBlockStart = static_cast<char*>(m_pAllocatorMemory) + (iBit * m_initData.sizeBlocks);
				printf("%p\t%p\tAllocated\n", pBlockStart, pBlockStart + m_initData.sizeBlocks);
			}
		}
	}

	void FixedSizeAllocator::Destroy()
	{
		m_pAvailableBlocks->Destroy();
	}

	bool FixedSizeAllocator::IsEmpty()
	{
		return m_pAvailableBlocks->AreAllBitsSet();
	}

	FixedSizeAllocator::~FixedSizeAllocator()
	{
	}
}

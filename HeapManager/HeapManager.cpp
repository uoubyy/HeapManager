#include "HeapManager.h"
#include "BitArray.h"
#include <assert.h>
#include <Windows.h>

namespace HeapManagerProxy
{

	HeapManager::HeapManager() : pDefaultHeap(nullptr)
	{

	}

	HeapManager::~HeapManager()
	{

	}

	void HeapManager::CreateHeaps()
	{
		std::vector<FSAInitData> FSASizes;
		FSASizes.push_back(FSAInitData(64, 1024 * 1024 / 64));
		FSASizes.push_back(FSAInitData(128, 1024 * 1024 / 128));
		FSASizes.push_back(FSAInitData(256, 1024 * 1024 / 256));
		// alloc 4MB memory, 1MB for defaultHeap, 1MB for 64KB fixed-size heap
		// 1MB for 128KB fixed-size heap, 1MB for 256 KB fixed-size heap
		const size_t sizeHeap = 4 * 1024 * 1024;

#ifdef USE_HEAP_ALLOC
		void* pHeapMemory = HeapAlloc(GetProcessHeap(), 0, sizeHeap);
#else
		// Get SYSTEM_INFO, which includes the memory page size
		SYSTEM_INFO SysInfo;
		GetSystemInfo(&SysInfo);
		// round our size to a multiple of memory page size
		assert(SysInfo.dwPageSize > 0);
		size_t sizeHeapInPageMultiples = SysInfo.dwPageSize * ((sizeHeap + SysInfo.dwPageSize) / SysInfo.dwPageSize);

		assert(sizeHeapInPageMultiples > sizeof(HeapAllocator));
		void* pHeapMemory = VirtualAlloc(NULL, sizeHeap, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
#endif

		assert((pHeapMemory != nullptr));

		void* pAllocatorMemory = reinterpret_cast<HeapAllocator*>(pHeapMemory) + 1;
		pDefaultHeap = new (pHeapMemory) HeapAllocator(pAllocatorMemory, 1024 * 1024 - sizeof(HeapAllocator));

		for (size_t i = 0; i < FSASizes.size(); ++i)
		{
			void* pAllocatorMemory = static_cast<char*>(pHeapMemory) + (i + 1) * 1024 * 1024;

			// alloc BitArray and FixedSizeAllocator pointer from default heap
			void* pFixedSizeHeap = pDefaultHeap->alloc(sizeof(FixedSizeAllocator));
			BitArray* pAvailableBlocks = BitArray::Create(FSASizes[i].numBlocks, pDefaultHeap);
			
			FixedSizeAllocator* fixedSizeAllocator = new (pFixedSizeHeap) FixedSizeAllocator(pAllocatorMemory, pAvailableBlocks, FSASizes[i].sizeBlocks, FSASizes[i].numBlocks);

			FSAs.push_back(fixedSizeAllocator);
			BitArrays.push_back(pAvailableBlocks);
		}

		printf("Default Heap start from %p to %p\n", pHeapMemory, static_cast<char*>(pHeapMemory) + 1 * 1024 * 1024);
		printf("Fixed-size Heap in  64KB start from %p to %p\n", static_cast<char*>(pHeapMemory) + 1 * 1024 * 1024, static_cast<char*>(pHeapMemory) + 2 * 1024 * 1024);
		printf("Fixed-size Heap in 128KB start from %p to %p\n", static_cast<char*>(pHeapMemory) + 2 * 1024 * 1024, static_cast<char*>(pHeapMemory) + 3 * 1024 * 1024);
		printf("Fixed-size Heap in 256KB start from %p to %p\n", static_cast<char*>(pHeapMemory) + 3 * 1024 * 1024, static_cast<char*>(pHeapMemory) + 4 * 1024 * 1024);
	}

	void* HeapManager::malloc(size_t i_size)
	{
		void* pUserMemory = nullptr;
		for (size_t i = 0; i < FSAs.size(); ++i)
		{
			if (i_size <= FSAs[i]->GetNumBlocks())
			{
				pUserMemory = FSAs[i]->alloc(i_size);
				break;
			}
		}

		if (pUserMemory == nullptr)
		{
			pUserMemory = pDefaultHeap->alloc(i_size);
		}

		return pUserMemory;
	}

	bool HeapManager::free(void* i_ptr)
	{
		for (size_t i = 0; i < FSAs.size(); ++i)
		{
			if (FSAs[i]->Contains(i_ptr))
				return FSAs[i]->free(i_ptr);
		}

		return pDefaultHeap->free(i_ptr);
	}

	void HeapManager::Destroy()
	{
		while (!FSAs.empty())
		{
			FixedSizeAllocator* fixedSizeHeap = FSAs.back();
			FSAs.pop_back();
			fixedSizeHeap->Destroy();

			BitArray* pBitArray = BitArrays.back();
			BitArrays.pop_back();

			pBitArray->~BitArray();
			pDefaultHeap->free(pBitArray);

			fixedSizeHeap->~FixedSizeAllocator();
			pDefaultHeap->free(fixedSizeHeap);
		}
		
		if (pDefaultHeap)
		{
			if (pDefaultHeap->IsEmpty() == false)
			{
				fprintf(stderr, "HeapManager still has outstanding blocks!");
				return;
			}

			pDefaultHeap->~HeapAllocator();
#ifdef USE_HEAP_ALLOC
			HeapFree(GetProcessHeap(), 0, pDefaultHeap);
#else
			VirtualFree(pDefaultHeap, 0, MEM_RELEASE);
#endif
		}

	}

	void HeapManager::Collect()
	{
		assert(pDefaultHeap);

		pDefaultHeap->Collect();

		// no need to collect fixed size allocator
	}

	void HeapManager::ShowFreeBlocks()
	{
		assert(pDefaultHeap);

		pDefaultHeap->ShowFreeBlocks();
		for (size_t i = 0; i < FSAs.size(); ++i)
		{
			FSAs[i]->ShowFreeBlocks();
		}
	}

	void HeapManager::ShowOutstandingAllocations()
	{
		assert(pDefaultHeap);

		pDefaultHeap->ShowOutstandingAllocations();
		for (size_t i = 0; i < FSAs.size(); ++i)
		{
			FSAs[i]->ShowOutstandingAllocations();
		}
	}
}
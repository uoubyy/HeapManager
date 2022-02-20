#pragma once
#include <Windows.h>

#include <assert.h>
#include <algorithm>
#include <vector>

#include "HeapManager.h"
#include "HeapAllocator.h"
#include "FixedSizeAllocator.h"

#define SUPPORTS_ALIGNMENT
#define SUPPORTS_SHOWFREEBLOCKS
#define SUPPORTS_SHOWOUTSTANDINGALLOCATIONS

#define TEST_SINGLE_LARGE_ALLOCATION

bool HeapManager_UnitTest()
{
	using namespace HeapManagerProxy;

	const size_t 		sizeHeap = 1024 * 1024;
	const unsigned int 	numDescriptors = 2048;

	// Create a heap manager for my test heap.
	HeapManager* pHeapManager = new HeapManager();
	pHeapManager->CreateHeaps();

	HeapAllocator* pHeapAllocator = pHeapManager->GetDefaultHeap();
	assert(pHeapAllocator);

	if (pHeapAllocator == nullptr)
		return false;

#ifdef TEST_SINGLE_LARGE_ALLOCATION
	// This is a test I wrote to check to see if using the whole block if it was almost consumed by 
	// an allocation worked. Also helped test my ShowFreeBlocks() and ShowOutstandingAllocations().
	{
#ifdef SUPPORTS_SHOWFREEBLOCKS
		pHeapAllocator->ShowFreeBlocks();
#endif // SUPPORTS_SHOWFREEBLOCKS

		size_t largestBeforeAlloc = pHeapAllocator->GetLargestFreeBlock();
		void* pPtr = pHeapAllocator->alloc(largestBeforeAlloc - HeapAllocator::s_MinumumToLeave);

		if (pPtr)
		{
#if defined(SUPPORTS_SHOWFREEBLOCKS) || defined(SUPPORTS_SHOWOUTSTANDINGALLOCATIONS)
			printf("After large allocation:\n");
#ifdef SUPPORTS_SHOWFREEBLOCKS
			pHeapAllocator->ShowFreeBlocks();
#endif // SUPPORTS_SHOWFREEBLOCKS
#ifdef SUPPORTS_SHOWOUTSTANDINGALLOCATIONS
			pHeapAllocator->ShowOutstandingAllocations();
#endif // SUPPORTS_SHOWOUTSTANDINGALLOCATIONS
			printf("\n");
#endif

			size_t largestAfterAlloc = pHeapAllocator->GetLargestFreeBlock();
			bool success = pHeapAllocator->Contains(pPtr) && pHeapAllocator->IsAllocated(pPtr);
			assert(success);

			success = pHeapAllocator->free(pPtr);
			assert(success);

			pHeapAllocator->Collect();

#if defined(SUPPORTS_SHOWFREEBLOCKS) || defined(SUPPORTS_SHOWOUTSTANDINGALLOCATIONS)
			printf("After freeing allocation and garbage collection:\n");
#ifdef SUPPORTS_SHOWFREEBLOCKS
			pHeapAllocator->ShowFreeBlocks();
#endif // SUPPORTS_SHOWFREEBLOCKS
#ifdef SUPPORTS_SHOWOUTSTANDINGALLOCATIONS
			pHeapAllocator->ShowOutstandingAllocations();
#endif // SUPPORTS_SHOWOUTSTANDINGALLOCATIONS
			printf("\n");
#endif

			size_t largestAfterCollect = pHeapAllocator->GetLargestFreeBlock();
		}
	}
#endif

	std::vector<void*> AllocatedAddresses;

	long	numAllocs = 0;
	long	numFrees = 0;
	long	numCollects = 0;

	// allocate memory of random sizes up to 1024 bytes from the heap manager
	// until it runs out of memory
	do
	{
		const size_t		maxTestAllocationSize = 1024;

		size_t			sizeAlloc = 1 + (rand() & (maxTestAllocationSize - 1));

#ifdef SUPPORTS_ALIGNMENT
		// pick an alignment
		const unsigned int	alignments[] = { 4, 8, 16, 32, 64 };

		const unsigned int	index = rand() % (sizeof(alignments) / sizeof(alignments[0]));

		const unsigned int	alignment = alignments[index];

		void* pPtr = pHeapAllocator->alloc(sizeAlloc, alignment);

		// check that the returned address has the requested alignment
		assert((reinterpret_cast<uintptr_t>(pPtr) & (alignment - 1)) == 0);
#else
		void* pPtr = pHeapAllocator->alloc(sizeAlloc);
#endif // SUPPORT_ALIGNMENT

		// if allocation failed see if garbage collecting will create a large enough block
		if (pPtr == nullptr)
		{
			pHeapAllocator->Collect();

#ifdef SUPPORTS_ALIGNMENT
			pPtr = pHeapAllocator->alloc(sizeAlloc, alignment);
#else
			pPtr = pHeapAllocator->alloc(sizeAlloc);
#endif // SUPPORT_ALIGNMENT

			// if not we're done. go on to cleanup phase of test
			if (pPtr == nullptr)
				break;
		}

		AllocatedAddresses.push_back(pPtr);
		numAllocs++;

		// randomly free and/or garbage collect during allocation phase
		const unsigned int freeAboutEvery = 10;
		const unsigned int garbageCollectAboutEvery = 40;

		if (!AllocatedAddresses.empty() && ((rand() % freeAboutEvery) == 0))
		{
			void* pPtr = AllocatedAddresses.back();
			AllocatedAddresses.pop_back();

			bool success = pHeapAllocator->Contains(pPtr) && pHeapAllocator->IsAllocated(pPtr);
			assert(success);

			success = pHeapAllocator->free(pPtr);
			assert(success);

			numFrees++;
		}

		if ((rand() % garbageCollectAboutEvery) == 0)
		{
			pHeapAllocator->Collect();

			numCollects++;
		}

	} while (1);

#if defined(SUPPORTS_SHOWFREEBLOCKS) || defined(SUPPORTS_SHOWOUTSTANDINGALLOCATIONS)
	printf("After exhausting allocations:\n");
#ifdef SUPPORTS_SHOWFREEBLOCKS
	pHeapAllocator->ShowFreeBlocks();
#endif // SUPPORTS_SHOWFREEBLOCKS
#ifdef SUPPORTS_SHOWOUTSTANDINGALLOCATIONS
	pHeapAllocator->ShowOutstandingAllocations();
#endif // SUPPORTS_SHOWOUTSTANDINGALLOCATIONS
	printf("\n");
#endif

	// now free those blocks in a random order
	if (!AllocatedAddresses.empty())
	{
		// randomize the addresses
		std::random_shuffle(AllocatedAddresses.begin(), AllocatedAddresses.end());

		// return them back to the heap manager
		while (!AllocatedAddresses.empty())
		{
			void* pPtr = AllocatedAddresses.back();
			AllocatedAddresses.pop_back();

			bool success = pHeapAllocator->Contains(pPtr) && pHeapAllocator->IsAllocated(pPtr);
			assert(success);

			success = pHeapAllocator->free(pPtr);
			assert(success);
		}

#if defined(SUPPORTS_SHOWFREEBLOCKS) || defined(SUPPORTS_SHOWOUTSTANDINGALLOCATIONS)
		printf("After freeing allocations:\n");
#ifdef SUPPORTS_SHOWFREEBLOCKS
		pHeapAllocator->ShowFreeBlocks();
#endif // SUPPORTS_SHOWFREEBLOCKS

#ifdef SUPPORTS_SHOWOUTSTANDINGALLOCATIONS
		pHeapAllocator->ShowOutstandingAllocations();
#endif // SUPPORTS_SHOWOUTSTANDINGALLOCATIONS
		printf("\n");
#endif

		// do garbage collection
		pHeapAllocator->Collect();
		// our heap should be one single block, all the memory it started with

#if defined(SUPPORTS_SHOWFREEBLOCKS) || defined(SUPPORTS_SHOWOUTSTANDINGALLOCATIONS)
		printf("After garbage collection:\n");
#ifdef SUPPORTS_SHOWFREEBLOCKS
		pHeapAllocator->ShowFreeBlocks();
#endif // SUPPORTS_SHOWFREEBLOCKS

#ifdef SUPPORTS_SHOWOUTSTANDINGALLOCATIONS
		pHeapAllocator->ShowOutstandingAllocations();
#endif // SUPPORTS_SHOWOUTSTANDINGALLOCATIONS
		printf("\n");
#endif

		// do a large test allocation to see if garbage collection worked
		void* pPtr = pHeapAllocator->alloc(sizeHeap / 2);
		assert(pPtr);

		if (pPtr)
		{
			bool success = pHeapAllocator->Contains(pPtr) && pHeapAllocator->IsAllocated(pPtr);
			assert(success);

			success = pHeapAllocator->free(pPtr);
			assert(success);
		}

		//pHeapAllocator->ShowFreeBlocks();
	}

	pHeapManager->Destroy();

	delete pHeapManager;
	pHeapManager = nullptr;

	// we succeeded
	return true;
}
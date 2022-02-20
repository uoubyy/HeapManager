#pragma once
#include "HeapManager.h"
#include <algorithm>  

bool MemorySystem_UnitTest()
{
	using namespace HeapManagerProxy;

	const size_t maxAllocations = 10 * 1024;
	std::vector<void*> AllocatedAddresses;

	long	numAllocs = 0;
	long	numFrees = 0;
	long	numCollects = 0;

	size_t totalAllocated = 0;

	// reserve space in AllocatedAddresses for the maximum number of allocation attempts
	// prevents new returning null when std::vector expands the underlying array
	AllocatedAddresses.reserve(10 * 1024);

	HeapManager* pHeapManager = new HeapManager();
	pHeapManager->CreateHeaps();

	// allocate memory of random sizes up to 1024 bytes from the heap manager
	// until it runs out of memory
	do
	{
		const size_t		maxTestAllocationSize = 1024;

		size_t			sizeAlloc = 1 + (rand() & (maxTestAllocationSize - 1));

		void* pPtr = pHeapManager->malloc(sizeAlloc);

		// if allocation failed see if garbage collecting will create a large enough block
		if (pPtr == nullptr)
		{
			pHeapManager->Collect();

			pPtr = pHeapManager->malloc(sizeAlloc);

			// if not we're done. go on to cleanup phase of test
			if (pPtr == nullptr)
				break;
		}

		AllocatedAddresses.push_back(pPtr);
		numAllocs++;

		totalAllocated += sizeAlloc;

		// randomly free and/or garbage collect during allocation phase
		const unsigned int freeAboutEvery = 0x07;
		const unsigned int garbageCollectAboutEvery = 0x07;

		if (!AllocatedAddresses.empty() && ((rand() % freeAboutEvery) == 0))
		{
			void* pPtrToFree = AllocatedAddresses.back();
			AllocatedAddresses.pop_back();

			pHeapManager->free(pPtrToFree);
			numFrees++;
		}
		else if ((rand() % garbageCollectAboutEvery) == 0)
		{
			pHeapManager->Collect();

			numCollects++;
		}

	} while (numAllocs < maxAllocations);

	// now free those blocks in a random order
	if (!AllocatedAddresses.empty())
	{
		// randomize the addresses
		// std::random_shuffle(AllocatedAddresses.begin(), AllocatedAddresses.end());

		// return them back to the heap manager
		while (!AllocatedAddresses.empty())
		{
			void* pPtrToFree = AllocatedAddresses.back();
			AllocatedAddresses.pop_back();

			if (pHeapManager->free(pPtrToFree) == false)
				printf("failed to free memory %p\n", pPtrToFree);
			// delete[] pPtrToFree;
		}

		// do garbage collection
		pHeapManager->Collect();
		// our heap should be one single block, all the memory it started with

		// do a large test allocation to see if garbage collection worked
		pHeapManager->ShowFreeBlocks();
		void* pPtr = pHeapManager->malloc(totalAllocated / 2);

		if (pPtr)
		{
			pHeapManager->free(pPtr);
		}
		else
		{
			// something failed
			return false;
		}
	}
	else
	{
		return false;
	}
	pHeapManager->ShowOutstandingAllocations();
	// this new [] / delete [] pair should run through your allocator
	char* pNewTest = new char[1024];

	delete[] pNewTest;

	pHeapManager->Destroy();
	delete pHeapManager;

	// we succeeded
	return true;
}

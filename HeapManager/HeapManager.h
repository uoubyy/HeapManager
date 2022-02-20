#pragma once
#include <vector>
#include "HeapAllocator.h"
#include "FixedSizeAllocator.h"

namespace HeapManagerProxy
{
	class HeapManager
	{

	public:
		HeapManager();
		~HeapManager();

		void CreateHeaps();

		void* malloc(size_t i_size);

		bool free(void* i_ptr);

		HeapAllocator* GetDefaultHeap() const { return pDefaultHeap; }

		void Destroy();

		void Collect();

		void ShowFreeBlocks();

		void ShowOutstandingAllocations();

	private:
		std::vector<FixedSizeAllocator*> FSAs;
		std::vector<BitArray*> BitArrays;
		HeapAllocator* pDefaultHeap;
	};
}


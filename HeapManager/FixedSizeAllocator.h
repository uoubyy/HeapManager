#pragma once
#include "IAllocator.h"

namespace HeapManagerProxy
{
    class BitArray;

    struct FSAInitData
    {
        size_t sizeBlocks;
        size_t numBlocks;

        FSAInitData() :sizeBlocks(0), numBlocks(0) {}
        FSAInitData(size_t size, size_t num) :sizeBlocks(size), numBlocks(num) {}
    };

    class FixedSizeAllocator : public IAllocator
    {
    public:
        FixedSizeAllocator() = delete; // remove default constructor
        FixedSizeAllocator(void* i_pAllocatorMemory, void* i_pAvailableBlocks, const size_t sizeBlock, const size_t numBlocks);

        virtual ~FixedSizeAllocator();

		void* alloc(const size_t sizeAlloc, const unsigned int alignment = 4) override;

		bool free(const void* pPtr) override;

        void Collect() override {}; // no need collect

        bool Contains(const void* pPtr) override;

		bool IsAllocated(const void* pPtr) override;

		void ShowFreeBlocks() override;

		void ShowOutstandingAllocations() override;

        void Destroy() override;

        bool IsEmpty() override;

        inline const size_t GetNumBlocks() const { return m_initData.numBlocks; }

	protected:
		void* m_pAllocatorMemory;

        BitArray* m_pAvailableBlocks;
        FSAInitData m_initData;
    };
}


#pragma once

namespace HeapManagerProxy
{

#if _DEBUG
#define GUARD_BAND_SIZE 4
#else
#define GUARD_BAND_SIZE 0
#endif

	class IAllocator
	{
	public:
		// allocate a block of memory
		virtual void* alloc(const size_t sizeAlloc, const unsigned int alignment = 4) = 0;

		virtual bool free(const void* pPtr) = 0;

		// garbage collect, merge empty block
		virtual void Collect() = 0;

		virtual bool Contains(const void* pPtr) = 0;

		virtual bool IsAllocated(const void* pPtr) = 0;

		virtual void ShowFreeBlocks() = 0;

		virtual void ShowOutstandingAllocations() = 0;

		virtual void Destroy() = 0;

		virtual bool IsEmpty() = 0;
	};

	const unsigned char _bNoMansLandFill = 0xFD; // Guard Band
	const unsigned char _bAlignLandFill = 0xED; // Padding to get requested alignment
	const unsigned char _bDeadLandFill = 0xDD; // free()d memory
	const unsigned char _bCleanLandFill = 0xCD; // fill before returning from malloc()
}
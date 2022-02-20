#pragma once

#include <stdio.h>
#include <stdint.h>

namespace HeapManagerProxy
{
	class HeapAllocator;

	class BitArray
	{
#if WIN32
		typedef uint32_t t_BitData;
#else
		typedef uint64_t t_BitData;
#endif
		size_t m_numBits;
		t_BitData* m_pBits;

		static size_t bitsPerElement;

		HeapAllocator* m_pAllocator;
	public:
		static BitArray* Create(size_t i_numBits, HeapAllocator* i_pAllocator);

		BitArray(size_t i_numBits, t_BitData* i_pBits, HeapAllocator* i_pAllocator) : m_numBits(i_numBits), m_pBits(i_pBits), m_pAllocator(i_pAllocator) {}
		virtual ~BitArray();

		void ClearAll(void);
		void SetAll(void);

		bool AreAllBitsClear(void) const;
		bool AreAllBitsSet(void) const;

		inline bool IsBitSet(size_t i_bitNumber) const;
		inline bool IsBitClear(size_t i_bitNumber) const;

		void SetBit(size_t i_bitNumber);
		void ClearBit(size_t i_bitNumber);

		bool GetFirstClearBit(size_t& o_bitNumber) const;
		bool GetFirstSetBit(size_t& o_bitNumber) const;

		const size_t GetElementSize() const { return bitsPerElement; }
		const size_t GetBitsNum() const { return m_numBits; }

		bool operator[](size_t i_bitNumber) const;

		void Destroy();
	};
}


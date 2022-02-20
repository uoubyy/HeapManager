#include "BitArray.h"

#include <string.h>
#include <intrin.h>
#include <assert.h>
#include <new>

#include "HeapAllocator.h"

#if WIN32
#pragma intrinsic(_BitScanForward)
#else
#pragma intrinsic(_BitScanForward64)
#endif

namespace HeapManagerProxy
{
	size_t BitArray::bitsPerElement = sizeof(t_BitData) * 8;

	BitArray* BitArray::Create(size_t i_numBits, HeapAllocator* i_pAllocator)
	{
		t_BitData* pBits = reinterpret_cast<t_BitData*>(i_pAllocator->alloc((i_numBits + bitsPerElement) / 8));

		BitArray* m_pBitArray = new (i_pAllocator->alloc(sizeof(BitArray))) BitArray(i_numBits, pBits, i_pAllocator);

		assert(m_pBitArray && m_pBitArray->m_pBits);

		m_pBitArray->SetAll();

		return m_pBitArray;
	}

	BitArray::~BitArray()
	{
		assert(m_pBits == nullptr);
	}

	void BitArray::ClearAll(void)
	{
		assert(m_pBits);
		memset(m_pBits, 0, (m_numBits + bitsPerElement) / 8);
	}

	void BitArray::SetAll(void) // empty
	{
		assert(m_pBits);
		memset(m_pBits, 0xFF, (m_numBits + bitsPerElement) / 8);
	}

	bool BitArray::AreAllBitsClear(void) const
	{
		for (size_t iByte = 0; iByte <= m_numBits / bitsPerElement; ++iByte)
		{
			unsigned long index;
			unsigned char isNonzero = 0;

			//  If no set bit is found, 0 is returned; otherwise, 1 is returned.
#if WIN32
			isNonzero = _BitScanForward(&index, m_pBits[iByte]);
#else
			isNonzero = _BitScanForward64(&index, m_pBits[iByte]);
#endif
			if (isNonzero)
				return false;
		}

		return true;
	}

	bool BitArray::AreAllBitsSet(void) const
	{
		for (size_t iByte = 0; iByte <= m_numBits / bitsPerElement; ++iByte)
		{
			size_t zero = 0;
			if (~(m_pBits[iByte] & (~zero)))
				return false;
		}

		return true;
	}

	bool BitArray::IsBitSet(size_t i_bitNumber) const
	{
		size_t iByte = i_bitNumber / bitsPerElement;
		size_t iBit = i_bitNumber % bitsPerElement;

		return (m_pBits[iByte] & (size_t(1) << iBit));
	}

	bool BitArray::IsBitClear(size_t i_bitNumber) const
	{
		size_t iByte = i_bitNumber / bitsPerElement;
		size_t iBit = i_bitNumber % bitsPerElement;

		return (m_pBits[iByte] & (size_t(1) << iBit));
	}

	void BitArray::SetBit(size_t i_bitNumber)
	{
		assert(i_bitNumber < m_numBits);

		size_t iByte = i_bitNumber / bitsPerElement;
		size_t iBit = i_bitNumber % bitsPerElement;

		t_BitData setHelp = t_BitData(1) << iBit;
		m_pBits[iByte] = m_pBits[iByte] | setHelp;
	}

	void BitArray::ClearBit(size_t i_bitNumber)
	{
		assert(i_bitNumber < m_numBits);

		size_t iByte = i_bitNumber / bitsPerElement;
		size_t iBit = i_bitNumber % bitsPerElement;

		t_BitData setHelp = ~(t_BitData(1) << iBit);
		m_pBits[iByte] = m_pBits[iByte] & setHelp;
	}

	bool BitArray::GetFirstClearBit(size_t& o_bitNumber) const
	{
		for (size_t iByte = 0; iByte <= m_numBits / bitsPerElement; ++iByte)
		{
			for (size_t iBit = 0; iBit < bitsPerElement; ++iBit)
			{
				if (m_pBits[iByte] & (t_BitData(1) << iBit))
				{
					o_bitNumber = iByte * bitsPerElement + iBit;
					return o_bitNumber < m_numBits;
				}
			}
		}
		return false;
	}

	bool BitArray::GetFirstSetBit(size_t& o_bitNumber) const
	{
		for (size_t iByte = 0; iByte <= m_numBits / bitsPerElement; ++iByte)
		{
			if(m_pBits[iByte] == t_BitData(0))
				continue;

			unsigned long iBit;
#if WIN32
			_BitScanForward(&iBit, m_pBits[iByte]);
#else
			_BitScanForward64(&iBit, m_pBits[iByte]);
#endif
			o_bitNumber = iByte * bitsPerElement + iBit;
			return (iBit < m_numBits);
		}
		return false;
	}

	bool BitArray::operator[](size_t i_bitNumber) const
	{
		assert(i_bitNumber < m_numBits);

		size_t iByte = i_bitNumber / bitsPerElement;
		size_t iBit = i_bitNumber % bitsPerElement;

		t_BitData setHelp = t_BitData(1) << iBit;
		return (m_pBits[iByte] & setHelp);
	}

	void BitArray::Destroy()
	{
		m_pAllocator->free(m_pBits);
		m_pBits = nullptr;
	}

}
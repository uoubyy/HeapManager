#pragma once
#include <assert.h>

namespace HeapManagerProxy
{
	class Utils {
	public:
		static bool IsPowerOfTwo(size_t value)
		{
			return !(value == 0) && !(value & (value - 1));
		}

		static uintptr_t AlignDown(uintptr_t i_value, uintptr_t i_align)
		{
			assert(i_align);
			assert(IsPowerOfTwo(i_align));

			return i_value & ~(i_align - 1);
		}

		static uintptr_t AlignUp(uintptr_t i_value, uintptr_t i_align)
		{
			assert(i_align);
			assert(IsPowerOfTwo(i_align));

			return (i_value + (i_align - 1)) & ~(i_align - 1);
		}

		static void* AlignDownAddress(void* i_pAddr, unsigned int i_align = 4)
		{
			return reinterpret_cast<void*>(AlignDown(reinterpret_cast<uintptr_t>(i_pAddr), i_align));
		}

		static void* AlignUpAddress(void* i_pAddr, unsigned int i_align = 4)
		{
			return reinterpret_cast<void*>(AlignUp(reinterpret_cast<uintptr_t>(i_pAddr), i_align));
		}
	};
}
# HeapManager

This project is an implement of heap allocator. Its job is to manage allocation and deallocation of addresses for a given block of memory. 

**Features**

1. Work well in both x86 and x64.
2. Support address alignment.
3. Has multi Fixed Size Allocators and a Genral Allocator inside. Use the Fixed Size Allocator to cover most small size (64KB, 128KB and 256KB) allocation and use the General Allocator to cover other situations.
4. Dynamic garbage collection and compact structure. In the General Allocator, allocation start from the end of the internal heap, and use the top of the heap to place memory description blocks. Every allocation use first fit strategy, automatic collect and merge garbage after release.
5. Support using Guardbands to check data overflow.
6. Use BitArray to track the used situation of memory block in the Fixed Size Allocator. 
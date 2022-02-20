#include <Windows.h>

#include <assert.h>
#include <algorithm>
#include <vector>

#include "HeapManager_UnitTest.h"
#include "MemorySystem_UnitTest.h"

#ifdef _DEBUG
#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>
#endif // _DEBUG

int main()
{
	//HeapManager_UnitTest();
	MemorySystem_UnitTest();

#if defined(_DEBUG)
	_CrtDumpMemoryLeaks();
#endif // _DEBUG

	return 0;
}
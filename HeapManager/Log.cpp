#include "Log.h"
#include <stdio.h>

namespace HeapManagerProxy
{
	void Log_i(const char* message)
	{
		printf(message);
	}
}
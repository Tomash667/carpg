#include "Pch.h"
#include "ScriptExtensions.h"

#include <scriptarray\scriptarray.h>

int CScriptArray_GetElementSize(CScriptArray& arr)
{
	struct CScriptArrayHackerman : public CScriptArray
	{
	public:
		int GetElementSize() const { return elementSize; }
	};

	return static_cast<CScriptArrayHackerman&>(arr).GetElementSize();
}

void CScriptArray_Shuffle(CScriptArray& arr)
{
	if(arr.GetSize() == 0)
		return;

	assert(CScriptArray_GetElementSize(arr) == 4); // TODO
	int* start = static_cast<int*>(arr.GetBuffer());
	int* end = start + arr.GetSize();
	Shuffle(start, end);
}

#include "Pch.h"
#include "Encounter.h"

Encounter::~Encounter()
{
	if(pooledString)
		StringPool.Free(pooledString);
}

const string& Encounter::GetTextS()
{
	if(!pooledString)
	{
		pooledString = StringPool.Get();
		text = pooledString->c_str();
	}
	return *pooledString;
}

void Encounter::SetTextS(const string& str)
{
	if(!pooledString)
		pooledString = StringPool.Get();
	*pooledString = str;
	text = pooledString->c_str();
}

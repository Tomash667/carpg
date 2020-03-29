#include "Pch.h"
#include "Encounter.h"

Encounter::~Encounter()
{
	if(pooled_string)
		StringPool.Free(pooled_string);
}

const string& Encounter::GetTextS()
{
	if(!pooled_string)
	{
		pooled_string = StringPool.Get();
		text = pooled_string->c_str();
	}
	return *pooled_string;
}

void Encounter::SetTextS(const string& str)
{
	if(!pooled_string)
		pooled_string = StringPool.Get();
	*pooled_string = str;
	text = pooled_string->c_str();
}

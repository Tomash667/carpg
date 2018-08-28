#include "Pch.h"
#include "GameCore.h"
#include "UnitStats.h"
#include "BitStreamFunc.h"

void UnitStats::Write(BitStreamWriter& f) const
{
	f << attrib;
	f << skill;
}

void UnitStats::Read(BitStreamReader& f)
{
	f >> attrib;
	f >> skill;
}

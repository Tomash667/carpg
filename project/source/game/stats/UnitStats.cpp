#include "Pch.h"
#include "GameCore.h"
#include "UnitStats.h"
#include "BitStreamFunc.h"

std::map<std::pair<StatProfile*, int>, UnitStats*> UnitStats::shared_stats;

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

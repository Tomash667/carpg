#include "Pch.h"
#include "GameCore.h"
#include "World.h"
#include "BitStreamFunc.h"
#include "Language.h"

const int start_year = 100;
World W;

void World::Init()
{
	txDate = Str("dateFormat");
}

void World::OnNewGame()
{
	year = start_year;
	day = 0;
	month = 0;
	worldtime = 0;
}

void World::Update(int days)
{
	worldtime += days;
	day += days;
	if(day >= 30)
	{
		int count = day / 30;
		month += count;
		day -= count * 30;
		if(month >= 12)
		{
			count = month / 12;
			year += count;
			month -= count * 12;
		}
	}
}

void World::Save(FileWriter& f)
{
	f << year;
	f << month;
	f << day;
	f << worldtime;
}

void World::Load(FileReader& f)
{
	f >> year;
	f >> month;
	f >> day;
	f >> worldtime;
}

void World::WriteTime(BitStreamWriter& f)
{
	f << worldtime;
	f.WriteCasted<byte>(day);
	f.WriteCasted<byte>(month);
	f << year;
}

void World::ReadTime(BitStreamReader& f)
{
	f >> worldtime;
	f.ReadCasted<byte>(day);
	f.ReadCasted<byte>(month);
	f >> year;
}

bool World::IsSameWeek(int worldtime2) const
{
	if(worldtime2 == -1)
		return false;
	int week = worldtime / 10;
	int week2 = worldtime2 / 10;
	return week == week2;
}

cstring World::GetDate() const
{
	return Format("%d-%d-%d", year, month + 1, day + 1);
}

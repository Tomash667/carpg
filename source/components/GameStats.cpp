#include "Pch.h"
#include "GameStats.h"

#include "BitStreamFunc.h"

GameStats* game_stats;

void GameStats::Reset()
{
	total_kills = 0;
	hour = 0;
	minute = 0;
	second = 0;
	tick = 0.f;
}

void GameStats::Update(float dt)
{
	tick += dt;
	if(tick >= 1.f)
	{
		tick -= 1.f;
		++second;
		if(second == 60)
		{
			++minute;
			second = 0;
			if(minute == 60)
			{
				++hour;
				minute = 0;
			}
		}
	}
}

void GameStats::Save(GameWriter& f)
{
	f << total_kills;
	f << hour;
	f << minute;
	f << second;
	f << tick;
}

void GameStats::Load(GameReader& f)
{
	f >> total_kills;
	f >> hour;
	f >> minute;
	f >> second;
	f >> tick;
}

void GameStats::Write(BitStreamWriter& f)
{
	f << hour;
	f.WriteCasted<byte>(minute);
	f.WriteCasted<byte>(second);
}

void GameStats::Read(BitStreamReader& f)
{
	f >> hour;
	f.ReadCasted<byte>(minute);
	f.ReadCasted<byte>(second);
}

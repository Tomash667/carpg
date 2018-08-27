#include "Pch.h"
#include "GameCore.h"
#include "GameStats.h"
#include "GameFile.h"

GameStats Singleton<GameStats>::instance;

void GameStats::Reset()
{
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

void GameStats::Write(BitStream& stream)
{
	stream.Write(hour);
	stream.WriteCasted<byte>(minute);
	stream.WriteCasted<byte>(second);
}

bool GameStats::Read(BitStream& stream)
{
	return stream.Read(hour)
		&& stream.ReadCasted<byte>(minute)
		&& stream.ReadCasted<byte>(second);
}

void GameStats::Save(GameWriter& f)
{
	f << hour;
	f << minute;
	f << second;
	f << tick;
}

void GameStats::Load(GameReader& f)
{
	f >> hour;
	f >> minute;
	f >> second;
	f >> tick;
}

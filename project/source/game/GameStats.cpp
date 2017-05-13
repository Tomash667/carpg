#include "Pch.h"
#include "Base.h"
#include "GameStats.h"

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

void GameStats::Save(HANDLE file)
{
	DWORD tmp;
	WriteFile(file, &hour, sizeof(hour), &tmp, nullptr);
	WriteFile(file, &minute, sizeof(minute), &tmp, nullptr);
	WriteFile(file, &second, sizeof(second), &tmp, nullptr);
	WriteFile(file, &tick, sizeof(tick), &tmp, nullptr);
}

void GameStats::Load(HANDLE file)
{
	DWORD tmp;
	ReadFile(file, &hour, sizeof(hour), &tmp, nullptr);
	ReadFile(file, &minute, sizeof(minute), &tmp, nullptr);
	ReadFile(file, &second, sizeof(second), &tmp, nullptr);
	ReadFile(file, &tick, sizeof(tick), &tmp, nullptr);
}

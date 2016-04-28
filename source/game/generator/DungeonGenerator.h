#pragma once

class InsideLocation2;

namespace DungeonGenerator
{
	struct Config
	{

	};

	void Init();
	void Generate(InsideLocation2* loc, const Config& cfg);
}

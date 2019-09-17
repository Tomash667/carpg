#pragma once

//-----------------------------------------------------------------------------
namespace NameHelper
{
	void SetHeroNames();
	void GenerateHeroName(HeroData& hero);
	void GenerateHeroName(Class* clas, bool crazy, string& name);
}

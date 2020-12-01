#pragma once

//-----------------------------------------------------------------------------
namespace NameHelper
{
	void SetHeroNames();
	void GenerateHeroName(Hero& hero);
	void GenerateHeroName(const Class* clas, bool crazy, string& name);
}

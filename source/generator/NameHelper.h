#pragma once

//-----------------------------------------------------------------------------
namespace NameHelper
{
	void SetHeroNames();
	void GenerateHeroName(Hero& hero);
	void GenerateHeroName(Class* clas, bool crazy, string& name);
}

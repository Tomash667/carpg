#include "Pch.h"
#include "Buff.h"

#include "Language.h"

#include <ResourceManager.h>

BuffInfo BuffInfo::info[] = {
	BuffInfo("buffRegeneration"),
	BuffInfo("buffNatural"),
	BuffInfo("buffFood"),
	BuffInfo("buffAlcohol"),
	BuffInfo("buffPoison"),
	BuffInfo("buffAntimagic"),
	BuffInfo("buffStamina"),
	BuffInfo("buffStun"),
	BuffInfo("buffPoisonRes"),
	BuffInfo("buffRooted"),
	BuffInfo("buffSlowMove")
};

void BuffInfo::LoadImages()
{
	for(int i = 0; i < BUFF_COUNT; ++i)
	{
		BuffInfo& buff = info[i];
		cstring path = Format("%s.png", buff.id);
		buff.img = resMgr->Load<Texture>(path);
	}
}

void BuffInfo::LoadText()
{
	Language::Section s = Language::GetSection("Buffs");
	for(int i = 0; i < BUFF_COUNT; ++i)
	{
		auto& buff = info[i];
		buff.text = s.Get(buff.id);
	}
}

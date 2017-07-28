#include "Pch.h"
#include "Core.h"
#include "Buff.h"
#include "Language.h"
#include "ResourceManager.h"

BuffInfo BuffInfo::info[] = {
	BuffInfo("buff_regeneration"),
	BuffInfo("buff_natural"),
	BuffInfo("buff_food"),
	BuffInfo("buff_alcohol"),
	BuffInfo("buff_poison"),
	BuffInfo("buff_antimagic"),
	BuffInfo("buff_stamina")
};

void BuffInfo::LoadImages(ResourceManager& res_mgr)
{
	for(int i=0; i<BUFF_COUNT; ++i)
	{
		auto& buff = info[i];
		cstring path = Format("%s.png", buff.id);
		res_mgr.GetLoadedTexture(path, buff.img);
	}
}

void BuffInfo::LoadText()
{
	for(int i=0; i<BUFF_COUNT; ++i)
	{
		auto& buff = info[i];
		buff.text = Str(buff.id);
	}
}

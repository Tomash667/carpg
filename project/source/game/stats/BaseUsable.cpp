// bazowy u¿ywalny obiekt
#include "Pch.h"
#include "Core.h"
#include "BaseUsable.h"
#include "ResourceManager.h"

//-----------------------------------------------------------------------------
BaseUsable g_base_usables[] = {
	BaseUsable("u_chair", "chair", "siedzi_krzeslo", 2, BaseUsable::ALLOW_USE),
	BaseUsable("u_bench", "bench", "siedzi_lawa", 3, BaseUsable::ALLOW_USE),
	BaseUsable("u_anvil", "anvil", "kowalstwo", 1, BaseUsable::SLOW_STAMINA_RESTORE, "blacksmith_hammer", "blacksmith.mp3", 10.f / 30),
	BaseUsable("u_cauldron", "cauldron", "miesza", 0, BaseUsable::SLOW_STAMINA_RESTORE, "ladle"),
	BaseUsable("u_iron_vein", "iron_ore", "wydobywa", 4, BaseUsable::SLOW_STAMINA_RESTORE, "pickaxe", "kilof.mp3", 22.f / 40),
	BaseUsable("u_gold_vein", "gold_ore", "wydobywa", 4, BaseUsable::SLOW_STAMINA_RESTORE, "pickaxe", "kilof.mp3", 22.f / 40),
	BaseUsable("u_throne", "throne", "siedzi_tron", 2, BaseUsable::ALLOW_USE),
	BaseUsable("u_stool", "stool", "siedzi_krzeslo", 2, BaseUsable::ALLOW_USE),
	BaseUsable("u_bench_dir", "bench_dir", "siedzi_lawa", 2, BaseUsable::ALLOW_USE),
	BaseUsable("u_bookshelf", "bookshelf", nullptr, 1, BaseUsable::CONTAINER)
};
const uint n_base_usables = countof(g_base_usables);

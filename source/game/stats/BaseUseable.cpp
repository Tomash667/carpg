// bazowy u¿ywalny obiekt
#include "Pch.h"
#include "Base.h"
#include "BaseUseable.h"

//-----------------------------------------------------------------------------
BaseUsable g_base_usables[] = {
	BaseUsable("u_chair", "chair", "siedzi_krzeslo", 2, true),
	BaseUsable("u_bench", "bench", "siedzi_lawa", 3, true),
	BaseUsable("u_anvil", "anvil", "kowalstwo", 1, false, "blacksmith_hammer", "blacksmith.mp3", 10.f/30),
	BaseUsable("u_cauldron", "cauldron", "miesza", 0, false, "ladle"),
	BaseUsable("u_iron_vain", "iron_ore", "wydobywa", 4, false, "pickaxe", "kilof.mp3", 22.f/40),
	BaseUsable("u_gold_vain", "gold_ore", "wydobywa", 4, false, "pickaxe", "kilof.mp3", 22.f/40),
	BaseUsable("u_throne", "throne", "siedzi_tron", 2, true),
	BaseUsable("u_stool", "stool", "siedzi_krzeslo", 2, true),
	BaseUsable("u_bench_dir", "bench_dir", "siedzi_lawa", 2, true),
};
const uint n_base_usables = countof(g_base_usables);

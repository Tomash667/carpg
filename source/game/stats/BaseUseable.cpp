// bazowy u¿ywalny obiekt
#include "Pch.h"
#include "Base.h"
#include "BaseUseable.h"
#include "Object.h"

//-----------------------------------------------------------------------------
BaseUsable g_base_usables[] = {
	BaseUsable("u_chair", "chair", "siedzi_krzeslo", 2, true),
	BaseUsable("u_bench", "bench", "siedzi_lawa", 3, true),
	BaseUsable("u_anvil", "anvil", "kowalstwo", 1, false, "blunt_blacksmith", "blacksmith.mp3", 10.f/30),
	BaseUsable("u_cauldron", "cauldron", "miesza", 0, false, "ladle"),
	BaseUsable("u_iron_vain", "iron_ore", "wydobywa", 4, false, "pickaxe", "kilof.mp3", 22.f/40),
	BaseUsable("u_gold_vain", "gold_ore", "wydobywa", 4, false, "pickaxe", "kilof.mp3", 22.f/40),
	BaseUsable("u_throne", "throne", "siedzi_tron", 2, true),
	BaseUsable("u_stool", "stool", "siedzi_krzeslo", 2, true),
	BaseUsable("u_bench_dir", "bench_dir", "siedzi_lawa", 2, true),
};
const uint n_base_usables = countof(g_base_usables);

//=================================================================================================
// Convert object flags to useable enum
USEABLE_ID ObjectToUseable(OBJ_FLAGS obj_flags, OBJ_FLAGS2 obj_flags2)
{
	if(IS_SET(obj_flags, OBJ_BENCH))
		return U_BENCH;
	else if(IS_SET(obj_flags, OBJ_ANVIL))
		return U_ANVIL;
	else if(IS_SET(obj_flags, OBJ_CHAIR))
		return U_CHAIR;
	else if(IS_SET(obj_flags, OBJ_CAULDRON))
		return U_CAULDRON;
	else if(IS_SET(obj_flags, OBJ_IRON_VAIN))
		return U_IRON_VAIN;
	else if(IS_SET(obj_flags, OBJ_GOLD_VAIN))
		return U_GOLD_VAIN;
	else if(IS_SET(obj_flags, OBJ_THRONE))
		return U_THRONE;
	else if(IS_SET(obj_flags, OBJ_STOOL))
		return U_STOOL;
	else if(IS_SET(obj_flags2, OBJ2_BENCH_ROT))
		return U_BENCH_ROT;
	else
	{
		assert(0);
		return U_CHAIR;
	}
}

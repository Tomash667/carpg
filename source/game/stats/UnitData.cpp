#include "Pch.h"
#include "Base.h"
#include "UnitData.h"
#include "ItemScript.h"
#include "Dialog.h"
#include "Spell.h"
#include "Item.h"
#include "Crc.h"

extern string g_system_dir;

vector<StatProfile*> stat_profiles;
vector<ItemScript*> item_scripts;
vector<SpellList*> spell_lists;
vector<SoundPack*> sound_packs;
vector<IdlePack*> idle_packs;
vector<TexPack*> tex_packs;
vector<FrameInfo*> frame_infos;
vector<UnitData*> unit_datas;
std::map<string, DialogEntry*> dialogs_map;

//=================================================================================================
// PRZEDMIOTY POSTACI
//=================================================================================================
#define S(x) ((int)x)

int base_warrior_items[] = {
	PS_MANY, 4, S("p_hp"),
	PS_MANY, 2, S("p_nreg"),
	PS_END
};

int base_hunter_items[] = {
	PS_MANY, 2, S("p_hp"),
	PS_MANY, 3, S("p_nreg"),
	PS_END
};

int base_rogue_items[] = {
	PS_MANY, 3, S("p_hp"),
	PS_MANY, 2, S("p_nreg"),
	PS_END
};

int blacksmith_items[] = {
	PS_ONE, S("al_blacksmith"),
	PS_ONE, S("blunt_blacksmith"),
	PS_RANDOM, 1, 3, S("!normal_food"),
	PS_END
};

int merchant_items[] = {
	PS_ONE_OF_MANY, 2, S("al_clothes2"), S("al_clothes2b"),
	PS_ONE, S("dagger_sword"),
	PS_RANDOM, 1, 3, S("!normal_food"),
	PS_END
}; 

int alchemist_items[] = {
	PS_ONE_OF_MANY, 2, S("al_clothes2"), S("al_clothes2b"),
	PS_ONE, S("dagger_short"),
	PS_ONE, S("p_hp"),
	PS_ONE, S("ladle"),
	PS_RANDOM, 1, 3, S("!normal_food"),
	PS_END
};

int goblin_items[] = {
	PS_CHANCE, 20, S("bow_short"),
	PS_CHANCE, 20, S("shield_wood"),
	PS_CHANCE, 5, S("p_hp"),
	PS_ONE_OF_MANY, 3, S("dagger_short"), S("axe_small"), S("blunt_club"),
	PS_CHANCE, 25, S("al_goblin"),
	PS_RANDOM, 0, 2, S("!orc_food"),
	PS_END
};

int tut_goblin_items[] = {
	PS_CHANCE, 20, S("shield_wood"),
	PS_ONE_OF_MANY, 3, S("dagger_short"), S("axe_small"), S("blunt_club"),
	PS_CHANCE, 25, S("al_goblin"),
	PS_RANDOM, 0, 2, S("!orc_food"),
	PS_END
};

int goblin_hunter_items[] = {
	PS_IF_LEVEL, 5,
		PS_ONE, S("bow_long"),
	PS_ELSE,
		PS_ONE, S("bow_short"),
	PS_END_IF,
	PS_CHANCE, 10, S("p_hp"),
	PS_ONE_OF_MANY, 3, S("dagger_short"), S("axe_small"), S("blunt_club"),
	PS_CHANCE, 50, S("al_goblin"),
	PS_CHANCE, 25, S("shield_wood"),
	PS_RANDOM, 0, 2, S("!orc_food"),
	PS_END
};

int goblin_fighter_items[] = {
	PS_IF_LEVEL, 5,
		PS_CHANCE2, 25, S("shield_iron"), S("shield_wood"),
		PS_ONE_OF_MANY, 4, S("dagger_sword"), S("axe_small"), S("blunt_club"), S("sword_long"),
	PS_ELSE,
		PS_CHANCE, 75, S("shield_wood"),
		PS_ONE_OF_MANY, 3, S("dagger_short"), S("axe_small"), S("blunt_club"),
	PS_END_IF,
	PS_CHANCE, 15, S("p_hp"),
	PS_ONE, S("al_goblin"),
	PS_RANDOM, 0, 2, S("!orc_food"),
	PS_END
};

int goblin_chief_items[] = {
	PS_CHANCE, 25, S("p_hp"),
	PS_ONE, S("al_goblin"),
	PS_CHANCE2, 75, S("shield_iron"), S("shield_wood"),
	PS_ONE_OF_MANY, 5, S("dagger_sword"), S("axe_small"), S("blunt_mace"), S("sword_long"), S("sword_scimitar"),
	PS_RANDOM, 1, 3, S("!orc_food"),
	PS_END
};

int orc_items[] = {
	PS_CHANCE, 10, S("p_hp"),
	PS_CHANCE, 33, S("al_orc"),
	PS_CHANCE, 75, S("shield_wood"),
	PS_ONE_OF_MANY, 3, S("blunt_mace"), S("axe_small"), S("sword_long"),
	PS_RANDOM, 1, 3, S("!orc_food"),
	PS_END
};

int orc_hunter_items[] = {
	PS_IF_LEVEL, 7,
		PS_ONE, S("bow_long"),
		PS_ONE, S("shield_wood"),
		PS_IF_CHANCE, 50,
			PS_ONE_OF_MANY, 3, S("blunt_orcish"), S("axe_orcish"), S("sword_orcish"),
		PS_ELSE,
			PS_ONE_OF_MANY, 3, S("blunt_mace"), S("axe_small"), S("sword_long"),
		PS_END_IF,
	PS_ELSE,
		PS_CHANCE2, 50, S("bow_long"), S("bow_short"),
		PS_ONE_OF_MANY, 3, S("blunt_mace"), S("axe_small"), S("sword_long"),
		PS_CHANCE, 75, S("shield_wood"),
	PS_END_IF,
	PS_CHANCE, 15, S("p_hp"),
	PS_CHANCE, 66, S("al_orc"),
	PS_RANDOM, 1, 3, S("!orc_food"),
	PS_END
};

int orc_fighter_items[] = {
	PS_IF_LEVEL, 7,
		PS_CHANCE2, 50, S("am_orc"), S("al_orc"),
		PS_ONE_OF_MANY, 3, S("blunt_orcish"), S("axe_orcish"), S("sword_orcish"),
		PS_ONE, S("shield_iron"),
	PS_ELSE,
		PS_IF_CHANCE, 50,
			PS_ONE_OF_MANY, 3, S("blunt_orcish"), S("axe_orcish"), S("sword_orcish"),
			PS_ONE, S("shield_iron"),
		PS_ELSE,
			PS_ONE_OF_MANY, 3, S("blunt_mace"), S("axe_small"), S("sword_long"),
			PS_CHANCE2, 50, S("shield_iron"), S("shield_wood"),
		PS_END_IF,
		PS_CHANCE2, 20, S("am_orc"), S("al_orc"),
	PS_END_IF,
	PS_CHANCE, 25, S("p_hp"),
	PS_RANDOM, 1, 3, S("!orc_food"),
	PS_END
};

int orc_chief_items[] = {
	PS_IF_LEVEL, 10,
		PS_ONE, S("am_orc"),
		PS_CHANCE2, 50, S("shield_steel"), S("shield_iron"),
		PS_IF_CHANCE, 25,
			PS_ONE_OF_MANY, 3, S("blunt_dwarven"), S("axe_crystal"), S("sword_serrated"),
		PS_ELSE,
			PS_ONE_OF_MANY, 3, S("blunt_orcish"), S("axe_orcish"), S("sword_orcish"),
		PS_END_IF,
	PS_ELSE,
		PS_CHANCE2, 75, S("am_orc"), S("al_orc"),
		PS_ONE, S("shield_iron"),
		PS_ONE_OF_MANY, 3, S("blunt_orcish"), S("axe_orcish"), S("sword_orcish"),
	PS_END_IF,
	PS_ONE, S("p_hp2"),
	PS_RANDOM, 2, 4, S("!orc_food"),
	PS_END
};

int orc_shaman_items[] = {
	PS_ONE, S("al_orc_shaman"),
	PS_ONE, S("blunt_orcish_shaman"),
	PS_CHANCE, 15, S("p_hp"),
	PS_RANDOM, 1, 3, S("!orc_food"),
	PS_END
};

int skeleton_items[] = {
	PS_CHANCE2, 50, S("bow_short"), S("shield_wood"),
	PS_ONE_OF_MANY, 5, S("dagger_short"), S("dagger_sword"), S("sword_long"), S("axe_small"), S("blunt_club"),
	PS_END
};

int skeleton_archer_items[] = {
	PS_CHANCE2, 50, S("bow_long"), S("bow_short"),
	PS_ONE_OF_MANY, 5, S("dagger_short"), S("dagger_sword"), S("sword_long"), S("axe_small"), S("blunt_club"),
	PS_CHANCE, 50, S("shield_wood"),
	PS_END
};

int skeleton_fighter_items[] = {
	PS_CHANCE2, 50, S("shield_wood"), S("shield_iron"),
	PS_CHANCE, 50, S("bow_short"),
	PS_IF_CHANCE, 50,
		PS_ONE_OF_MANY, 4, S("dagger_rapier"), S("sword_scimitar"), S("axe_battle"), S("blunt_mace"),
	PS_ELSE,
		PS_ONE_OF_MANY, 5, S("dagger_short"), S("dagger_sword"), S("sword_long"), S("axe_small"), S("blunt_club"),
	PS_END_IF,
	PS_END
};

int skeleton_mage_items[] = {
	PS_ONE, S("wand_1"),
	PS_END
};

int bandit_items[] = {
	PS_IF_CHANCE, 50,
		PS_ONE, S("!medium_armor"),
		PS_ONE, S("!shield"),
		PS_ONE_OF_MANY, 3, S("!long_blade"), S("!axe"), S("!blunt"),
		PS_CHANCE, 50, S("!-2bow"),
	PS_ELSE,
		PS_ONE, S("!light_armor"),
		PS_ONE, S("!-2shield"),
		PS_ONE_OF_MANY, 4, S("!long_blade"), S("!axe"), S("!blunt"), S("!short_blade"),
		PS_CHANCE, 50, S("!bow"),
	PS_END_IF,
	PS_CHANCE, 10, S("p_hp"),
	PS_RANDOM, 1, 3, S("!normal_food"),
	PS_END
};

int bandit_archer_items[] = {
	PS_ONE, S("!light_armor"),
	PS_ONE, S("!bow"),
	PS_ONE_OF_MANY, 4, S("!-2long_blade"), S("!-2axe"), S("!-2blunt"), S("!-2short_blade"),
	PS_CHANCE, 10, S("p_hp"),
	PS_RANDOM, 1, 3, S("!normal_food"),
	PS_END
};

int bandit_hegemon_items[] = {
	PS_ONE_OF_MANY, 2, S("!medium_armor"), S("!heavy_armor"),
	PS_ONE_OF_MANY, 4, S("!long_blade"), S("!axe"), S("!blunt"), S("!short_blade"),
	PS_ONE, S("!shield"),
	PS_CHANCE, 50, S("!-2bow"),
	PS_ONE, S("p_hp2"),
	PS_RANDOM, 2, 4, S("!normal_food"),
	PS_END
};

int mage_items[] = {
	PS_ONE, S("!wand"),
	PS_ONE, S("!mage_armor"),
	PS_IF_LEVEL, 12,
		PS_CHANCE, 50, S("p_hp"),
	PS_ELSE,
		PS_IF_LEVEL, 8,
			PS_CHANCE, 25, S("p_hp"),
		PS_ELSE,
			PS_CHANCE, 10, S("p_hp"),
		PS_END_IF,
	PS_END_IF,
	PS_RANDOM, 1, 3, S("!normal_food"),
	PS_END
};

int guard_items[] = {
	PS_IF_CHANCE, 33,
		PS_ONE, S("!light_armor"),
		PS_ONE_OF_MANY, 4, S("!-1long_blade"), S("!-1axe"), S("!-1blunt"), S("!-1short_blade"),
		PS_ONE, S("!bow"),
	PS_ELSE,
		PS_IF_CHANCE, 50,
			PS_ONE, S("!medium_armor"),
			PS_ONE_OF_MANY, 4, S("!long_blade"), S("!axe"), S("!blunt"), S("!short_blade"),
			PS_ONE_OF_MANY, 2, S("!-2bow"), S("!-2shield"),
		PS_ELSE,
			PS_ONE, S("!heavy_armor"),
			PS_ONE_OF_MANY, 3, S("!long_blade"), S("!axe"), S("!blunt"),
			PS_ONE, S("!shield"),
		PS_END_IF,
	PS_END_IF,
	PS_IF_LEVEL, 9,
		PS_CHANCE, 50, S("p_hp2"),
	PS_ELSE,
		PS_CHANCE, 50, S("p_hp"),
	PS_END_IF,
	PS_RANDOM, 1, 3, S("!normal_food"),
	PS_END
};

int q_guard_items[] = {
	PS_MANY, 3, S("p_hp2"),
	PS_IF_CHANCE, 33,
		PS_ONE, S("!light_armor"),
		PS_ONE_OF_MANY, 4, S("!-1long_blade"), S("!-1axe"), S("!-1blunt"), S("!-1short_blade"),
		PS_ONE, S("!bow"),
	PS_ELSE,
		PS_IF_CHANCE, 50,
			PS_ONE, S("!medium_armor"),
			PS_ONE_OF_MANY, 4, S("!long_blade"), S("!axe"), S("!blunt"), S("!short_blade"),
			PS_ONE_OF_MANY, 2, S("!-2bow"), S("!-2shield"),
		PS_ELSE,
			PS_ONE, S("!heavy_armor"),
			PS_ONE_OF_MANY, 3, S("!long_blade"), S("!axe"), S("!blunt"),
			PS_ONE, S("!shield"),
		PS_END_IF,
	PS_END_IF,
	PS_IF_LEVEL, 9,
		PS_CHANCE, 50, S("p_hp2"),
	PS_ELSE,
		PS_CHANCE, 50, S("p_hp"),
	PS_END_IF,
	PS_RANDOM, 1, 3, S("!normal_food"),
	PS_END
};

int necromancer_items[] = {
	PS_ONE, S("al_necromancer"),
	PS_ONE, S("!wand"),
	PS_RANDOM, 1, 3, S("f_raw_meat"),
	PS_END
};

int evil_cleric_items[] = {
	PS_ONE, S("!blunt"),
	PS_ONE, S("!shield"),
	PS_IF_LEVEL, 10,
		PS_ONE, S("!heavy_armor"),
	PS_ELSE,
		PS_ONE, S("!medium_armor"),
	PS_END_IF,
	PS_ONE, S("p_hp2"),
	PS_RANDOM, 1, 3, S("f_raw_meat"),
	PS_END
};

int warrior_items[] = {
	PS_IF_CHANCE, 50,
		PS_ONE, S("!heavy_armor"),
		PS_ONE_OF_MANY, 3, S("!long_blade"), S("!axe"), S("!blunt"),
		PS_ONE, S("!shield"),
		PS_ONE, S("!-4bow"),
	PS_ELSE,
		PS_ONE, S("!medium_armor"),
		PS_ONE_OF_MANY, 4, S("!long_blade"), S("!axe"), S("!blunt"), S("!short_blade"),
		PS_ONE, S("!-2shield"),
		PS_ONE, S("!-2bow"),
	PS_END_IF,
	PS_IF_LEVEL, 15,
		PS_ONE, S("p_hp3"),
		PS_MANY, 3, S("p_hp2"),
	PS_ELSE,
		PS_IF_LEVEL, 12,
			PS_MANY, 2, S("p_hp2"),
			PS_ONE, S("p_hp"),
		PS_ELSE,
			PS_IF_LEVEL, 9,
				PS_ONE, S("p_hp2"),
				PS_MANY, 2, S("p_hp"),				
			PS_ELSE,
				PS_ONE, S("p_hp"),
				PS_IF_LEVEL, 6,
					PS_ONE, S("p_hp"),					
				PS_END_IF,
			PS_END_IF,
		PS_END_IF,
	PS_END_IF,
	PS_RANDOM, 1, 3, S("!normal_food"),
	PS_END
};

int hunter_items[] = {
	PS_ONE_OF_MANY, 2, S("!-1light_armor"), S("!-1medium_armor"),
	PS_ONE, S("!bow"),
	PS_ONE, S("!-2shield"),
	PS_ONE_OF_MANY, 4, S("!-2long_blade"), S("!-2axe"), S("!-2blunt"), S("!-2short_blade"),
	PS_IF_LEVEL, 15,
		PS_ONE, S("p_hp3"),
		PS_ONE, S("p_hp2"),
	PS_ELSE,
		PS_IF_LEVEL, 12,
			PS_MANY, 2, S("p_hp2"),
		PS_ELSE,
			PS_IF_LEVEL, 9,
				PS_ONE, S("p_hp2"),
				PS_ONE, S("p_hp"),
			PS_ELSE,
				PS_ONE, S("p_hp"),
				PS_IF_LEVEL, 6,
					PS_ONE, S("p_hp"),
				PS_END_IF,
			PS_END_IF,
		PS_END_IF,
	PS_END_IF,
	PS_RANDOM, 1, 3, S("!normal_food"),
	PS_END
};

int rogue_items[] = {
	PS_ONE_OF_MANY, 2, S("!short_blade"), S("!long_blade"),
	PS_ONE, S("!light_armor"),
	PS_ONE_OF_MANY, 2, S("!-2bow"), S("!-2shield"),
	PS_IF_LEVEL, 15,
		PS_ONE, S("p_hp3"),
		PS_ONE, S("p_hp2"),
	PS_ELSE,
		PS_IF_LEVEL, 12,
			PS_MANY, 2, S("p_hp2"),
		PS_ELSE,
			PS_IF_LEVEL, 9,
				PS_ONE, S("p_hp2"),
				PS_ONE, S("p_hp"),
			PS_ELSE,
				PS_ONE, S("p_hp"),
				PS_IF_LEVEL, 6,
					PS_ONE, S("p_hp"),
				PS_END_IF,
			PS_END_IF,
		PS_END_IF,
	PS_END_IF,
	PS_RANDOM, 1, 3, S("!normal_food"),
	PS_END
};

int szaleniec_items[] = {
	PS_ONE, S("p_hp2"),
	PS_ONE_OF_MANY, 4, S("!-2long_blade"), S("!-2axe"), S("!-2blunt"), S("!-2short_blade"),
	PS_ONE, S("!bow"),
	PS_ONE, S("!light_armor"),
	PS_ONE, S("!-2shield"),
	PS_ONE, S("q_szaleni_kamien"),
	PS_RANDOM, 1, 3, S("!normal_food"),
	PS_END
};

int xar_items[] = {
	PS_ONE, S("blunt_skullsmasher"),
	PS_ONE, S("ah_black_armor"),
	PS_ONE, S("shield_unique"),
	PS_MANY, 3, S("p_hp3"),
	PS_ONE, S("p_str"),
	PS_ONE, S("p_end"),
	PS_RANDOM, 2, 4, S("f_raw_meat"),
	PS_END
};

int tomash_items[] = {
	PS_ONE, S("sword_forbidden"),
	PS_ONE, S("ah_black_armor"),
	PS_ONE, S("shield_adamantine"),
	PS_ONE, S("bow_dragonbone"),
	PS_MANY, 3, S("p_hp3"),
	PS_MANY, 5, S("vs_krystal"),
	PS_END
};

int citizen_items[] = {
	PS_IF_CHANCE, 40,
		PS_ONE_OF_MANY, 2, S("al_clothes2"), S("al_clothes2b"),
	PS_ELSE,
		PS_ONE, S("al_clothes"),
	PS_END_IF,
	PS_ONE_OF_MANY, 3, S("dagger_short"), S("axe_small"), S("blunt_club"),
	PS_RANDOM, 1, 3, S("!normal_food"),
	PS_END
};

int poslaniec_items[] = {
	PS_ONE, S("al_clothes"),
	PS_ONE_OF_MANY, 3, S("dagger_short"), S("axe_small"), S("blunt_club"),
	PS_RANDOM, 1, 3, S("!normal_food"),
	PS_END
};

int captive_items[] = {
	PS_IF_CHANCE, 40,
		PS_ONE_OF_MANY, 2, S("al_clothes3"), S("al_clothes3b"),
	PS_ELSE,
		PS_ONE_OF_MANY, 2, S("al_clothes2"), S("al_clothes2b"),
	PS_END_IF,
	PS_ONE_OF_MANY, 2, S("dagger_short"), S("dagger_rapier"),
	PS_RANDOM, 0, 2, S("!normal_food"),
	PS_END
};

int burmistrz_items[] = {
	PS_ONE_OF_MANY, 2, S("al_clothes3"), S("al_clothes3b"),
	PS_ONE_OF_MANY, 4, S("dagger_sword"), S("dagger_rapier"), S("sword_long"), S("sword_scimitar"),
	PS_RANDOM, 1, 3, S("!normal_food"),
	PS_END
};

int karczmarz_items[] = {
	PS_ONE, S("al_innkeeper"),
	PS_ONE_OF_MANY, 3, S("blunt_mace"), S("axe_small"), S("sword_long"),
	PS_MANY, 5, S("p_beer"),
	PS_MANY, 5, S("p_water"),
	PS_MANY, 3, S("p_vodka"),
	PS_RANDOM, 1, 3, S("!normal_food"),
	PS_END
};

int podrozny_items[] = {
	PS_ONE_OF_MANY, 3, S("al_clothes"), S("al_leather"), S("al_padded"),
	PS_ONE_OF_MANY, 3, S("blunt_mace"), S("axe_small"), S("sword_long"),
	PS_CHANCE, 50, S("shield_wood"),
	PS_CHANCE, 50, S("bow_short"),
	PS_RANDOM, 1, 3, S("!normal_food"),
	PS_END
};

int artur_drwal_items[] = {
	PS_ONE, S("al_studded"),
	PS_ONE, S("axe_battle"),
	PS_RANDOM, 1, 3, S("!normal_food"),
	PS_END
};

int drwal_items[] = {
	PS_ONE_OF_MANY, 3, S("al_leather"), S("al_clothes"), S("al_padded"),
	PS_ONE_OF_MANY, 2, S("axe_small"), S("axe_battle"),
	PS_RANDOM, 1, 3, S("!normal_food"),
	PS_END
};

int gornik_items[] = {
	PS_ONE, S("pickaxe"),
	PS_ONE_OF_MANY, 3, S("al_leather"), S("al_clothes"), S("al_padded"),
	PS_RANDOM, 1, 3, S("!normal_food"),
	PS_END
};

int inwestor_items[] = {
	PS_ONE_OF_MANY, 2, S("al_clothes3"), S("al_clothes3b"),
	PS_ONE, S("dagger_rapier"),
	PS_RANDOM, 1, 3, S("!normal_food"),
	PS_END
};

int q_bandyci_szef_items[] = {
	PS_CHANCE2, 25, S("bow_elven"), S("bow_composite"),
	PS_ONE, S("al_dragonskin"),
	PS_ONE, S("dagger_spinesheath"),
	PS_CHANCE2, 25, S("shield_mithril"), S("shield_steel"),
	PS_ONE, S("p_hp2"),
	PS_ONE, S("p_dex"),
	PS_RANDOM, 2, 4, S("!normal_food"),
	PS_END
};

int q_magowie_boss_items[] = {
	PS_ONE, S("sword_unique"),
	PS_ONE, S("al_mage4"),
	PS_ONE, S("p_hp2"),
	PS_ONE, S("q_magowie_kula2"),
	PS_ONE, S("p_end"),
	PS_RANDOM, 2, 4, S("!normal_food"),
	PS_END
};

int gorush_items[] = {
	PS_ONE, S("p_hp"),
	PS_ONE, S("al_orc"),
	PS_ONE, S("shield_wood"),
	PS_ONE, S("axe_orcish"),
	PS_ONE, S("bow_short"),
	PS_RANDOM, 0, 2, S("!orc_food"),
	PS_END
};

int gorush_woj_items[] = {
	PS_ONE, S("p_hp2"),
	PS_ONE, S("am_orc"),
	PS_ONE, S("shield_steel"),
	PS_ONE, S("axe_crystal"),
	PS_ONE, S("bow_long"),
	PS_RANDOM, 3, 5, S("!orc_food"),
	PS_END
};

int gorush_lowca_items[] = {
	PS_ONE, S("p_hp2"),
	PS_ONE, S("bow_composite"),
	PS_ONE, S("shield_iron"),
	PS_ONE, S("axe_orcish"),
	PS_ONE, S("al_orc"),
	PS_RANDOM, 3, 5, S("!orc_food"),
	PS_END
};

int gorush_szaman_items[] = {
	PS_ONE, S("al_orc_shaman"),
	PS_ONE, S("blunt_orcish_shaman"),
	PS_ONE, S("p_hp"),
	PS_ONE, S("shield_iron"),
	PS_RANDOM, 3, 5, S("!orc_food"),
	PS_END
};

int orkowie_boss_items[] = {
	PS_ONE, S("al_orc_shaman"),
	PS_ONE, S("axe_ripper"),
	PS_MANY, 3, S("p_hp3"),
	PS_ONE, S("shield_adamantine"),
	PS_ONE, S("bow_composite"),
	PS_ONE, S("p_str"),
	PS_RANDOM, 3, 5, S("!orc_food"),
	PS_END
};

int orkowie_slaby_items[] = {
	PS_ONE_OF_MANY, 3, S("blunt_club"), S("axe_small"), S("dagger_short"),
	PS_RANDOM, 0, 2, S("!orc_food"),
	PS_END
};

int szlachcic_items[] = {
	PS_ONE, S("dagger_assassin"),
	PS_ONE, S("bow_unique"),
	PS_ONE, S("al_chain_shirt_mith"),
	PS_ONE, S("p_hp3"),
	PS_MANY, 2, S("p_hp2"),
	PS_ONE, S("p_dex"),
	PS_RANDOM, 1, 3, S("!normal_food"),
	PS_END
};

int kaplan_items[] = {
	PS_ONE, S("blunt_morningstar"),
	PS_ONE, S("shield_iron"),
	PS_ONE, S("am_breastplate_hq"),
	PS_ONE, S("p_hp2"),
	PS_RANDOM, 1, 3, S("!normal_food"),
	PS_END
};

int wolf_items[] = {
	PS_RANDOM, 2, 3, S("f_raw_meat"),
	PS_END
};

//=================================================================================================
// ZAKL CIA POSTACI
//=================================================================================================
SpellList orc_shaman_spells(0, "magic_bolt", 9, "fireball", 0, nullptr, false);
SpellList zombie_rotting_spells(0, "spit_poison", 0, nullptr, 0, nullptr, false);
SpellList skeleton_mage_spells(0, "magic_bolt", 8, "fireball", 10, "raise", true);
SpellList mage_spells(0, "magic_bolt", 8, "fireball", 12, "thunder_bolt", false);
SpellList necromancer_spells(0, "magic_bolt", 5, "raise", 10, "exploding_skull", true);
SpellList evil_cleric_spells(0, nullptr, 5, "drain", 10, "raise", true);
SpellList xar_spells(0, "exploding_skull", 0, "drain2", 0, nullptr, false);
SpellList mage_boss_spells(0, "xmagic_bolt", 0, "fireball", 0, "thunder_bolt", false);
SpellList tomash_spells(0, "mystic_ball", 0, nullptr, 0, nullptr, false);
SpellList jozan_spells(0, "heal", 0, nullptr, 0, nullptr, true);

//=================================================================================================
// DèWI KI POSTACI
//=================================================================================================
SoundPack sounds_def("hey.mp3", "aahhzz.mp3", "ahhh.mp3", nullptr);
SoundPack sounds_wolf("wolf_x.mp3", "dog_whine.mp3", "dog_whine.mp3", "wolf_attack.mp3");
SoundPack sounds_spider("spider_tarantula_hiss.mp3", nullptr, nullptr, "spider_bite_and_suck.mp3");
SoundPack sounds_rat("rat.wav", nullptr, "ratdeath.mp3", nullptr);
SoundPack sounds_zombie("zombie groan 1.wav", "zombie.mp3", "zombie2.mp3", "zombie attack.wav");
SoundPack sounds_skeleton("skeleton_alert.mp3", "bone_break.mp3", "falling-bones.mp3", nullptr);
SoundPack sounds_golem("golem_alert.mp3", nullptr, "earthquake.wav", nullptr);
SoundPack sounds_golem_iron("golem_alert.mp3", nullptr, "irongolem_break.mp3", nullptr);
SoundPack sounds_goblin("goblin-3.wav", "goblin-11.wav", "goblin-15.wav", nullptr);
SoundPack sounds_orc("ogre2.wav", "ogre5.wav", "ogre3.wav", nullptr);
SoundPack sounds_orc_boss("snarl.mp3", "ogre5.wav", "ogre3.wav", nullptr);
SoundPack sounds_boss("shade8.wav", "shade5.wav", "scream.wav", nullptr);
SoundPack sounds_undead("shade8.wav", "shade5.wav", "shade12.wav", nullptr);
SoundPack sounds_nieznany(nullptr, "mnstr15.wav", "mnstr7.wav", "mnstr2.wav");

//=================================================================================================
// ATAKI GRACZA
//=================================================================================================
AttackFrameInfo human_attacks = { {
	{ 10.f / 30, 21.f / 30, A_LONG_BLADE | A_AXE | A_BLUNT | A_SHORT_BLADE },
	{ 9.f / 30, 22.f / 30, A_LONG_BLADE | A_AXE | A_BLUNT },
	{ 7.f / 30, 21.f / 30, A_LONG_BLADE | A_SHORT_BLADE | A_BLUNT },
	{ 11.f / 30, 21.f / 30, A_LONG_BLADE | A_AXE | A_BLUNT },
	{ 9.f / 25, 18.f / 25, A_SHORT_BLADE | A_LONG_BLADE }
} };

//=================================================================================================
// ATAKI POSTACI
//=================================================================================================
FrameInfo fi_human(&human_attacks, { 30.f / 35, 10.f / 25, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 20.f / 25 }, 5);
FrameInfo fi_skeleton(nullptr, { 1.f, 10.f / 20, 6.f / 15, 1.f, 11.f / 23, 1.f, 8.f / 17, 1.f, 20.f / 30 }, 3);
FrameInfo fi_zombie(nullptr, { 20.f / 30.f, 0.f, 15.f / 20, 1.f, 9.f / 15, 1.f, 0.f, 0.f, 0.f }, 2);
FrameInfo fi_goblin(nullptr, { 0.f, 10.f / 20, 5.f / 15, 10.f / 15, 5.f / 15, 10.f / 15, 0.f, 0.f, 10.f / 15 }, 2);
FrameInfo fi_ork(nullptr, { 1.f, 12.f / 24, 7.f / 20, 13.f / 20, 7.f / 20, 13.f / 20, 0.f, 0.f, 15.f / 21 }, 2);
FrameInfo fi_wilk(nullptr, { 0.f, 0.f, 12.f / 17, 1.f, 13.f / 20, 15.f / 20, 0.f, 0.f, 0.f }, 2);
FrameInfo fi_szczur(nullptr, { 0.f, 0.f, 7.f / 15, 10.f / 15, 0.f, 0.f, 0.f, 0.f, 0.f }, 1);
FrameInfo fi_golem(nullptr, { 0.f, 0.f, 20.f / 40, 30.f / 40, 20.f / 40, 30.f / 40, 0.f, 0.f, 0.f }, 2);
FrameInfo fi_spider(nullptr, { 0.f, 0.f, 5.f / 20, 10.f / 20, 0.f, 0.f, 0.f, 0.f, 0.f }, 1);
FrameInfo fi_nieznany(nullptr, { 0.f, 0.f, 7.f / 13, 1.f, 6.f / 11, 1.f, 0.f, 0.f, 0.f }, 2);

//=================================================================================================
// TEKSTURY POSTACI
//=================================================================================================
vector<TexId> ti_zombie_rotting = {TexId(nullptr), TexId("glowa_zombie2.jpg"), TexId(nullptr)};
vector<TexId> ti_zombie_ancient = { TexId("zombif.jpg"), TexId("glowa4.jpg"), TexId(nullptr) };
vector<TexId> ti_worg = { TexId("wilk_czarny.jpg"), TexId(nullptr), TexId(nullptr) };
vector<TexId> ti_golem_iron = { TexId("golem2.jpg"), TexId(nullptr), TexId(nullptr) };
vector<TexId> ti_golem_adamantine = { TexId("golem3.jpg"), TexId(nullptr), TexId(nullptr) };
vector<TexId> ti_crazy_hunter = { TexId(nullptr), TexId("crazy_hunter.jpg"), TexId(nullptr) };
vector<TexId> ti_necromant = { TexId(nullptr), TexId("necromant.jpg"), TexId(nullptr) };
vector<TexId> ti_undead = { TexId(nullptr), TexId("undead.jpg"), TexId(nullptr) };
vector<TexId> ti_evil = { TexId(nullptr), TexId("evil.jpg"), TexId(nullptr) };
vector<TexId> ti_evil_boss = { TexId(nullptr), TexId("evil_boss.jpg"), TexId(nullptr) };
vector<TexId> ti_orc_shaman = { TexId("ork_szaman.jpg"), TexId(nullptr), TexId(nullptr) };

//=================================================================================================
// ANIMACJE IDLE
//=================================================================================================
vector<string> idle_zombie = { "wygina" };
vector<string> idle_szkielet = { "rozglada", "przysiad", "wygina" };
vector<string> idle_czlowiek = { "wygina", "rozglada", "drapie" };
vector<string> idle_goblin = { "rozglada" };
vector<string> idle_wilk = { "rozglada", "wygina" };
vector<string> idle_szczur = { "idle", "idle2" };
vector<string> idle_golem = { "idle" };
vector<string> idle_spider = { "idle" };
vector<string> idle_nieznany = { "idle" };

//=================================================================================================
// STATYSTYKI POSTACI
// hp_bonus to teraz bazowe hp (500 cz≥owiek, 600 ork, 400 goblin itp)
//=================================================================================================
UnitData g_base_units[] = {
	// id mesh material level skill_profile
	// flags flags2 flags3
	// hp+ def+ items spells gold gold2 dialog group
	// dmg_type walk_speed run_speed rot_speed blood
	// sounds frames tex &idles count width attack_range unit_armor_type
	//---- STARTOWE KLASY POSTACI ----
	UnitData("base_warrior", nullptr, MAT_BODY, INT2(0), StatProfileType::WARRIOR,
		F_HUMAN, 0, 0,
		500, 0, base_warrior_items, nullptr, INT2(85), INT2(85), nullptr, G_PLAYER,
		DMG_BLUNT, 1.5f, 5.f, 3.f, BLOOD_RED,
		&sounds_def, &fi_human, nullptr, &idle_czlowiek, 0.3f, 1.f, ArmorUnitType::HUMAN),
	UnitData("base_hunter", nullptr, MAT_BODY, INT2(0), StatProfileType::HUNTER,
		F_HUMAN, 0, 0,
		500, 0, base_hunter_items, nullptr, INT2(60), INT2(60), nullptr, G_PLAYER,
		DMG_BLUNT, 1.5f, 5.f, 3.f, BLOOD_RED,
		&sounds_def, &fi_human, nullptr, &idle_czlowiek, 0.3f, 1.f, ArmorUnitType::HUMAN),
	UnitData("base_rogue", nullptr, MAT_BODY, INT2(0), StatProfileType::ROGUE,
		F_HUMAN, F2_BACKSTAB, 0,
		500, 0, base_rogue_items, nullptr, INT2(100), INT2(100), nullptr, G_PLAYER,
		DMG_BLUNT, 1.5f, 5.f, 3.f, BLOOD_RED,
		&sounds_def, &fi_human, nullptr, &idle_czlowiek, 0.3f, 1.f, ArmorUnitType::HUMAN),

	//---- NPC ----
	UnitData("blacksmith", nullptr, MAT_BODY, INT2(5), StatProfileType::BLACKSMITH,
		F_HUMAN | F_AI_STAY, 0, F3_DONT_EAT,
		500, 0, blacksmith_items, nullptr, INT2(500,1000), INT2(500,1000), dialog_kowal, G_CITIZENS,
		DMG_BLUNT, 1.5f, 5.f, 3.f, BLOOD_RED,
		&sounds_def, &fi_human, nullptr, &idle_czlowiek, 0.3f, 1.f, ArmorUnitType::HUMAN),
	UnitData("merchant", nullptr, MAT_BODY, INT2(4), StatProfileType::MERCHANT,
		F_HUMAN | F_AI_GUARD | F_AI_STAY, 0, F3_DONT_EAT,
		500, 0, merchant_items, nullptr, INT2(500,1000), INT2(500,1000), dialog_kupiec, G_CITIZENS,
		DMG_BLUNT, 1.5f, 5.f, 3.f, BLOOD_RED,
		&sounds_def, &fi_human, nullptr, &idle_czlowiek, 0.3f, 1.f, ArmorUnitType::HUMAN),
	UnitData("food_seller", nullptr, MAT_BODY, INT2(4), StatProfileType::MERCHANT,
		F_HUMAN | F_AI_GUARD | F_AI_STAY, 0, F3_DONT_EAT,
		500, 0, merchant_items, nullptr, INT2(500,1000), INT2(500,1000), dialog_sprzedawca_jedzenia, G_CITIZENS,
		DMG_BLUNT, 1.5f, 5.f, 3.f, BLOOD_RED,
		&sounds_def, &fi_human, nullptr, &idle_czlowiek, 0.3f, 1.f, ArmorUnitType::HUMAN),
	UnitData("alchemist", nullptr, MAT_BODY, INT2(4), StatProfileType::ALCHEMIST,
		F_HUMAN | F_AI_STAY, 0, F3_DONT_EAT,
		500, 0, alchemist_items, nullptr, INT2(500,1000), INT2(500,1000), dialog_alchemik, G_CITIZENS,
		DMG_BLUNT, 1.5f, 5.f, 3.f, BLOOD_RED,
		&sounds_def, &fi_human, nullptr, &idle_czlowiek, 0.3f, 1.f, ArmorUnitType::HUMAN),
	UnitData("citizen", nullptr, MAT_BODY, INT2(1,5), StatProfileType::COMMONER,
		F_HUMAN | F_AI_WANDERS | F_AI_DRUNKMAN, 0, F3_CONTEST_25,
		500, 0, citizen_items, nullptr, INT2(10,50), INT2(20,60), dialog_mieszkaniec, G_CITIZENS,
		DMG_BLUNT, 1.5f, 5.f, 3.f, BLOOD_RED,
		&sounds_def, &fi_human, nullptr, &idle_czlowiek, 0.3f, 1.f, ArmorUnitType::HUMAN),
	UnitData("tut_czlowiek", nullptr, MAT_BODY, INT2(1,5), StatProfileType::COMMONER,
		F_HUMAN | F_AI_STAY, 0, 0,
		500, 0, citizen_items, nullptr, INT2(10,50), INT2(20,60), dialog_tut_czlowiek, G_CITIZENS,
		DMG_BLUNT, 1.5f, 5.f, 3.f, BLOOD_RED,
		&sounds_def, &fi_human, nullptr, &idle_czlowiek, 0.3f, 1.f, ArmorUnitType::HUMAN),
	UnitData("villager", nullptr, MAT_BODY, INT2(1,5), StatProfileType::COMMONER,
		F_HUMAN | F_AI_WANDERS | F_AI_DRUNKMAN, 0, F3_CONTEST_25,
		500, 0, citizen_items, nullptr, INT2(10,50), INT2(20,60), dialog_mieszkaniec, G_CITIZENS,
		DMG_BLUNT, 1.5f, 5.f, 3.f, BLOOD_RED,
		&sounds_def, &fi_human, nullptr, &idle_czlowiek, 0.3f, 1.f, ArmorUnitType::HUMAN),
	UnitData("mayor", nullptr, MAT_BODY, INT2(1,5), StatProfileType::CLERK,
		F_HUMAN | F_AI_CLERK | F_AI_STAY, 0, F3_DONT_EAT,
		500, 0, burmistrz_items, nullptr, INT2(100,500), INT2(200,600), dialog_burmistrz, G_CITIZENS,
		DMG_BLUNT, 1.5f, 5.f, 3.f, BLOOD_RED,
		&sounds_def, &fi_human, nullptr, &idle_czlowiek, 0.3f, 1.f, ArmorUnitType::HUMAN),
	UnitData("soltys", nullptr, MAT_BODY, INT2(1,5), StatProfileType::CLERK,
		F_HUMAN | F_AI_CLERK | F_AI_STAY, 0, F3_DONT_EAT,
		500, 0, burmistrz_items, nullptr, INT2(100,500), INT2(200,600), dialog_burmistrz, G_CITIZENS,
		DMG_BLUNT, 1.5f, 5.f, 3.f, BLOOD_RED,
		&sounds_def, &fi_human, nullptr, &idle_czlowiek, 0.3f, 1.f, ArmorUnitType::HUMAN),
	UnitData("trainer", nullptr, MAT_BODY, INT2(10,15),
		StatProfileType::FIGHTER,
		F_HUMAN | F_AI_GUARD | F_AI_STAY, 0, F3_DONT_EAT,
		500, 0, guard_items, nullptr, INT2(50,200), INT2(100,300), dialog_trener, G_CITIZENS,
		DMG_BLUNT, 1.5f, 5.f, 3.f, BLOOD_RED,
		&sounds_def, &fi_human, nullptr, &idle_czlowiek, 0.3f, 1.f, ArmorUnitType::HUMAN),
	UnitData("guard", nullptr, MAT_BODY, INT2(2,6), StatProfileType::FIGHTER,
		F_HUMAN | F_AI_GUARD | F_AI_STAY, F2_LIMITED_ROT, F3_DONT_EAT,
		500, 0, guard_items, nullptr, INT2(5,25), INT2(20,100), dialog_straznik, G_CITIZENS,
		DMG_BLUNT, 1.5f, 5.f, 3.f, BLOOD_RED,
		&sounds_def, &fi_human, nullptr, &idle_czlowiek, 0.3f, 1.f, ArmorUnitType::HUMAN),
	UnitData("guard2", nullptr, MAT_BODY, INT2(2,6), StatProfileType::FIGHTER,
		F_HUMAN, 0, F3_DONT_EAT,
		500, 0, guard_items, nullptr, INT2(5,25), INT2(20,100), dialog_straznicy_nagroda, G_CITIZENS,
		DMG_BLUNT, 1.5f, 5.f, 3.f, BLOOD_RED,
		&sounds_def, &fi_human, nullptr, &idle_czlowiek, 0.3f, 1.f, ArmorUnitType::HUMAN),
	UnitData("guard3", nullptr, MAT_BODY, INT2(2,6), StatProfileType::FIGHTER,
		F_HUMAN, 0, F3_DONT_EAT,
		500, 0, guard_items, nullptr, INT2(5,25), INT2(20,100), dialog_straznik, G_CITIZENS,
		DMG_BLUNT, 1.5f, 5.f, 3.f, BLOOD_RED,
		&sounds_def, &fi_human, nullptr, &idle_czlowiek, 0.3f, 1.f, ArmorUnitType::HUMAN),
	UnitData("guard_move", nullptr, MAT_BODY, INT2(2,6), StatProfileType::FIGHTER,
		F_HUMAN | F_AI_WANDERS, F2_AI_TRAIN, F3_DONT_EAT,
		500, 0, guard_items, nullptr, INT2(5,25), INT2(20,100), dialog_straznik, G_CITIZENS,
		DMG_BLUNT, 1.5f, 5.f, 3.f, BLOOD_RED,
		&sounds_def, &fi_human, nullptr, &idle_czlowiek, 0.3f, 1.f, ArmorUnitType::HUMAN),
	UnitData("guard_captain", nullptr, MAT_BODY, INT2(6,12), StatProfileType::FIGHTER,
		F_HUMAN | F_AI_GUARD | F_AI_STAY, 0, F3_DONT_EAT,
		500, 0, guard_items, nullptr, INT2(40,80), INT2(80,160), dialog_dowodca_strazy, G_CITIZENS,
		DMG_BLUNT, 1.5f, 5.f, 3.f, BLOOD_RED,
		&sounds_def, &fi_human, nullptr, &idle_czlowiek, 0.3f, 1.f, ArmorUnitType::HUMAN),
	UnitData("clerk", nullptr, MAT_BODY, INT2(1,5), StatProfileType::CLERK,
		F_HUMAN | F_AI_CLERK | F_AI_STAY, 0, F3_DONT_EAT,
		500, 0, citizen_items, nullptr, INT2(50,200), INT2(100,400), dialog_urzednik, G_CITIZENS,
		DMG_BLUNT, 1.5f, 5.f, 3.f, BLOOD_RED,
		&sounds_def, &fi_human, nullptr, &idle_czlowiek, 0.3f, 1.f, ArmorUnitType::HUMAN),
	UnitData("innkeeper", nullptr, MAT_BODY, INT2(4), StatProfileType::MERCHANT,
		F_HUMAN | F_AI_GUARD, F2_LIMITED_ROT, F3_DONT_EAT | F3_TALK_AT_COMPETITION,
		500, 0, karczmarz_items, nullptr, INT2(500,1000), INT2(500,1000), dialog_karczmarz, G_CITIZENS,
		DMG_BLUNT, 1.5f, 5.f, 3.f, BLOOD_RED,
		&sounds_def, &fi_human, nullptr, &idle_czlowiek, 0.3f, 1.f, ArmorUnitType::HUMAN),
	UnitData("arena_master", nullptr, MAT_BODY, INT2(10,15), StatProfileType::WARRIOR,
		F_HUMAN | F_AI_GUARD | F_AI_STAY, F2_LIMITED_ROT, F3_DONT_EAT | F3_TALK_AT_COMPETITION,
		500, 0, guard_items, nullptr, INT2(50,200), INT2(100,300), dialog_mistrz_areny, G_CITIZENS,
		DMG_BLUNT, 1.5f, 5.f, 3.f, BLOOD_RED,
		&sounds_def, &fi_human, nullptr, &idle_czlowiek, 0.3f, 1.f, ArmorUnitType::HUMAN),
	UnitData("viewer", nullptr, MAT_BODY, INT2(1,5), StatProfileType::COMMONER,
		F_HUMAN | F_AI_GUARD | F_AI_STAY, F2_LIMITED_ROT, F3_DONT_EAT,
		500, 0, citizen_items, nullptr, INT2(10,50), INT2(20,60), dialog_widz, G_CITIZENS,
		DMG_BLUNT, 1.5f, 5.f, 3.f, BLOOD_RED,
		&sounds_def, &fi_human, nullptr, &idle_czlowiek, 0.3f, 1.f, ArmorUnitType::HUMAN),
	UnitData("captive", nullptr, MAT_BODY, INT2(1,5), StatProfileType::COMMONER,
		F_HUMAN | F_AI_WANDERS | F_HERO, F2_SPECIFIC_NAME | F2_NO_CLASS, 0,
		500, 0, captive_items, nullptr, INT2(10,50), INT2(20,60), dialog_porwany, G_CITIZENS,
		DMG_BLUNT, 1.5f, 5.f, 3.f, BLOOD_RED,
		&sounds_def, &fi_human, nullptr, &idle_czlowiek, 0.3f, 1.f, ArmorUnitType::HUMAN),
	UnitData("traveler", nullptr, MAT_BODY, INT2(2,6), StatProfileType::COMMONER,
		F_HUMAN | F_AI_WANDERS, 0, F3_CONTEST_25,
		500, 0, podrozny_items, nullptr, INT2(50,100), INT2(100,200), dialog_zadanie, G_CITIZENS,
		DMG_BLUNT, 1.5f, 5.f, 3.f, BLOOD_RED,
		&sounds_def, &fi_human, nullptr, &idle_czlowiek, 0.3f, 1.f, ArmorUnitType::HUMAN),
	UnitData("artur_drwal", nullptr, MAT_BODY, INT2(5), StatProfileType::WORKER,
		F_HUMAN | F_AI_STAY | F_HERO, F2_SPECIFIC_NAME | F2_NO_CLASS | F2_CONTEST_50, 0,
		500, 0, artur_drwal_items, nullptr, INT2(500,1000), INT2(500,1000), dialog_artur_drwal, G_CITIZENS,
		DMG_BLUNT, 1.5f, 5.f, 3.f, BLOOD_RED,
		&sounds_def, &fi_human, nullptr, &idle_czlowiek, 0.3f, 1.f, ArmorUnitType::HUMAN),
	UnitData("poslaniec_tartak", nullptr, MAT_BODY, INT2(1,5), StatProfileType::COMMONER,
		F_HUMAN | F_AI_WANDERS, 0, 0,
		500, 0, poslaniec_items, nullptr, INT2(10,50), INT2(20,60), dialog_artur_drwal, G_CITIZENS,
		DMG_BLUNT, 1.5f, 5.f, 3.f, BLOOD_RED,
		&sounds_def, &fi_human, nullptr, &idle_czlowiek, 0.3f, 1.f, ArmorUnitType::HUMAN),
	UnitData("drwal", nullptr, MAT_BODY, INT2(1,5), StatProfileType::WORKER,
		F_HUMAN, 0, 0,
		500, 0, drwal_items, nullptr, INT2(20,50), INT2(30,60), dialog_drwal, G_CITIZENS,
		DMG_BLUNT, 1.5f, 5.f, 3.f, BLOOD_RED,
		&sounds_def, &fi_human, nullptr, &idle_czlowiek, 0.3f, 1.f, ArmorUnitType::HUMAN),
	UnitData("inwestor", nullptr, MAT_BODY, INT2(10), StatProfileType::CLERK,
		F_HUMAN | F_AI_STAY | F_HERO, F2_SPECIFIC_NAME | F2_NO_CLASS, 0,
		500, 0, inwestor_items, nullptr, INT2(1000,1200), INT2(1000,1200), dialog_inwestor, G_CITIZENS,
		DMG_BLUNT, 1.5f, 5.f, 3.f, BLOOD_RED,
		&sounds_def, &fi_human, nullptr, &idle_czlowiek, 0.3f, 1.f, ArmorUnitType::HUMAN),
	UnitData("poslaniec_kopalnia", nullptr, MAT_BODY, INT2(1,5), StatProfileType::COMMONER,
		F_HUMAN | F_AI_WANDERS, 0, 0,
		500, 0, poslaniec_items, nullptr, INT2(10,50), INT2(20,60), dialog_inwestor, G_CITIZENS,
		DMG_BLUNT, 1.5f, 5.f, 3.f, BLOOD_RED,
		&sounds_def, &fi_human, nullptr, &idle_czlowiek, 0.3f, 1.f, ArmorUnitType::HUMAN),
	UnitData("gornik", nullptr, MAT_BODY, INT2(1,5), StatProfileType::WORKER,
		F_HUMAN, F2_UPDATE_V0_ITEMS, F3_MINER,
		500, 0, gornik_items, nullptr, INT2(20,50), INT2(30,60), dialog_gornik, G_CITIZENS,
		DMG_BLUNT, 1.5f, 5.f, 3.f, BLOOD_RED,
		&sounds_def, &fi_human, nullptr, &idle_czlowiek, 0.3f, 1.f, ArmorUnitType::HUMAN),
	UnitData("gornik_szef", nullptr, MAT_BODY, INT2(5), StatProfileType::WORKER,
		F_HUMAN | F_AI_STAY, F2_UPDATE_V0_ITEMS, 0,
		500, 0, gornik_items, nullptr, INT2(50,100), INT2(50,100), dialog_inwestor, G_CITIZENS,
		DMG_BLUNT, 1.5f, 5.f, 3.f, BLOOD_RED,
		&sounds_def, &fi_human, nullptr, &idle_czlowiek, 0.3f, 1.f, ArmorUnitType::HUMAN),
	UnitData("pijak", nullptr, MAT_BODY, INT2(2,20), StatProfileType::COMMONER,
		F_HUMAN, F2_CONTEST, F3_DRUNKMAN_AFTER_CONTEST,
		500, 0, poslaniec_items, nullptr, INT2(8,22), INT2(80,220), dialog_pijak, G_CITIZENS,
		DMG_BLUNT, 1.5f, 5.f, 3.f, BLOOD_RED,
		&sounds_def, &fi_human, nullptr, &idle_czlowiek, 0.3f, 1.f, ArmorUnitType::HUMAN),
	UnitData("mistrz_agentow", nullptr, MAT_BODY, INT2(10,15), StatProfileType::ROGUE,
		F_HUMAN | F_AI_STAY | F_HERO, F2_SPECIFIC_NAME | F2_NO_CLASS | F2_BACKSTAB, F3_DONT_EAT,
		500, 0, rogue_items, nullptr, INT2(50,200), INT2(100,300), dialog_q_bandyci, G_CITIZENS,
		DMG_BLUNT, 1.5f, 5.f, 3.f, BLOOD_RED,
		&sounds_def, &fi_human, nullptr, &idle_czlowiek, 0.3f, 1.f, ArmorUnitType::HUMAN),
	UnitData("guard_q_bandyci", nullptr, MAT_BODY, INT2(2,6), StatProfileType::FIGHTER,
		F_HUMAN | F_AI_WANDERS, F2_AI_TRAIN, F3_DONT_EAT,
		500, 0, q_guard_items, nullptr, INT2(5, 25), INT2(20, 100), dialog_q_bandyci, G_CITIZENS,
		DMG_BLUNT, 1.5f, 5.f, 3.f, BLOOD_RED,
		&sounds_def, &fi_human, nullptr, &idle_czlowiek, 0.3f, 1.f, ArmorUnitType::HUMAN),
	UnitData("agent", nullptr, MAT_BODY, INT2(6,12), StatProfileType::ROGUE,
		F_HUMAN | F_AI_STAY | F_HERO, F2_NO_CLASS | F2_SPECIFIC_NAME, F3_DONT_EAT,
		500, 0, rogue_items, nullptr, INT2(10,50), INT2(20,60), dialog_q_bandyci, G_CITIZENS,
		DMG_BLUNT, 1.5f, 5.f, 3.f, BLOOD_RED,
		&sounds_def, &fi_human, nullptr, &idle_czlowiek, 0.3f, 1.f, ArmorUnitType::HUMAN),
	UnitData("q_magowie_uczony", nullptr, MAT_BODY, INT2(1,5), StatProfileType::MAGE,
		F_HUMAN | F_AI_STAY, 0, 0,
		500, 0, burmistrz_items, nullptr, INT2(100,500), INT2(200,600), dialog_q_magowie, G_CITIZENS,
		DMG_BLUNT, 1.5f, 5.f, 3.f, BLOOD_RED,
		&sounds_def, &fi_human, nullptr, &idle_czlowiek, 0.3f, 1.f, ArmorUnitType::HUMAN),
	UnitData("q_magowie_stary", nullptr, MAT_BODY, INT2(3,15), StatProfileType::MAGE,
		F_HUMAN | F_MAGE | F_HERO, F2_OLD | F2_CLASS_FLAG, F3_DRUNK_MAGE,
		500, 0, mage_items, &mage_spells, INT2(15,30), INT2(150,300), dialog_q_magowie2, G_CITIZENS,
		DMG_BLUNT, 1.5f, 5.f, 3.f, BLOOD_RED,
		&sounds_def, &fi_human, nullptr, &idle_czlowiek, 0.3f, 1.f, ArmorUnitType::HUMAN),
	UnitData("q_orkowie_straznik", nullptr, MAT_BODY, INT2(2,6), StatProfileType::FIGHTER,
		F_HUMAN | F_AI_WANDERS, F2_AI_TRAIN, F3_DONT_EAT,
		500, 0, guard_items, nullptr, INT2(5,25), INT2(20,100), dialog_q_orkowie, G_CITIZENS,
		DMG_BLUNT, 1.5f, 5.f, 3.f, BLOOD_RED,
		&sounds_def, &fi_human, nullptr, &idle_czlowiek, 0.3f, 1.f, ArmorUnitType::HUMAN),
	UnitData("q_orkowie_gorush", "ork.qmsh", MAT_BODY, INT2(5), StatProfileType::ORC,
		F_HUMANOID | F_HERO, F2_AI_TRAIN | F2_SPECIFIC_NAME | F2_CLASS_FLAG | F2_WARRIOR | F2_CONTEST_50 | F2_ORC_SOUNDS | F2_TOURNAMENT, F3_ORC_FOOD,
		600, 5, gorush_items, nullptr, INT2(5,20), INT2(10,30), dialog_q_orkowie2, G_CITIZENS,
		DMG_BLUNT, 1.5f, 5.f, 3.f, BLOOD_RED,
		&sounds_orc, &fi_ork, nullptr, &idle_goblin, 0.41f, 1.f, ArmorUnitType::ORC),
	UnitData("q_orkowie_gorush_woj", "ork.qmsh", MAT_BODY, INT2(10), StatProfileType::GORUSH_WARRIOR,
		F_HUMANOID | F_HERO, F2_AI_TRAIN | F2_SPECIFIC_NAME | F2_CLASS_FLAG | F2_WARRIOR | F2_CONTEST | F2_MELEE | F2_SIT_ON_THRONE | F2_ORC_SOUNDS | F2_TOURNAMENT, F3_ORC_FOOD,
		600, 15, gorush_woj_items, nullptr, INT2(5,20), INT2(10,30), dialog_q_orkowie2, G_CITIZENS,
		DMG_BLUNT, 1.5f, 5.f, 3.f, BLOOD_RED,
		&sounds_orc, &fi_ork, nullptr, &idle_goblin, 0.41f, 1.f, ArmorUnitType::ORC),
	UnitData("q_orkowie_gorush_lowca", "ork.qmsh", MAT_BODY, INT2(10), StatProfileType::GORUSH_HUNTER,
		F_HUMANOID | F_HERO | F_ARCHER, F2_AI_TRAIN | F2_SPECIFIC_NAME | F2_CLASS_FLAG | F2_HUNTER | F2_CONTEST_50 | F2_SIT_ON_THRONE | F2_ORC_SOUNDS | F2_TOURNAMENT, F3_ORC_FOOD,
		600, 10, gorush_lowca_items, nullptr, INT2(5,20), INT2(10,30), dialog_q_orkowie2, G_CITIZENS,
		DMG_BLUNT, 1.5f, 5.f, 3.f, BLOOD_RED,
		&sounds_orc, &fi_ork, nullptr, &idle_goblin, 0.41f, 1.f, ArmorUnitType::ORC),
	UnitData("q_orkowie_gorush_szaman", "ork.qmsh", MAT_BODY, INT2(10), StatProfileType::SHAMAN,
		F_HUMANOID | F_HERO | F_MAGE, F2_AI_TRAIN | F2_SPECIFIC_NAME | F2_CLASS_FLAG | F2_SIT_ON_THRONE | F2_ORC_SOUNDS | F2_TOURNAMENT, F3_ORC_FOOD,
		600, 5, gorush_szaman_items, &orc_shaman_spells, INT2(5,20), INT2(10,30), dialog_q_orkowie2, G_CITIZENS,
		DMG_BLUNT, 1.5f, 5.f, 3.f, BLOOD_RED,
		&sounds_orc, &fi_ork, &ti_orc_shaman, &idle_goblin, 0.41f, 1.f, ArmorUnitType::ORC),
	UnitData("q_orkowie_slaby", "ork.qmsh", MAT_BODY, INT2(0), StatProfileType::ORC,
		F_HUMANOID | F_COWARD, F2_ORC_SOUNDS, F3_ORC_FOOD,
		600, 5, orkowie_slaby_items, nullptr, INT2(5,10), INT2(5,10), dialog_q_orkowie2, G_CITIZENS,
		DMG_BLUNT, 1.5f, 5.f, 3.f, BLOOD_RED,
		&sounds_orc, &fi_ork, nullptr, &idle_goblin, 0.41f, 1.f, ArmorUnitType::ORC),
	UnitData("q_orkowie_kowal", "ork.qmsh", MAT_BODY, INT2(5,9), StatProfileType::ORC_BLACKSMITH,
		F_HUMANOID | F_AI_STAY, F2_ORC_SOUNDS, F3_ORC_FOOD,
		600, 10, orc_fighter_items, nullptr, INT2(100,200), INT2(100,200), dialog_q_orkowie2, G_CITIZENS,
		DMG_BLUNT, 1.5f, 5.f, 3.f, BLOOD_RED,
		&sounds_orc, &fi_ork, nullptr, &idle_goblin, 0.41f, 1.f, ArmorUnitType::ORC),
	UnitData("q_orkowie_orc", "ork.qmsh", MAT_BODY, INT2(1,4), StatProfileType::ORC,
		F_HUMANOID, F2_AI_TRAIN | F2_ORC_SOUNDS, F3_ORC_FOOD,
		600, 5, orc_items, nullptr, INT2(5,20), INT2(10,30), dialog_q_orkowie2, G_CITIZENS,
		DMG_BLUNT, 1.5f, 5.f, 3.f, BLOOD_RED,
		&sounds_orc, &fi_ork, nullptr, &idle_goblin, 0.41f, 1.f, ArmorUnitType::ORC),
	UnitData("q_orkowie_orc_hunter", "ork.qmsh", MAT_BODY, INT2(5,9), StatProfileType::ORC,
		F_HUMANOID | F_ARCHER, F2_AI_TRAIN | F2_ORC_SOUNDS, F3_ORC_FOOD,
		600, 10, orc_hunter_items, nullptr, INT2(12,32), INT2(25,50), dialog_q_orkowie2, G_CITIZENS,
		DMG_BLUNT, 1.5f, 5.f, 3.f, BLOOD_RED,
		&sounds_orc, &fi_ork, nullptr, &idle_goblin, 0.41f, 1.f, ArmorUnitType::ORC),
	UnitData("q_orkowie_orc_fighter", "ork.qmsh", MAT_BODY, INT2(5,9), StatProfileType::ORC,
		F_HUMANOID, F2_AI_TRAIN | F2_ORC_SOUNDS | F2_MELEE, F3_ORC_FOOD,
		600, 10, orc_fighter_items, nullptr, INT2(12,32), INT2(25,50), dialog_q_orkowie2, G_CITIZENS,
		DMG_BLUNT, 1.5f, 5.f, 3.f, BLOOD_RED,
		&sounds_orc, &fi_ork, nullptr, &idle_goblin, 0.41f, 1.f, ArmorUnitType::ORC),
	UnitData("q_orkowie_orc_chief", "ork.qmsh", MAT_BODY, INT2(10,15), StatProfileType::ORC,
		F_HUMANOID | F_LEADER, F2_ORC_SOUNDS | F2_MELEE, F3_ORC_FOOD,
		600, 15, orc_chief_items, nullptr, INT2(50,100), INT2(100,200), dialog_q_orkowie2, G_CITIZENS,
		DMG_BLUNT, 1.5f, 5.f, 3.f, BLOOD_RED,
		&sounds_orc, &fi_ork, nullptr, &idle_goblin, 0.41f, 1.f, ArmorUnitType::ORC),
	UnitData("q_orkowie_orc_shaman", "ork.qmsh", MAT_BODY, INT2(6,11), StatProfileType::SHAMAN,
		F_HUMANOID | F_MAGE, F2_ORC_SOUNDS, F3_ORC_FOOD,
		600, 5, orc_shaman_items, &orc_shaman_spells, INT2(20,40), INT2(30,60), dialog_q_orkowie2, G_CITIZENS,
		DMG_BLUNT, 1.5f, 5.f, 3.f, BLOOD_RED,
		&sounds_orc, &fi_ork, &ti_orc_shaman, &idle_goblin, 0.41f, 1.f, ArmorUnitType::ORC),
	UnitData("q_gobliny_szlachcic", nullptr, MAT_BODY, INT2(18), StatProfileType::ROGUE,
		F_HUMAN | F_AI_STAY | F_HERO, F2_SPECIFIC_NAME | F2_NO_CLASS, 0,
		500, 0, burmistrz_items, nullptr, INT2(250,500), INT2(250,500), dialog_q_gobliny, G_CITIZENS,
		DMG_BLUNT, 1.5f, 5.f, 3.f, BLOOD_RED,
		&sounds_def, &fi_human, nullptr, &idle_czlowiek, 0.3f, 1.f, ArmorUnitType::HUMAN),
	UnitData("q_gobliny_poslaniec", nullptr, MAT_BODY, INT2(1,5), StatProfileType::COMMONER,
		F_HUMAN | F_AI_WANDERS, 0, 0,
		500, 0, poslaniec_items, nullptr, INT2(10,50), INT2(20,60), dialog_q_gobliny, G_CITIZENS,
		DMG_BLUNT, 1.5f, 5.f, 3.f, BLOOD_RED,
		&sounds_def, &fi_human, nullptr, &idle_czlowiek, 0.3f, 1.f, ArmorUnitType::HUMAN),
	UnitData("q_gobliny_mag",nullptr, MAT_BODY, INT2(3,15), StatProfileType::MAGE,
		F_HUMAN | F_MAGE, 0, 0,
		500, 0, mage_items, &mage_spells, INT2(15,30), INT2(150,300), dialog_q_gobliny, G_CITIZENS,
		DMG_BLUNT, 1.5f, 5.f, 3.f, BLOOD_RED,
		&sounds_def, &fi_human, nullptr, &idle_czlowiek, 0.3f, 1.f, ArmorUnitType::HUMAN),
	UnitData("q_zlo_kaplan", nullptr, MAT_BODY, INT2(0,20), StatProfileType::CLERIC,
		F_HUMAN | F_HERO | F_AI_STAY, F2_AI_TRAIN | F2_CLASS_FLAG | F2_SPECIFIC_NAME | F2_CLERIC, 0,
		500, 0, kaplan_items, &jozan_spells, INT2(8,22), INT2(80,220), dialog_q_zlo, G_CITIZENS,
		DMG_BLUNT, 1.5f, 5.f, 3.f, BLOOD_RED,
		&sounds_def, &fi_human, nullptr, &idle_czlowiek, 0.3f, 1.f, ArmorUnitType::HUMAN),
	UnitData("q_zlo_mag", nullptr, MAT_BODY, INT2(3,15), StatProfileType::MAGE,
		F_HUMAN | F_MAGE, 0, 0,
		500, 0, mage_items, &mage_spells, INT2(15,30), INT2(150,300), dialog_q_zlo, G_CITIZENS,
		DMG_BLUNT, 1.5f, 5.f, 3.f, BLOOD_RED,
		&sounds_def, &fi_human, nullptr, &idle_czlowiek, 0.3f, 1.f, ArmorUnitType::HUMAN),

	//---- GOBLINY ----
	UnitData("goblin", "goblin.qmsh", MAT_BODY, INT2(1,3), StatProfileType::GOBLIN,
		F_HUMANOID | F_COWARD, F2_AI_TRAIN | F2_GOBLIN_SOUNDS, F3_ORC_FOOD,
		400, 0, goblin_items, nullptr, INT2(1,10), INT2(3,14), nullptr, G_GOBLINS,
		DMG_BLUNT, 1.75f, 6.f, 3.5f, BLOOD_RED,
		&sounds_goblin, &fi_goblin, nullptr, &idle_goblin, 0.3f, 0.8f, ArmorUnitType::GOBLIN),
	UnitData("goblin_hunter", "goblin.qmsh", MAT_BODY, INT2(4,6), StatProfileType::GOBLIN,
		F_HUMANOID | F_COWARD | F_ARCHER, F2_AI_TRAIN | F2_GOBLIN_SOUNDS, F3_ORC_FOOD,
		400, 0, goblin_hunter_items, nullptr, INT2(4,16), INT2(6,20), nullptr, G_GOBLINS,
		DMG_BLUNT, 1.75f, 6.f, 3.5f, BLOOD_RED,
		&sounds_goblin, &fi_goblin, nullptr, &idle_goblin, 0.3f, 0.8f, ArmorUnitType::GOBLIN),
	UnitData("goblin_fighter", "goblin.qmsh", MAT_BODY, INT2(4,6), StatProfileType::GOBLIN,
		F_HUMANOID | F_COWARD, F2_AI_TRAIN | F2_GOBLIN_SOUNDS, F3_ORC_FOOD,
		400, 0, goblin_fighter_items, nullptr, INT2(4,16), INT2(6,20), nullptr, G_GOBLINS,
		DMG_BLUNT, 1.75f, 6.f, 3.5f, BLOOD_RED,
		&sounds_goblin, &fi_goblin, nullptr, &idle_goblin, 0.3f, 0.8f, ArmorUnitType::GOBLIN),
	UnitData("goblin_chief", "goblin.qmsh", MAT_BODY, INT2(7,10), StatProfileType::GOBLIN,
		F_HUMANOID | F_COWARD | F_LEADER, F2_GOBLIN_SOUNDS, F3_ORC_FOOD,
		400, 0, goblin_chief_items, nullptr, INT2(20,50), INT2(50,100), nullptr, G_GOBLINS,
		DMG_BLUNT, 1.75f, 6.f, 3.5f, BLOOD_RED,
		&sounds_goblin, &fi_goblin, nullptr, &idle_goblin, 0.3f, 0.8f, ArmorUnitType::GOBLIN),
	UnitData("goblin_chief_q", "goblin.qmsh", MAT_BODY, INT2(7,10), StatProfileType::GOBLIN,
		F_HUMANOID | F_COWARD | F_LEADER, F2_GOBLIN_SOUNDS | F2_MARK, F3_ORC_FOOD,
		400, 0, goblin_chief_items, nullptr, INT2(20,50), INT2(50,100), nullptr, G_GOBLINS,
		DMG_BLUNT, 1.75f, 6.f, 3.5f, BLOOD_RED,
		&sounds_goblin, &fi_goblin, nullptr, &idle_goblin, 0.3f, 0.8f, ArmorUnitType::GOBLIN),
	UnitData("q_gobliny_szlachcic2", nullptr, MAT_BODY, INT2(18), StatProfileType::ROGUE,
		F_HUMAN | F_ARCHER | F_HERO | F_AI_WANDERS | F_LEADER, F2_SPECIFIC_NAME | F2_NO_CLASS | F2_BOSS | F2_SIT_ON_THRONE | F2_MARK | F2_NOT_GOBLIN, 0,
		600, 25, szlachcic_items, nullptr, INT2(1000,2000), INT2(1000,2000), dialog_q_gobliny, G_GOBLINS,
		DMG_BLUNT, 1.5f, 5.f, 3.f, BLOOD_RED,
		&sounds_def, &fi_human, nullptr, &idle_czlowiek, 0.3f, 1.f, ArmorUnitType::HUMAN),
	UnitData("q_gobliny_ochroniarz", nullptr, MAT_BODY, INT2(5,10), StatProfileType::FIGHTER,
		F_HUMAN, F2_AI_TRAIN | F2_NOT_GOBLIN, 0,
		500, 0, guard_items, nullptr, INT2(10,20), INT2(50,100), dialog_ochroniarz, G_GOBLINS,
		DMG_BLUNT, 1.5f, 5.f, 3.f, BLOOD_RED,
		&sounds_def, &fi_human, nullptr, &idle_czlowiek, 0.3f, 1.f, ArmorUnitType::HUMAN),
	UnitData("tut_goblin", "goblin.qmsh", MAT_BODY, INT2(1,3), StatProfileType::GOBLIN,
		F_HUMANOID | F_COWARD | F_AI_STAY | F_AI_GUARD, F2_GOBLIN_SOUNDS | F2_MARK, F3_ORC_FOOD,
		400, 0, tut_goblin_items, nullptr, INT2(1,10), INT2(3,14), nullptr, G_GOBLINS,
		DMG_BLUNT, 1.75f, 6.f, 3.5f, BLOOD_RED,
		&sounds_goblin, &fi_goblin, nullptr, &idle_goblin, 0.3f, 0.8f, ArmorUnitType::GOBLIN),

	//---- ORKOWIE ----
	UnitData("orc", "ork.qmsh", MAT_BODY, INT2(1,4), StatProfileType::ORC,
		F_HUMANOID, F2_AI_TRAIN | F2_ORC_SOUNDS, F3_ORC_FOOD,
		600, 5, orc_items, nullptr, INT2(5,20), INT2(10,30), nullptr, G_ORCS,
		DMG_BLUNT, 1.5f, 5.f, 3.f, BLOOD_RED,
		&sounds_orc, &fi_ork, nullptr, &idle_goblin, 0.41f, 1.f, ArmorUnitType::ORC),
	UnitData("orc_hunter", "ork.qmsh", MAT_BODY, INT2(5,9), StatProfileType::ORC,
		F_HUMANOID | F_ARCHER, F2_AI_TRAIN | F2_ORC_SOUNDS, F3_ORC_FOOD,
		600, 10, orc_hunter_items, nullptr, INT2(12,32), INT2(25,50), nullptr, G_ORCS,
		DMG_BLUNT, 1.5f, 5.f, 3.f, BLOOD_RED,
		&sounds_orc, &fi_ork, nullptr, &idle_goblin, 0.41f, 1.f, ArmorUnitType::ORC),
	UnitData("orc_fighter", "ork.qmsh", MAT_BODY, INT2(5,9), StatProfileType::ORC,
		F_HUMANOID, F2_AI_TRAIN | F2_MELEE | F2_ORC_SOUNDS, F3_ORC_FOOD,
		600, 10, orc_fighter_items, nullptr, INT2(12,32), INT2(25,50), nullptr, G_ORCS,
		DMG_BLUNT, 1.5f, 5.f, 3.f, BLOOD_RED,
		&sounds_orc, &fi_ork, nullptr, &idle_goblin, 0.41f, 1.f, ArmorUnitType::ORC),
	UnitData("orc_chief", "ork.qmsh", MAT_BODY, INT2(10,15), StatProfileType::ORC,
		F_HUMANOID | F_LEADER, F2_MELEE | F2_ORC_SOUNDS, F3_ORC_FOOD,
		600, 15, orc_chief_items, nullptr, INT2(50,100), INT2(100,200), nullptr, G_ORCS,
		DMG_BLUNT, 1.5f, 5.f, 3.f, BLOOD_RED,
		&sounds_orc, &fi_ork, nullptr, &idle_goblin, 0.41f, 1.f, ArmorUnitType::ORC),
	UnitData("orc_chief_q", "ork.qmsh", MAT_BODY, INT2(10,15), StatProfileType::ORC,
		F_HUMANOID | F_LEADER, F2_MELEE | F2_ORC_SOUNDS | F2_MARK, F3_ORC_FOOD,
		600, 15, orc_chief_items, nullptr, INT2(50,100), INT2(100,200), nullptr, G_ORCS,
		DMG_BLUNT, 1.5f, 5.f, 3.f, BLOOD_RED,
		&sounds_orc, &fi_ork, nullptr, &idle_goblin, 0.41f, 1.f, ArmorUnitType::ORC),
	UnitData("orc_shaman", "ork.qmsh", MAT_BODY, INT2(6,11), StatProfileType::SHAMAN,
		F_HUMANOID | F_MAGE, F2_ORC_SOUNDS, F3_ORC_FOOD,
		600, 5, orc_shaman_items, &orc_shaman_spells, INT2(20,40), INT2(30,60), nullptr, G_ORCS,
		DMG_BLUNT, 1.5f, 5.f, 3.f, BLOOD_RED,
		&sounds_orc, &fi_ork, &ti_orc_shaman, &idle_goblin, 0.41f, 1.f, ArmorUnitType::ORC),
	UnitData("q_orkowie_boss", "ork.qmsh", MAT_BODY, INT2(18), StatProfileType::ORC,
		F_HUMANOID | F_LEADER, F2_MELEE | F2_BOSS | F2_SIT_ON_THRONE | F2_ORC_SOUNDS | F2_YELL | F2_MARK | F2_GUARDED, F3_ORC_FOOD,
		800, 30, orkowie_boss_items, nullptr, INT2(1000,2000), INT2(1000,2000), nullptr, G_ORCS,
		DMG_BLUNT, 1.5f, 5.f, 3.f, BLOOD_RED,
		&sounds_orc_boss, &fi_ork, nullptr, &idle_goblin, 0.41f, 1.f, ArmorUnitType::ORC),

	//---- NIEUMARLI I Z£O ----
	UnitData("zombie", "zombie.qmsh", MAT_BODY, INT2(2,4), StatProfileType::ZOMBIE,
		F_UNDEAD | F_DONT_ESCAPE | F_SLOW | F_PIERCE_RES25 | F_BLUNT_RES25 | F_DONT_OPEN | F_POISON_RES | F_NO_POWER_ATTACK, F2_IGNORE_BLOCK | F2_BACKSTAB_RES, F3_DONT_EAT,
		800, 25, nullptr, nullptr, INT2(0,0), INT2(0,0), nullptr, G_UNDEAD,
		DMG_BLUNT, 1.5f, 0.f, 2.5f, BLOOD_GREEN,
		&sounds_zombie, &fi_zombie, nullptr, &idle_zombie, 0.3f, 1.f, ArmorUnitType::NONE),
	UnitData("zombie_rotting", "zombie.qmsh", MAT_BODY, INT2(4,8), StatProfileType::ZOMBIE,
		F_UNDEAD | F_DONT_ESCAPE | F_SLOW | F_PIERCE_RES25 | F_BLUNT_RES25 | F_DONT_OPEN | F_POISON_RES | F_NO_POWER_ATTACK, F2_IGNORE_BLOCK | F2_BACKSTAB_RES, F3_DONT_EAT,
		800, 50, nullptr, &zombie_rotting_spells, INT2(0,0), INT2(0,0), nullptr, G_UNDEAD,
		DMG_BLUNT, 1.5f, 0.f, 2.5f, BLOOD_GREEN,
		&sounds_zombie, &fi_zombie, &ti_zombie_rotting, &idle_zombie, 0.3f, 1.f, ArmorUnitType::NONE),
	UnitData("zombie_ancient", "zombie.qmsh", MAT_BODY, INT2(8,12), StatProfileType::ZOMBIE,
		F_UNDEAD | F_DONT_ESCAPE | F_SLOW | F_PIERCE_RES25 | F_BLUNT_RES25 | F_DONT_OPEN | F_POISON_RES | F_NO_POWER_ATTACK, F2_IGNORE_BLOCK | F2_BACKSTAB_RES | F2_MAGIC_RES25, F3_DONT_EAT,
		800, 75, nullptr, nullptr, INT2(0,0), INT2(0,0), nullptr, G_UNDEAD,
		DMG_BLUNT, 1.3f, 0.f, 2.2f, BLOOD_BLACK,
		&sounds_zombie, &fi_zombie, &ti_zombie_ancient, &idle_zombie, 0.3f, 1.f, ArmorUnitType::NONE),
	//
	UnitData("skeleton", "skeleton.qmsh", MAT_BONE, INT2(2,4), StatProfileType::SKELETON,
		F_HUMANOID | F_UNDEAD | F_DONT_ESCAPE | F_PIERCE_RES25 | F_BLUNT_WEAK25 | F_POISON_RES, F2_BLOODLESS | F2_BACKSTAB_RES, F3_DONT_EAT,
		450, 20, skeleton_items, nullptr, INT2(0,0), INT2(0,0), nullptr, G_UNDEAD,
		DMG_SLASH, 1.5f, 5.f, 3.f, BLOOD_BONE,
		&sounds_skeleton, &fi_skeleton, nullptr, &idle_szkielet, 0.3f, 1.f, ArmorUnitType::NONE),
	UnitData("skeleton_archer", "skeleton.qmsh", MAT_BONE, INT2(3,6), StatProfileType::SKELETON,
		F_HUMANOID | F_UNDEAD | F_DONT_ESCAPE | F_PIERCE_RES25 | F_BLUNT_WEAK25 | F_ARCHER | F_POISON_RES, F2_BLOODLESS | F2_BACKSTAB_RES, F3_DONT_EAT,
		450, 20, skeleton_archer_items, nullptr, INT2(0,0), INT2(0,0), nullptr, G_UNDEAD,
		DMG_SLASH, 1.5f, 5.f, 3.f, BLOOD_BONE,
		&sounds_skeleton, &fi_skeleton, nullptr, &idle_szkielet, 0.3f, 1.f, ArmorUnitType::NONE),
	UnitData("skeleton_fighter", "skeleton.qmsh", MAT_BONE, INT2(3,6), StatProfileType::SKELETON,
		F_HUMANOID | F_UNDEAD | F_DONT_ESCAPE | F_PIERCE_RES25 | F_BLUNT_WEAK25 | F_POISON_RES, F2_MELEE | F2_BLOODLESS | F2_BACKSTAB_RES, F3_DONT_EAT,
		450, 25, skeleton_fighter_items, nullptr, INT2(0,0), INT2(0,0), nullptr, G_UNDEAD,
		DMG_SLASH, 1.5f, 5.f, 3.f, BLOOD_BONE,
		&sounds_skeleton, &fi_skeleton, nullptr, &idle_szkielet, 0.3f, 1.f, ArmorUnitType::NONE),
	UnitData("skeleton_mage", "skeleton.qmsh", MAT_BONE, INT2(4,12), StatProfileType::SKELETON_MAGE,
		F_HUMANOID | F_UNDEAD | F_DONT_ESCAPE | F_PIERCE_RES25 | F_BLUNT_WEAK25 | F_MAGE | F_POISON_RES, F2_BLOODLESS | F2_BACKSTAB_RES, F3_DONT_EAT,
		450, 20, skeleton_mage_items, &skeleton_mage_spells, INT2(0,0), INT2(0,0), nullptr, G_UNDEAD,
		DMG_SLASH, 1.5f, 5.f, 3.f, BLOOD_BONE,
		&sounds_skeleton, &fi_skeleton, nullptr, &idle_szkielet, 0.3f, 1.f, ArmorUnitType::NONE),
	//
	UnitData("necromancer", nullptr, MAT_BODY, INT2(3,15), StatProfileType::MAGE,
		F_HUMAN | F_MAGE | F_LEADER | F_GRAY_HAIR, 0, 0,
		500, 0, necromancer_items, &necromancer_spells, INT2(15,30), INT2(120,240), nullptr, G_UNDEAD,
		DMG_BLUNT, 1.5f, 5.f, 3.f, BLOOD_BLACK,
		&sounds_def, &fi_human, &ti_necromant, &idle_czlowiek, 0.3f, 1.f, ArmorUnitType::HUMAN),
	UnitData("undead_guard", nullptr, MAT_BODY, INT2(2,10), StatProfileType::WARRIOR,
	F_HUMAN | F_UNDEAD | F_DONT_ESCAPE | F_BLUNT_RES25 | F_POISON_RES | F_GRAY_HAIR, F2_MELEE, F3_DONT_EAT,
		525, 10, warrior_items, nullptr, INT2(10,20), INT2(40,80), nullptr, G_UNDEAD,
		DMG_BLUNT, 1.5f, 5.f, 3.f, BLOOD_BLACK,
		&sounds_undead, &fi_human, &ti_undead, &idle_czlowiek, 0.3f, 1.f, ArmorUnitType::HUMAN),
	UnitData("undead_archer", nullptr, MAT_BODY, INT2(2,10), StatProfileType::HUNTER,
		F_HUMAN | F_UNDEAD | F_DONT_ESCAPE | F_BLUNT_RES25 | F_POISON_RES | F_GRAY_HAIR | F_ARCHER, 0, F3_DONT_EAT,
		525, 10, hunter_items, nullptr, INT2(10,20), INT2(40,80), nullptr, G_UNDEAD,
		DMG_BLUNT, 1.5f, 5.f, 3.f, BLOOD_BLACK,
		&sounds_undead, &fi_human, &ti_undead, &idle_czlowiek, 0.3f, 1.f, ArmorUnitType::HUMAN),
	//
	UnitData("evil_cleric", nullptr, MAT_BODY, INT2(4,15), StatProfileType::CLERIC,
		F_HUMAN | F_LEADER | F_GRAY_HAIR, F2_AI_TRAIN, 0,
		525, 0, evil_cleric_items, &evil_cleric_spells, INT2(25,50), INT2(160,320), nullptr, G_UNDEAD,
		DMG_BLUNT, 1.5f, 5.f, 3.f, BLOOD_BLACK,
		&sounds_def, &fi_human, &ti_evil, &idle_czlowiek, 0.3f, 1.f, ArmorUnitType::HUMAN),
	UnitData("evil_cleric_q", nullptr, MAT_BODY, INT2(4,15), StatProfileType::CLERIC,
		F_HUMAN | F_LEADER | F_GRAY_HAIR, F2_AI_TRAIN | F2_MARK, 0,
		525, 0, evil_cleric_items, &evil_cleric_spells, INT2(25,50), INT2(160,320), nullptr, G_UNDEAD,
		DMG_BLUNT, 1.5f, 5.f, 3.f, BLOOD_BLACK,
		&sounds_def, &fi_human, &ti_evil, &idle_czlowiek, 0.3f, 1.f, ArmorUnitType::HUMAN),
	UnitData("q_zlo_boss", nullptr, MAT_BODY, INT2(20), StatProfileType::EVIL_BOSS,
		F_HUMAN | F_LEADER | F_DONT_ESCAPE | F_POISON_RES | F_GRAY_HAIR, F2_MELEE | F2_BOSS | F2_XAR | F2_MAGIC_RES25 | F2_BACKSTAB_RES | F2_MARK, F3_DONT_EAT,
		800, 50, xar_items, &xar_spells, INT2(4000,6000), INT2(4000,6000), dialog_q_zlo, G_UNDEAD,
		DMG_BLUNT, 1.5f, 5.f, 3.f, BLOOD_BLACK,
		&sounds_boss, &fi_human, &ti_evil_boss, &idle_czlowiek, 0.3f, 1.f, ArmorUnitType::HUMAN),

	//---- BANDYCI ----
	UnitData("bandit", nullptr, MAT_BODY, INT2(2,6), StatProfileType::FIGHTER,
		F_HUMAN, F2_AI_TRAIN, 0,
		500, 0, bandit_items, nullptr, INT2(15,30), INT2(20,40), dialog_bandyta, G_BANDITS,
		DMG_BLUNT, 1.5f, 5.f, 3.f, BLOOD_RED,
		&sounds_def, &fi_human, nullptr, &idle_czlowiek, 0.3f, 1.f, ArmorUnitType::HUMAN),
	UnitData("bandit_archer", nullptr, MAT_BODY, INT2(3,7), StatProfileType::FIGHTER,
		F_HUMAN | F_ARCHER, F2_AI_TRAIN, 0,
		500, 0, bandit_archer_items, nullptr, INT2(25,50), INT2(30,60), dialog_bandyta, G_BANDITS,
		DMG_BLUNT, 1.5f, 5.f, 3.f, BLOOD_RED,
		&sounds_def, &fi_human, nullptr, &idle_czlowiek, 0.3f, 1.f, ArmorUnitType::HUMAN),
	UnitData("bandit_hegemon", nullptr, MAT_BODY, INT2(8,12), StatProfileType::FIGHTER,
		F_HUMAN | F_LEADER, F2_BACKSTAB, 0,
		500, 0, bandit_hegemon_items, nullptr, INT2(75,150), INT2(100,200), dialog_bandyta, G_BANDITS,
		DMG_BLUNT, 1.5f, 5.f, 3.f, BLOOD_RED,
		&sounds_def, &fi_human, nullptr, &idle_czlowiek, 0.3f, 1.f, ArmorUnitType::HUMAN),
	UnitData("bandit_hegemon_q", nullptr, MAT_BODY, INT2(8,12), StatProfileType::FIGHTER,
		F_HUMAN | F_LEADER, F2_BACKSTAB | F2_MARK, 0,
		500, 0, bandit_hegemon_items, nullptr, INT2(75,150), INT2(100,200), dialog_bandyta, G_BANDITS,
		DMG_BLUNT, 1.5f, 5.f, 3.f, BLOOD_RED,
		&sounds_def, &fi_human, nullptr, &idle_czlowiek, 0.3f, 1.f, ArmorUnitType::HUMAN),
	UnitData("q_bandyci_szef", nullptr, MAT_BODY, INT2(18), StatProfileType::ROGUE,
		F_HUMAN | F_AI_WANDERS, F2_MELEE | F2_BOSS | F2_BACKSTAB | F2_MARK | F2_GUARDED, 0,
		600, 25, q_bandyci_szef_items, nullptr, INT2(1000,2000), INT2(1000,2000), dialog_q_bandyci, G_BANDITS,
		DMG_BLUNT, 1.5f, 5.f, 3.f, BLOOD_RED,
		&sounds_def, &fi_human, nullptr, &idle_czlowiek, 0.3f, 1.f, ArmorUnitType::HUMAN),

	//---- MAGOWIE I GOLEMY ----
	UnitData("mage", nullptr, MAT_BODY, INT2(3, 15), StatProfileType::MAGE,
		F_HUMAN | F_MAGE, 0, 0,
		500, 0, mage_items, &mage_spells, INT2(15,30), INT2(150,300), dialog_mag_obstawa, G_MAGES,
		DMG_BLUNT, 1.5f, 5.f, 3.f, BLOOD_RED,
		&sounds_def, &fi_human, nullptr, &idle_czlowiek, 0.3f, 1.f, ArmorUnitType::HUMAN),
	UnitData("mage_q", nullptr, MAT_BODY, INT2(3,15), StatProfileType::MAGE,
		F_HUMAN | F_MAGE, F2_MARK, 0,
		500, 0, mage_items, &mage_spells, INT2(15,30), INT2(150,300), dialog_mag_obstawa, G_MAGES,
		DMG_BLUNT, 1.5f, 5.f, 3.f, BLOOD_RED,
		&sounds_def, &fi_human, nullptr, &idle_czlowiek, 0.3f, 1.f, ArmorUnitType::HUMAN),
	UnitData("mage_guard", nullptr, MAT_BODY, INT2(2,10), StatProfileType::FIGHTER,
		F_HUMAN, F2_AI_TRAIN, 0,
		500, 0, guard_items, nullptr, INT2(10,20), INT2(50,100), dialog_mag_obstawa, G_MAGES,
		DMG_BLUNT, 1.5f, 5.f, 3.f, BLOOD_RED,
		&sounds_def, &fi_human, nullptr, &idle_czlowiek, 0.3f, 1.f, ArmorUnitType::HUMAN),
	UnitData("golem_stone", "golem.qmsh", MAT_ROCK, INT2(8), StatProfileType::GOLEM,
		F_DONT_ESCAPE | F_PIERCE_RES25 | F_SLASH_RES25 | F_BLUNT_WEAK25 | F_DONT_SUFFER | F_POISON_RES, F2_BLOODLESS | F2_IGNORE_BLOCK | F2_MAGIC_RES25 | F2_BACKSTAB_RES, F3_DONT_EAT,
		800, 50, nullptr, nullptr, INT2(0,0), INT2(0,0), nullptr, G_MAGES,
		DMG_BLUNT, 1.5f, 3.f, 2.f, BLOOD_ROCK,
		&sounds_golem, &fi_golem, nullptr, &idle_golem, 0.5f, 1.2f, ArmorUnitType::NONE),
	UnitData("golem_iron", "golem.qmsh", MAT_IRON, INT2(12), StatProfileType::GOLEM,
		F_DONT_ESCAPE | F_PIERCE_RES25 | F_SLASH_RES25 | F_DONT_SUFFER | F_POISON_RES, F2_BLOODLESS | F2_IGNORE_BLOCK | F2_MAGIC_RES25 | F2_BACKSTAB_RES, F3_DONT_EAT,
		900, 100, nullptr, nullptr, INT2(0,0), INT2(0,0), nullptr, G_MAGES,
		DMG_BLUNT, 1.5f, 3.f, 2.f, BLOOD_IRON,
		&sounds_golem_iron, &fi_golem, &ti_golem_iron, &idle_golem, 0.5f, 1.2f, ArmorUnitType::NONE),
	UnitData("golem_adamantine", "golem.qmsh", MAT_IRON, INT2(20), StatProfileType::GOLEM,
		F_DONT_ESCAPE | F_PIERCE_RES25 | F_SLASH_RES25 | F_BLUNT_RES25 | F_DONT_SUFFER | F_POISON_RES, F2_BLOODLESS | F2_IGNORE_BLOCK | F2_MAGIC_RES50 | F2_BACKSTAB_RES, F3_DONT_EAT,
		1000, 150, nullptr, nullptr, INT2(0), INT2(0), nullptr, G_MAGES,
		DMG_BLUNT, 1.5f, 3.f, 2.f, BLOOD_IRON,
		&sounds_golem_iron, &fi_golem, &ti_golem_adamantine, &idle_golem, 0.5f, 1.2f, ArmorUnitType::NONE),
	UnitData("q_magowie_golem", "golem.qmsh", MAT_ROCK, INT2(8), StatProfileType::GOLEM,
		F_DONT_ESCAPE | F_PIERCE_RES25 | F_SLASH_RES25 | F_BLUNT_WEAK25 | F_DONT_SUFFER | F_POISON_RES, F2_BLOODLESS | F2_GOLEM_SOUNDS | F2_IGNORE_BLOCK | F2_MAGIC_RES25 | F2_BACKSTAB_RES, F3_DONT_EAT,
		800, 50, nullptr, nullptr, INT2(0,0), INT2(0,0), dialog_q_magowie, G_MAGES,
		DMG_BLUNT, 1.5f, 3.f, 2.f, BLOOD_ROCK,
		&sounds_golem, &fi_golem, nullptr, &idle_golem, 0.5f, 1.2f, ArmorUnitType::NONE),
	UnitData("q_magowie_boss", nullptr, MAT_BODY, INT2(20), StatProfileType::MAGE,
		F_HUMAN | F_MAGE, F2_OLD | F2_BOSS | F2_MARK | F2_GUARDED, 0,
		600, 25, q_magowie_boss_items, &mage_boss_spells, INT2(1000,2000), INT2(1000,2000), dialog_q_magowie2, G_MAGES,
		DMG_BLUNT, 1.5f, 5.f, 3.f, BLOOD_RED,
		&sounds_def, &fi_human, nullptr, &idle_czlowiek, 0.3f, 1.f, ArmorUnitType::HUMAN),

	//---- ZWIERZ TA I £OWCY ----
	UnitData("wolf", "wilk.qmsh", MAT_SKIN, INT2(1, 3), StatProfileType::ANIMAL,
		F_DONT_OPEN | F_NO_POWER_ATTACK, F2_IGNORE_BLOCK, F3_DONT_EAT,
		300, 5, wolf_items, nullptr, INT2(0,0), INT2(0,0), nullptr, G_ANIMALS,
		DMG_SLASH, 1.5f, 6.f, 3.5f, BLOOD_RED,
		&sounds_wolf, &fi_wilk, nullptr, &idle_wilk, 0.4f, 1.f, ArmorUnitType::NONE),
	UnitData("worg", "wilk.qmsh", MAT_SKIN, INT2(4,8), StatProfileType::ANIMAL,
		F_DONT_OPEN | F_NO_POWER_ATTACK, F2_IGNORE_BLOCK, F3_DONT_EAT,
		300, 10, wolf_items, nullptr, INT2(0,0), INT2(0,0), nullptr, G_ANIMALS,
		DMG_SLASH, 1.5f, 6.f, 3.5f, BLOOD_RED,
		&sounds_wolf, &fi_wilk, &ti_worg, &idle_wilk, 0.4f, 1.f, ArmorUnitType::NONE),
	//
	UnitData("spider", "pajak.qmsh", MAT_BODY, INT2(1, 3), StatProfileType::ANIMAL,
		F_POISON_ATTACK | F_DONT_OPEN | F_SLIGHT | F_POISON_RES | F_NO_POWER_ATTACK, F2_IGNORE_BLOCK, F3_DONT_EAT,
		100, 5, nullptr, nullptr, INT2(0,0), INT2(0,0), nullptr, G_ANIMALS,
		DMG_PIERCE, 1.f, 4.f, 3.5f, BLOOD_GREEN,
		&sounds_spider, &fi_spider, nullptr, &idle_spider, 0.3f, 1.f, ArmorUnitType::NONE),
	UnitData("spider_big", "pajak2.qmsh", MAT_BODY, INT2(4,8), StatProfileType::ANIMAL,
		F_POISON_ATTACK | F_DONT_OPEN | F_POISON_RES | F_NO_POWER_ATTACK, F2_IGNORE_BLOCK, F3_DONT_EAT,
		200, 10, nullptr, nullptr, INT2(0,0), INT2(0,0), nullptr, G_ANIMALS,
		DMG_PIERCE, 1.f, 4.f, 3.5f, BLOOD_GREEN,
		&sounds_spider, &fi_spider, nullptr, &idle_spider, 0.4f, 1.f, ArmorUnitType::NONE),
	//
	UnitData("rat", "szczur.qmsh", MAT_BODY, INT2(1, 3), StatProfileType::ANIMAL,
		F_COWARD | F_DONT_OPEN | F_SLIGHT | F_NO_POWER_ATTACK, F2_IGNORE_BLOCK, F3_DONT_EAT,
		50, 0, nullptr, nullptr, INT2(0,0), INT2(0,0), nullptr, G_ANIMALS,
		DMG_PIERCE, 1.f, 5.f, 3.5f, BLOOD_RED,
		&sounds_rat, &fi_szczur, nullptr, &idle_szczur, 0.2f, 0.75f, ArmorUnitType::NONE),
	//
	UnitData("wild_hunter", nullptr, MAT_BODY, INT2(5, 13), StatProfileType::HUNTER,
		F_HUMAN | F_ARCHER | F_LEADER, 0, 0,
		500, 5, hunter_items, nullptr, INT2(4,10), INT2(40,80), nullptr, G_ANIMALS,
		DMG_BLUNT, 1.5f, 5.f, 3.f, BLOOD_RED,
		&sounds_def, &fi_human, &ti_crazy_hunter, &idle_czlowiek, 0.3f, 1.f, ArmorUnitType::HUMAN),
	UnitData("q_szaleni_szaleniec", nullptr, MAT_BODY, INT2(5,13), StatProfileType::HUNTER,
		F_HUMAN | F_ARCHER | F_LEADER, F2_MARK, 0,
		500, 5, szaleniec_items, nullptr, INT2(4,10), INT2(40,80), dialog_q_szaleni, G_ANIMALS,
		DMG_BLUNT, 1.5f, 5.f, 3.f, BLOOD_RED,
		&sounds_def, &fi_human, &ti_crazy_hunter, &idle_czlowiek, 0.3f, 1.f, ArmorUnitType::HUMAN),

	//---- SZALE—CY ----
	UnitData("crazy_mage", nullptr, MAT_BODY, INT2(2,20), StatProfileType::MAGE,
		F_HUMAN | F_MAGE | F_CRAZY | F_HERO | F_AI_WANDERS, F2_TOURNAMENT, F3_CONTEST_25,
		510, 5, mage_items, &mage_spells, INT2(10,25), INT2(100,250), dialog_szalony, G_CRAZIES,
		DMG_BLUNT, 1.55f, 5.1f, 3.1f, BLOOD_RED,
		&sounds_def, &fi_human, nullptr, &idle_czlowiek, 0.3f, 1.f, ArmorUnitType::HUMAN),
	UnitData("crazy_warrior", nullptr, MAT_BODY, INT2(2,20), StatProfileType::WARRIOR,
		F_HUMAN | F_CRAZY | F_HERO | F_AI_WANDERS, F2_AI_TRAIN | F2_CONTEST | F2_MELEE | F2_TOURNAMENT, 0,
		510, 5, warrior_items, nullptr, INT2(8,22), INT2(80,220), dialog_szalony, G_CRAZIES,
		DMG_BLUNT, 1.55f, 5.1f, 3.1f, BLOOD_RED,
		&sounds_def, &fi_human, nullptr, &idle_czlowiek, 0.3f, 1.f, ArmorUnitType::HUMAN),
	UnitData("crazy_hunter", nullptr, MAT_BODY, INT2(2,20), StatProfileType::HUNTER,
		F_HUMAN | F_CRAZY | F_ARCHER | F_HERO | F_AI_WANDERS, F2_AI_TRAIN | F2_CONTEST_50 | F2_TOURNAMENT, 0,
		510, 5, hunter_items, nullptr, INT2(6,18), INT2(60,180), dialog_szalony, G_CRAZIES,
		DMG_BLUNT, 1.55f, 5.1f, 3.1f, BLOOD_RED,
		&sounds_def, &fi_human, nullptr, &idle_czlowiek, 0.3f, 1.f, ArmorUnitType::HUMAN),
	UnitData("crazy_rogue", nullptr, MAT_BODY, INT2(2,20), StatProfileType::ROGUE,
		F_HUMAN | F_CRAZY | F_HERO | F_AI_WANDERS, F2_AI_TRAIN | F2_CONTEST_50 | F2_MELEE_50 | F2_TOURNAMENT | F2_BACKSTAB, 0,
		510, 5, rogue_items, nullptr, INT2(7,20), INT2(70,200), dialog_szalony, G_CRAZIES,
		DMG_BLUNT, 1.55f, 5.1f, 3.1f, BLOOD_RED,
		&sounds_def, &fi_human, nullptr, &idle_czlowiek, 0.3f, 1.f, ArmorUnitType::HUMAN),

	//---- HEROSI ----
	UnitData("hero_mage", nullptr, MAT_BODY, INT2(2,20), StatProfileType::MAGE,
		F_HUMAN | F_MAGE | F_HERO | F_AI_WANDERS, F2_TOURNAMENT, F3_CONTEST_25,
		500, 0, mage_items, &mage_spells, INT2(10,25), INT2(100,250), dialog_hero, G_CITIZENS,
		DMG_BLUNT, 1.5f, 5.f, 3.f, BLOOD_RED,
		&sounds_def, &fi_human, nullptr, &idle_czlowiek, 0.3f, 1.f, ArmorUnitType::HUMAN),
	UnitData("hero_warrior", nullptr, MAT_BODY, INT2(2,20), StatProfileType::WARRIOR,
		F_HUMAN | F_HERO | F_AI_WANDERS, F2_AI_TRAIN | F2_CONTEST | F2_MELEE | F2_TOURNAMENT, 0,
		500, 0, warrior_items, nullptr, INT2(8,22), INT2(80,220), dialog_hero, G_CITIZENS,
		DMG_BLUNT, 1.5f, 5.f, 3.f, BLOOD_RED,
		&sounds_def, &fi_human, nullptr, &idle_czlowiek, 0.3f, 1.f, ArmorUnitType::HUMAN),
	UnitData("hero_hunter", nullptr, MAT_BODY, INT2(2,20), StatProfileType::HUNTER,
		F_HUMAN | F_ARCHER | F_HERO | F_AI_WANDERS, F2_AI_TRAIN | F2_CONTEST_50 | F2_TOURNAMENT, 0,
		500, 0, hunter_items, nullptr, INT2(6,18), INT2(60,180), dialog_hero, G_CITIZENS,
		DMG_BLUNT, 1.5f, 5.f, 3.f, BLOOD_RED,
		&sounds_def, &fi_human, nullptr, &idle_czlowiek, 0.3f, 1.f, ArmorUnitType::HUMAN),
	UnitData("hero_rogue", nullptr, MAT_BODY, INT2(2,20), StatProfileType::ROGUE,
		F_HUMAN | F_HERO | F_AI_WANDERS, F2_AI_TRAIN | F2_CONTEST_50 | F2_MELEE_50 | F2_TOURNAMENT | F2_BACKSTAB, 0,
		500, 0, rogue_items, nullptr, INT2(7,20), INT2(70,200), dialog_hero, G_CITIZENS,
		DMG_BLUNT, 1.5f, 5.f, 3.f, BLOOD_RED,
		&sounds_def, &fi_human, nullptr, &idle_czlowiek, 0.3f, 1.f, ArmorUnitType::HUMAN),

	//---- SPECJALNE POSTACIE ----	
	UnitData("tomashu", nullptr, MAT_BODY, INT2(25), StatProfileType::TOMASHU,
		F_HUMAN | F_DONT_ESCAPE | F_TOMASHU | F_LEADER | F_SECRET | F_IMMORTAL | F_POISON_RES, F2_SIT_ON_THRONE | F2_MELEE | F2_MAGIC_RES50 | F2_BACKSTAB_RES, F3_DONT_EAT,
		750, 50, tomash_items, &tomash_spells, INT2(100), INT2(100), dialog_tomashu, G_CITIZENS,
		DMG_BLUNT, 1.5f, 5.f, 3.f, BLOOD_RED,
		&sounds_def, &fi_human, nullptr, &idle_czlowiek, 0.3f, 1.f, ArmorUnitType::HUMAN),

	UnitData("unk", "unk.qmsh", MAT_ROCK, INT2(13), StatProfileType::UNK,
		F_DONT_ESCAPE | F_NO_POWER_ATTACK | F_DONT_SUFFER | F_POISON_RES | F_PIERCE_RES25, F2_BLOODLESS | F2_IGNORE_BLOCK | F2_MAGIC_RES25 | F2_BACKSTAB, F3_DONT_EAT,
		700, 50, nullptr, nullptr, INT2(0), INT2(0), nullptr, G_UNDEAD,
		DMG_SLASH, 0.5f, 10.f, 8.f, BLOOD_BLACK,
		&sounds_nieznany, &fi_nieznany, nullptr, &idle_nieznany, 0.4f, 1.5f, ArmorUnitType::NONE),
};
const uint n_base_units = countof(g_base_units);

void UnitData::CopyFrom(UnitData& ud)
{
	mesh = ud.mesh;
	mat = ud.mat;
	level = ud.level;
	profile = ud.profile;
	stat_profile = ud.stat_profile;
	hp_bonus = ud.hp_bonus;
	def_bonus = ud.def_bonus;
	dmg_type = ud.dmg_type;
	flags = ud.flags;
	flags2 = ud.flags2;
	flags3 = ud.flags3;
	items = ud.items;
	spells = ud.spells;
	gold = ud.gold;
	gold2 = ud.gold2;
	dialog = ud.dialog;
	group = ud.group;
	walk_speed = ud.walk_speed;
	run_speed = ud.run_speed;
	rot_speed = ud.rot_speed;
	width = ud.width;
	attack_range = ud.attack_range;
	blood = ud.blood;
	sounds = ud.sounds;
	frames = ud.frames;
	tex = ud.tex;
	idles = ud.idles;
	armor_type = ud.armor_type;
	item_script = ud.item_script;
	new_items = ud.new_items;
}

enum KeywordGroup
{
	G_TYPE,
	G_PROPERTY,
	G_MATERIAL,
	G_FLAGS,
	G_FLAGS2,
	G_FLAGS3,
	G_GROUP,
	G_DMG_TYPE,
	G_BLOOD,
	G_ARMOR_TYPE,
	G_ATTRIBUTE,
	G_SKILL,
	G_PROFILE_KEYWORD,
	G_SOUND_TYPE,
	G_FRAME_KEYWORD,
	G_WEAPON_FLAG,
	G_NULL,
	G_SPELL,
	G_SPELL_KEYWORD,
	G_ITEM_KEYWORD
};

enum Type
{
	T_UNIT,
	T_PROFILE,
	T_ITEMS,
	T_SPELLS,
	T_SOUNDS,
	T_FRAMES,
	T_TEX,
	T_IDLES
};

enum Property
{
	P_MESH,
	P_MAT,
	P_LEVEL,
	P_PROFILE,
	P_FLAGS,
	P_HP,
	P_DEF,
	P_ITEMS,
	P_SPELLS,
	P_GOLD,
	P_DIALOG,
	P_GROUP,
	P_DMG_TYPE,
	P_WALK_SPEED,
	P_RUN_SPEED,
	P_ROT_SPEED,
	P_BLOOD,
	P_SOUNDS,
	P_FRAMES,
	P_TEX,
	P_IDLES,
	P_WIDTH,
	P_ATTACK_RANGE,
	P_ARMOR_TYPE
};

enum ProfileKeyword
{
	PK_FIXED
};

enum SoundType
{
	ST_SEE_ENEMY, 
	ST_PAIN,
	ST_DEATH,
	ST_ATTACK
};

enum FrameKeyword
{
	FK_ATTACKS,
	FK_SIMPLE_ATTACKS,
	FK_CAST,
	FK_TAKE_WEAPON,
	FK_BASH
};

enum ItemKeyword
{
	IK_CHANCE,
	IK_RANDOM,
	IK_IF,
	IK_ELSE,
	IK_LEVEL
};

enum SpellKeyword
{
	SK_NON_COMBAT,
	SK_NULL
};

//=================================================================================================
bool LoadProfile(Tokenizer& t, CRC32& crc, StatProfile** result = nullptr)
{
	StatProfile* profile = new StatProfile;
	profile->fixed = false;
	for(int i = 0; i < (int)Attribute::MAX; ++i)
		profile->attrib[i] = 10;
	for(int i = 0; i < (int)Skill::MAX; ++i)
		profile->skill[i] = -1;

	try
	{
		if(!result)
		{
			// not inline, id
			profile->id = t.MustGetItemKeyword();
			crc.Update(profile->id);
			t.Next();

			// {
			t.AssertSymbol('{');
		}

		t.Next();

		while(!t.IsSymbol('}'))
		{
			if(t.IsKeyword(PK_FIXED, G_PROFILE_KEYWORD))
			{
				t.Next();
				profile->fixed = t.MustGetBool();
			}
			else if(t.IsKeywordGroup(G_ATTRIBUTE))
			{
				int a = t.MustGetKeywordId(G_ATTRIBUTE);
				t.Next();
				int val = t.MustGetInt();
				if(val < 1)
					t.Throw("Invalid attribute '%s' value %d.", g_attributes[a].id, val);
				crc.Update(a);
				crc.Update(val);
				profile->attrib[a] = val;
			}
			else if(t.IsKeywordGroup(G_SKILL))
			{
				int s = t.MustGetKeywordId(G_SKILL);
				t.Next();
				int val = t.MustGetInt();
				if(val < -1)
					t.Throw("Invalid skill '%s' value %d.", g_skills[s].id, val);
				crc.Update(s);
				crc.Update(val);
				profile->skill[s] = val;
			}
			else
			{
				int a = PK_FIXED, b = G_PROFILE_KEYWORD, c = G_ATTRIBUTE, d = G_SKILL;
				t.StartUnexpected().Add(Tokenizer::T_KEYWORD, &a, &b).Add(Tokenizer::T_KEYWORD_GROUP, &c).Add(Tokenizer::T_KEYWORD_GROUP, &d).Throw();
			}

			t.Next();
		}

		if(!profile->id.empty())
		{
			for(StatProfile* s : stat_profiles)
			{
				if(s->id == profile->id)
					t.Throw("Profile with that id already exists.");
			}
		}

		if(result)
			*result = profile;
		stat_profiles.push_back(profile);
		return true;
	}
	catch(const Tokenizer::Exception& e)
	{
		if(!profile->id.empty())
			ERROR(Format("Failed to load profile '%s': %s", profile->id.c_str(), e.ToString()));
		else
			ERROR(Format("Failed to load profile: %s", e.ToString()));
		delete profile;
		return false;
	}
}

//=================================================================================================
void AddItem(ItemScript* script, Tokenizer& t, CRC32& crc)
{
	if(!t.IsSymbol('!'))
	{
		const string& s = t.MustGetItemKeyword();
		const Item* item = FindItem(s.c_str(), false);
		if(item)
		{
			script->code.push_back(PS_ITEM);
			script->code.push_back((int)item);
			crc.Update(PS_ITEM);
			crc.Update(item->id);
		}
		else
			t.Throw("Missing item '%s'.", s.c_str());
	}
	else
	{
		t.Next();
		if(t.IsSymbol('+') || t.IsSymbol('-'))
		{
			bool minus = t.IsSymbol('-');
			char c = t.PeekChar();
			if(c >= '1' && c <= '9')
			{
				int mod = c - '0';
				if(minus)
					mod = -mod;
				t.NextChar();
				t.Next();
				const string& s = t.MustGetItemKeyword();
				ItemListResult lis = FindItemList(s.c_str(), false);
				if(lis.lis)
				{
					if(lis.is_leveled)
					{
						script->code.push_back(PS_LEVELED_LIST_MOD);
						script->code.push_back(mod);
						script->code.push_back((int)lis.llis);
						crc.Update(PS_LEVELED_LIST_MOD);
						crc.Update(mod);
						crc.Update(lis.llis->id);
					}
					else
						t.Throw("Can't use mod on non leveled list '%s'.", s.c_str());
				}
				else
					t.Throw("Missing item list '%s'.", s.c_str());
			}
			else
				t.Throw("Invalid leveled list mod '%c'.", c);
		}
		else
		{
			const string& s = t.MustGetItemKeyword();
			ItemListResult lis = FindItemList(s.c_str(), false);
			if(lis.lis)
			{
				ParseScript type = (lis.is_leveled ? PS_LEVELED_LIST : PS_LIST);
				script->code.push_back(type);
				script->code.push_back((int)lis.lis);
				crc.Update(type);
				crc.Update(lis.GetIdString());
			}
			else
				t.Throw("Missing item list '%s'.", s.c_str());
		}
	}
}

enum IfState
{
	IFS_START,
	IFS_START_INLINE,
	IFS_ELSE,
	IFS_ELSE_INLINE
};

//=================================================================================================
bool LoadItems(Tokenizer& t, CRC32& crc, ItemScript** result = nullptr)
{
	ItemScript* script = new ItemScript;
	vector<IfState> if_state;
	bool done_if = false;

	try
	{
		if(!result)
		{
			// not inline, id
			script->id = t.MustGetItemKeyword();
			crc.Update(script->id);
			t.Next();

			// {
			t.AssertSymbol('{');
		}

		t.Next();

		while(true)
		{
			if(t.IsSymbol('}'))
			{
				while(!if_state.empty() && if_state.back() == IFS_ELSE_INLINE)
				{
					script->code.push_back(PS_END_IF);
					crc.Update(PS_END_IF);
					if_state.pop_back();
				}
				if(if_state.empty())
					break;
				if(if_state.back() == IFS_START)
				{
					t.Next();
					if(t.IsKeyword(IK_ELSE, G_ITEM_KEYWORD))
					{
						script->code.push_back(PS_ELSE);
						crc.Update(PS_ELSE);
						t.Next();
						if(t.IsKeyword(IK_IF, G_ITEM_KEYWORD))
						{
							// else if
							t.Next();
							ItemKeyword iif = (ItemKeyword)t.MustGetKeywordId(G_ITEM_KEYWORD);
							if(iif == IK_LEVEL)
							{
								t.Next();
								int lvl = t.MustGetInt();
								if(lvl < 0)
									t.Throw("Invalid level %d.", lvl);
								script->code.push_back(PS_IF_LEVEL);
								script->code.push_back(lvl);
								crc.Update(PS_IF_LEVEL);
								crc.Update(lvl);
							}
							else if(iif == IK_CHANCE)
							{
								t.Next();
								int chance = t.MustGetInt();
								if(chance <= 0 || chance >= 100)
									t.Throw("Invalid chance %d.", chance);
								script->code.push_back(PS_IF_CHANCE);
								script->code.push_back(chance);
								crc.Update(PS_IF_CHANCE);
								crc.Update(chance);
							}
							else
							{
								int g = G_ITEM_KEYWORD,
									a = IK_CHANCE,
									b = IK_LEVEL;
								t.StartUnexpected().Add(Tokenizer::T_KEYWORD, &a, &g).Add(Tokenizer::T_KEYWORD, &b, &g).Throw();
							}
							t.Next();
							if_state.back() = IFS_ELSE_INLINE;
							if(t.IsSymbol('{'))
							{
								t.Next();
								if_state.push_back(IFS_START);
							}
							else
								if_state.push_back(IFS_START_INLINE);
						}
						else if(t.IsSymbol('{'))
						{
							// else { ... }
							t.Next();
							if_state.back() = IFS_ELSE;
						}
						else
						{
							// single expression else
							if_state.back() = IFS_ELSE_INLINE;
						}
					}
					else
					{
						// } end of if
						script->code.push_back(PS_END_IF);
						crc.Update(PS_END_IF);
						if_state.pop_back();
						while(!if_state.empty() && if_state.back() == IFS_ELSE_INLINE)
						{
							script->code.push_back(PS_END_IF);
							crc.Update(PS_END_IF);
							if_state.pop_back();
						}
					}
				}
				else if(if_state.back() == IFS_ELSE)
				{
					// } end of else
					script->code.push_back(PS_END_IF);
					if_state.pop_back();
					t.Next();
					while(!if_state.empty() && if_state.back() == IFS_ELSE_INLINE)
					{
						script->code.push_back(PS_END_IF);
						if_state.pop_back();
					}
				}
				else
					t.Unexpected();
				continue;
			}
			else if(t.IsInt())
			{
				// give multiple
				int count = t.MustGetInt();
				if(count <= 0)
					t.Throw("Invalid item count %d.", count);
				t.Next();

				// get item
				script->code.push_back(PS_MANY);
				script->code.push_back(count);
				crc.Update(PS_MANY);
				crc.Update(count);
				AddItem(script, t, crc);
			}
			else if(t.IsKeywordGroup(G_ITEM_KEYWORD))
			{
				ItemKeyword k = (ItemKeyword)t.GetKeywordId(G_ITEM_KEYWORD);
				t.Next();

				switch(k)
				{
				case IK_CHANCE:
					{
						// get chance value
						int chance = t.MustGetInt();
						t.Next();
						if(chance <= 0 || chance >= 100)
							t.Throw("Invalid chance %d.", chance);
						
						if(t.IsSymbol('{'))
						{
							// two item chance
							script->code.push_back(PS_CHANCE2);
							script->code.push_back(chance);
							crc.Update(PS_CHANCE2);
							crc.Update(chance);
							t.Next();
							AddItem(script, t, crc);
							t.Next();
							AddItem(script, t, crc);
							t.Next();
							t.AssertSymbol('}');
						}
						else
						{
							// single item chance
							script->code.push_back(PS_CHANCE);
							script->code.push_back(chance);
							crc.Update(PS_CHANCE);
							crc.Update(chance);
							AddItem(script, t, crc);
						}
					}
					break;
				case IK_RANDOM:
					if(t.IsSymbol('{'))
					{
						// one of many
						script->code.push_back(PS_ONE_OF_MANY);
						crc.Update(PS_ONE_OF_MANY);
						uint pos = script->code.size();
						int count = 0;
						script->code.push_back(0);
						t.Next();
						do
						{
							AddItem(script, t, crc);
							t.Next();
							++count;
						} while(!t.IsSymbol('}'));

						if(count < 2)
							t.Throw("Invalid one of many count %d.", count);
						script->code[pos] = count;
						crc.Update(count);
					}
					else
					{
						// get count
						int from = t.MustGetInt();
						t.Next();
						int to = t.MustGetInt();
						t.Next();
						if(from < 0 || from > to)
							t.Throw("Invalid random count (%d %d).", from, to);

						// get item
						script->code.push_back(PS_RANDOM);
						script->code.push_back(from);
						script->code.push_back(to);
						crc.Update(PS_RANDOM);
						crc.Update(from);
						crc.Update(to);
						AddItem(script, t, crc);
					}
					break;
				case IK_IF:
					{
						ItemKeyword iif = (ItemKeyword)t.MustGetKeywordId(G_ITEM_KEYWORD);
						if(iif == IK_LEVEL)
						{
							t.Next();
							int lvl = t.MustGetInt();
							if(lvl < 0)
								t.Throw("Invalid level %d.", lvl);
							script->code.push_back(PS_IF_LEVEL);
							script->code.push_back(lvl);
							crc.Update(PS_IF_LEVEL);
							crc.Update(lvl);
						}
						else if(iif == IK_CHANCE)
						{
							t.Next();
							int chance = t.MustGetInt();
							if(chance <= 0 || chance >= 100)
								t.Throw("Invalid chance %d.", chance);
							script->code.push_back(PS_IF_CHANCE);
							script->code.push_back(chance);
							crc.Update(PS_IF_CHANCE);
							crc.Update(chance);
						}
						else
						{
							int g = G_ITEM_KEYWORD,
								a = IK_CHANCE,
								b = IK_LEVEL;
							t.StartUnexpected().Add(Tokenizer::T_KEYWORD, &a, &g).Add(Tokenizer::T_KEYWORD, &b, &g).Throw();
						}
						t.Next();
						if(t.IsSymbol('{'))
						{
							t.Next();
							if_state.push_back(IFS_START);
						}
						else
							if_state.push_back(IFS_START_INLINE);
						done_if = true;
					}
					break;
				default:
					t.Unexpected();
				}
			}
			else if(t.IsKeyword() || t.IsItem() || t.IsSymbol('!'))
			{
				// single item
				script->code.push_back(PS_ONE);
				crc.Update(PS_ONE);
				AddItem(script, t, crc);
			}
			else
				t.Unexpected();
			
			if(done_if)
				done_if = false;
			else
			{
				t.Next();
				while(!if_state.empty())
				{
					if(if_state.back() == IFS_START_INLINE)
					{
						if_state.pop_back();
						if(t.IsKeyword(IK_ELSE, G_ITEM_KEYWORD))
						{
							script->code.push_back(PS_ELSE);
							crc.Update(PS_ELSE);
							t.Next();
							if(t.IsSymbol('{'))
							{
								t.Next();
								if_state.push_back(IFS_ELSE);
							}
							else
								if_state.push_back(IFS_ELSE_INLINE);
							break;
						}
						else
						{
							script->code.push_back(PS_END_IF);
							crc.Update(PS_END_IF);
						}
					}
					else if(if_state.back() == IFS_ELSE_INLINE)
					{
						script->code.push_back(PS_END_IF);
						crc.Update(PS_END_IF);
						if_state.pop_back();
					}
					else
						break;
				}
			}
		}

		while(!if_state.empty())
		{
			if(if_state.back() == IFS_START_INLINE || if_state.back() == IFS_ELSE_INLINE)
			{
				script->code.push_back(PS_END_IF);
				crc.Update(PS_END_IF);
				if_state.pop_back();
			}
			else
				t.Throw("Missing closing '}'.");
		}

		script->code.push_back(PS_END);
		crc.Update(PS_END);

		if(!script->id.empty())
		{
			for(ItemScript* is : item_scripts)
			{
				if(is->id == script->id)
					t.Throw("Item script with that id already exists.");
			}
		}

		item_scripts.push_back(script);
		if(result)
			*result = script;
		return true;
	}
	catch(const Tokenizer::Exception& e)
	{
		if(!script->id.empty())
			ERROR(Format("Failed to load item script '%s': %s", script->id.c_str(), e.ToString()));
		else
			ERROR(Format("Failed to load item script: %s", e.ToString()));
		delete script;
		return false;
	}
}

//=================================================================================================
bool LoadSpells(Tokenizer& t, CRC32& crc, SpellList** result = nullptr)
{
	SpellList* spell = new SpellList;

	try
	{
		if(!result)
		{
			// not inline, id
			spell->id = t.MustGetItemKeyword();
			crc.Update(spell->id);
			t.Next();

			// {
			t.AssertSymbol('{');
		}

		t.Next();

		int index = 0;

		do
		{
			if(t.IsKeywordGroup(G_SPELL_KEYWORD))
			{
				SpellKeyword k = (SpellKeyword)t.GetKeywordId(G_SPELL_KEYWORD);
				if(k == SK_NON_COMBAT)
				{
					// non_combat
					t.Next();
					spell->have_non_combat = t.MustGetBool();
					crc.Update(spell->have_non_combat ? 2 : 1);
				}
				else
				{
					// null
					if(index == 3)
						t.Throw("Too many spells (max 3 for now).");
					++index;
					crc.Update(0);
				}
			}
			else
			{
				if(index == 3)
					t.Throw("Too many spells (max 3 for now).");
				t.AssertSymbol('{');
				t.Next();
				int spell_id = t.MustGetKeywordId(G_SPELL);
				t.Next();
				int lvl = t.MustGetInt();
				if(lvl < 0)
					t.Throw("Invalid spell level %d.", lvl);
				t.Next();
				t.AssertSymbol('}');
				spell->spell[index] = &g_spells[spell_id];
				spell->level[index] = lvl;
				crc.Update(spell_id);
				crc.Update(lvl);
				++index;
			}
			t.Next();
		} while(!t.IsSymbol('}'));

		if(spell->spell[0] == nullptr && spell->spell[1] == nullptr && spell->spell[2] == nullptr)
			t.Throw("Empty spell list.");

		if(!spell->id.empty())
		{
			for(SpellList* sl : spell_lists)
			{
				if(sl->id == spell->id)
					t.Throw("Spell list with that id already exists.");
			}
		}

		spell_lists.push_back(spell);
		if(result)
			*result = spell;
		return true;
	}
	catch(const Tokenizer::Exception& e)
	{
		if(!spell->id.empty())
			ERROR(Format("Failed to load spell list '%s': %s", spell->id.c_str(), e.ToString()));
		else
			ERROR(Format("Failed to load spell list: %s", e.ToString()));
		delete spell;
		return false;
	}
}

//=================================================================================================
bool LoadSounds(Tokenizer& t, CRC32& crc, SoundPack** result = nullptr)
{
	SoundPack* sound = new SoundPack;

	try
	{
		if(!result)
		{
			// not inline, id
			sound->id = t.MustGetItemKeyword();
			crc.Update(sound->id);
			t.Next();

			// {
			t.AssertSymbol('{');
		}

		t.Next();

		while(!t.IsSymbol('}'))
		{
			SoundType type = (SoundType)t.MustGetKeywordId(G_SOUND_TYPE);
			t.Next();

			sound->filename[(int)type] = t.MustGetString();
			crc.Update(type);
			crc.Update(sound->filename[(int)type]);
			t.Next();
		}

		// check if not exists
		if(!sound->id.empty())
		{
			for(SoundPack* s : sound_packs)
			{
				if(s->id == sound->id)
					t.Throw("Sound pack with that id already exists.");
			}
		}

		if(result)
			*result = sound;
		sound_packs.push_back(sound);
		return true;
	}
	catch(const Tokenizer::Exception& e)
	{
		if(!sound->id.empty())
			ERROR(Format("Failed to load sound pack '%s': %s", sound->id.c_str(), e.ToString()));
		else
			ERROR(Format("Failed to load sound pack: %s", e.ToString()));
		delete sound;
		return false;
	}
}

//=================================================================================================
bool LoadFrames(Tokenizer& t, CRC32& crc, FrameInfo** result = nullptr)
{
	FrameInfo* frame = new FrameInfo;

	try
	{
		if(!result)
		{
			// not inline, id
			frame->id = t.MustGetItemKeyword();
			crc.Update(frame->id);
			t.Next();

			// {
			t.AssertSymbol('{');
		}

		t.Next();

		bool have_simple = false;

		while(!t.IsSymbol('}'))
		{
			FrameKeyword k = (FrameKeyword)t.MustGetKeywordId(G_FRAME_KEYWORD);
			t.Next();

			switch(k)
			{
			case FK_ATTACKS:
				if(have_simple || frame->extra)
					t.Throw("Frame info already have attacks information.");
				t.AssertSymbol('{');
				t.Next();
				frame->extra = new AttackFrameInfo;
				frame->own_extra = true;
				crc.Update(0);
				do
				{
					t.AssertSymbol('{');
					t.Next();
					float start = t.MustGetNumberFloat();
					t.Next();
					float end = t.MustGetNumberFloat();
					t.Next();
					int flags = ReadFlags(t, G_WEAPON_FLAG);
					t.Next();
					t.AssertSymbol('}');
					crc.Update(start);
					crc.Update(end);
					crc.Update(flags);

					if(start < 0.f || start >= end || end > 1.f)
						t.Throw("Invalid attack frame times (%g %g).", start, end);

					frame->extra->e.push_back({ start, end, flags });
					t.Next();
				} while(!t.IsSymbol('}'));
				frame->attacks = frame->extra->e.size();
				crc.Update(frame->attacks);
				break;
			case FK_SIMPLE_ATTACKS:
				if(have_simple || frame->extra)
					t.Throw("Frame info already have attacks information.");
				else
				{
					t.AssertSymbol('{');
					t.Next();
					crc.Update(1);
					int index = 0;
					frame->attacks = 0;
					do
					{
						if(index == 3)
							t.Throw("To many simple attacks (max 3 for now).");
						t.AssertSymbol('{');
						t.Next();
						float start = t.MustGetNumberFloat();
						t.Next();
						float end = t.MustGetNumberFloat();
						t.Next();
						t.AssertSymbol('}');
						crc.Update(start);
						crc.Update(end);

						if(start < 0.f || start >= end || end > 1.f)
							t.Throw("Invalid attack frame times (%g %g).", start, end);

						frame->t[F_ATTACK1_START + index * 2] = start;
						frame->t[F_ATTACK1_END + index * 2] = end;
						++index;
						++frame->attacks;
						t.Next();
					} while(!t.IsSymbol('}'));
				}
				break;
			case FK_CAST:
				{
					float f = t.MustGetNumberFloat();
					if(!in_range(f, 0.f, 1.f))
						t.Throw("Invalid cast frame time %g.", f);
					frame->t[F_CAST] = f;
					crc.Update(2);
					crc.Update(f);
				}
				break;
			case FK_TAKE_WEAPON:
				{
					float f = t.MustGetNumberFloat();
					if(!in_range(f, 0.f, 1.f))
						t.Throw("Invalid take weapon frame time %g.", f);
					frame->t[F_TAKE_WEAPON] = f;
					crc.Update(3);
					crc.Update(f);
				}
				break;
			case FK_BASH:
				{
					float f = t.MustGetNumberFloat();
					if(!in_range(f, 0.f, 1.f))
						t.Throw("Invalid bash frame time %g.", f);
					frame->t[F_BASH] = f;
					crc.Update(4);
					crc.Update(f);
				}
				break;
			}

			t.Next();
		}

		// check if not exists
		if(!frame->id.empty())
		{
			for(FrameInfo* s : frame_infos)
			{
				if(s->id == frame->id)
					t.Throw("Frame info with that id already exists.");
			}
		}

		frame_infos.push_back(frame);
		if(result)
			*result = frame;
		return true;
	}
	catch(const Tokenizer::Exception& e)
	{
		if(!frame->id.empty())
			ERROR(Format("Failed to load frame info '%s': %s", frame->id.c_str(), e.ToString()));
		else
			ERROR(Format("Failed to load frame info: %s", frame->id.c_str(), e.ToString()));
		delete frame;
		return false;
	}
}

//=================================================================================================
bool LoadTex(Tokenizer& t, CRC32& crc, TexPack** result = nullptr)
{
	TexPack* tex = new TexPack;

	try
	{
		if(!result)
		{
			// not inline, id
			tex->id = t.MustGetItemKeyword();
			crc.Update(tex->id);
			t.Next();

			// {
			t.AssertSymbol('{');
		}

		t.Next();

		do
		{
			if(t.IsKeywordGroup(G_NULL))
			{
				tex->textures.push_back(TexId(nullptr));
				crc.Update(0);
			}
			else
			{
				const string& s = t.MustGetString();
				tex->textures.push_back(TexId(s));
				crc.Update(s);
			}
			t.Next();
		} while(!t.IsSymbol('}'));

		bool any = false;
		for(TexId& ti : tex->textures)
		{
			if(!ti.id.empty())
			{
				any = true;
				break;
			}
		}
		if(!any)
			t.Throw("Texture pack without textures.");

		if(!tex->id.empty())
		{
			for(TexPack* tp : tex_packs)
			{
				if(tp->id == tex->id)
					t.Throw("Texture pack with that id already exists.");
			}
		}

		tex_packs.push_back(tex);
		if(result)
			*result = tex;
		return true;
	}
	catch(const Tokenizer::Exception& e)
	{
		if(!tex->id.empty())
			ERROR(Format("Failed to load texture pack '%s': %s", tex->id.c_str(), e.ToString()));
		else
			ERROR(Format("Failed to load texture pack: %s", e.ToString()));
		delete tex;
		return false;
	}
}

//=================================================================================================
bool LoadIdles(Tokenizer& t, CRC32& crc, IdlePack** result = nullptr)
{
	IdlePack* idle = new IdlePack;

	try
	{
		if(!result)
		{
			// not inline, id
			idle->id = t.MustGetItemKeyword();
			crc.Update(idle->id);
			t.Next();

			// {
			t.AssertSymbol('{');
		}

		t.Next();
		
		do
		{
			const string& s = t.MustGetString();
			idle->anims.push_back(s);
			crc.Update(s);
			t.Next();
		} while(!t.IsSymbol('}'));
		
		if(!idle->id.empty())
		{
			for(IdlePack* ip : idle_packs)
			{
				if(ip->id == idle->id)
					t.Throw("Idle pack with this id already exists.");
			}
		}

		idle_packs.push_back(idle);
		if(result)
			*result = idle;
		return true;
	}
	catch(const Tokenizer::Exception& e)
	{
		if(!idle->id.empty())
			ERROR(Format("Failed to load idles '%s': %s", idle->id.c_str(), e.ToString()));
		else
			ERROR(Format("Failed to load idles: %s", e.ToString()));
		delete idle;
		return false;
	}
}

//=================================================================================================
bool LoadUnit(Tokenizer& t, CRC32& crc)
{
	UnitData* unit = new UnitData;

	try
	{
		// id
		unit->id = t.MustGetItemKeyword();
		crc.Update(unit->id);
		t.Next();

		// parent unit
		if(t.IsSymbol(':'))
		{
			t.Next();
			const string& s = t.MustGetItemKeyword();
			UnitData* parent = nullptr;
			for(UnitData* ud : unit_datas)
			{
				if(ud->id == s)
				{
					parent = ud;
					break;
				}
			}
			if(!parent)
				t.Throw("Missing parent unit '%s'.", s.c_str());
			crc.Update(s);
			unit->CopyFrom(*parent);
			t.Next();
		}

		// {
		t.AssertSymbol('{');
		t.Next();

		// properties
		while(!t.IsSymbol('}'))
		{
			Property p = (Property)t.MustGetKeywordId(G_PROPERTY);
			t.Next();

			switch(p)
			{
			case P_MESH:
				unit->mesh = t.MustGetString();
				crc.Update(unit->mesh);
				break;
			case P_MAT:
				unit->mat = (MATERIAL_TYPE)t.MustGetKeywordId(G_MATERIAL);
				crc.Update(unit->mat);
				break;
			case P_LEVEL:
				if(t.IsSymbol('{'))
				{
					t.Next();
					unit->level.x = t.MustGetInt();
					t.Next();
					unit->level.y = t.MustGetInt();
					if(unit->level.x >= unit->level.y || unit->level.x < 0)
						t.Throw("Invalid level {%d %d}.", unit->level.x, unit->level.y);
					t.Next();
					t.AssertSymbol('}');
					crc.Update(unit->level);
				}
				else
				{
					int lvl = t.MustGetInt();
					if(lvl < 0)
						t.Throw("Invalid level %d.", lvl);
					unit->level = INT2(lvl);
					crc.Update(lvl);
				}
				break;
			case P_PROFILE:
				if(t.IsSymbol('{'))
				{
					if(!LoadProfile(t, crc, &unit->stat_profile))
						t.Throw("Failed to load inline profile.");
				}
				else
				{
					const string& id = t.MustGetItemKeyword();
					unit->stat_profile = nullptr;
					for(StatProfile* p : stat_profiles)
					{
						if(id == p->id)
						{
							unit->stat_profile = p;
							break;
						}
					}
					if(!unit->stat_profile)
						t.Throw("Missing stat profile '%s'.", id.c_str());
					crc.Update(id);
				}
				break;
			case P_FLAGS:
				{
					bool clear = !t.IsSymbol('|');
					if(!clear)
						t.Next();
					ReadFlags(t, {
						{ &unit->flags, G_FLAGS },
						{ &unit->flags2, G_FLAGS2 },
						{ &unit->flags3, G_FLAGS3 }
					}, clear);
					crc.Update(unit->flags);
					crc.Update(unit->flags2);
					crc.Update(unit->flags3);
				}
				break;
			case P_HP:
				unit->hp_bonus = t.MustGetInt();
				if(unit->hp_bonus < 0)
					t.Throw("Invalid hp bonus %d.", unit->hp_bonus);
				crc.Update(unit->hp_bonus);
				break;
			case P_DEF:
				unit->def_bonus = t.MustGetInt();
				if(unit->def_bonus < 0)
					t.Throw("Invalid def bonus %d.", unit->def_bonus);
				crc.Update(unit->def_bonus);
				break;
			case P_ITEMS:
				if(t.IsSymbol('{'))
				{
					if(LoadItems(t, crc, &unit->item_script))
					{
						unit->items = &unit->item_script->code[0];
						unit->new_items = true;
					}
					else
						t.Throw("Failed to load inline item script.");
				}
				else
				{
					const string& id = t.MustGetItemKeyword();
					unit->item_script = nullptr;
					for(ItemScript* s : item_scripts)
					{
						if(s->id == id)
						{
							unit->item_script = s;
							unit->new_items = true;
							break;
						}
					}
					if(unit->item_script)
						unit->items = &unit->item_script->code[0];
					else
						t.Throw("Missing item script '%s'.", id.c_str());
					crc.Update(id);
				}
				break;
			case P_SPELLS:
				if(t.IsSymbol('{'))
				{
					if(!LoadSpells(t, crc, &unit->spells))
						t.Throw("Failed to load inline spell list.");
				}
				else
				{
					const string& id = t.MustGetItemKeyword();
					unit->spells = nullptr;
					for(SpellList* s : spell_lists)
					{
						if(s->id == id)
						{
							unit->spells = s;
							break;
						}
					}
					if(!unit->spells)
						t.Throw("Missing spell list '%s'.", id.c_str());
					crc.Update(id);
				}
				break;
			case P_GOLD:
				if(t.IsSymbol('{'))
				{
					t.Next();
					if(t.IsSymbol('{'))
					{
						t.Next();
						unit->gold.x = t.MustGetInt();
						t.Next();
						unit->gold.y = t.MustGetInt();
						t.Next();
						t.AssertSymbol('}');
						t.Next();
						t.AssertSymbol('{');
						t.Next();
						unit->gold2.x = t.MustGetInt();
						t.Next();
						unit->gold2.y = t.MustGetInt();
						t.Next();
						t.AssertSymbol('}');
						t.Next();
						t.AssertSymbol('}');
						if(unit->gold.x >= unit->gold.y || unit->gold.x < 0 || unit->gold2.x >= unit->gold2.y || unit->gold2.x < unit->gold.x || unit->gold2.y < unit->gold.y)
							t.Throw("Invalid gold count {{%d %d} {%d %d}}.", unit->gold.x, unit->gold.y, unit->gold2.x, unit->gold2.y);
						crc.Update(unit->gold);
						crc.Update(unit->gold2);
					}
					else
					{
						unit->gold.x = t.MustGetInt();
						t.Next();
						unit->gold.y = t.MustGetInt();
						t.Next();
						t.AssertSymbol('}');
						if(unit->gold.x >= unit->gold.y || unit->gold.x < 0)
							t.Throw("Invalid gold count {%d %d}.", unit->gold.x, unit->gold.y);
						unit->gold2 = unit->gold;
						crc.Update(unit->gold);
					}
				}
				else
				{
					int gold = t.MustGetInt();
					if(gold < 0)
						t.Throw("Invalid gold count %d.", gold);
					unit->gold = unit->gold2 = INT2(gold);
					crc.Update(gold);
				}
				break;
			case P_DIALOG:
				{
					const string& s = t.MustGetItemKeyword();
					auto it = dialogs_map.find(s);
					if(it == dialogs_map.end())
						t.Throw("Missing dialog '%s'.", s.c_str());
					unit->dialog = it->second;
					crc.Update(s);
				}
				break;
			case P_GROUP:
				unit->group = (UNIT_GROUP)t.MustGetKeywordId(G_GROUP);
				crc.Update(unit->group);
				break;
			case P_DMG_TYPE:
				unit->dmg_type = ReadFlags(t, G_DMG_TYPE);
				crc.Update(unit->dmg_type);
				break;
			case P_WALK_SPEED:
				unit->walk_speed = t.MustGetNumberFloat();
				if(unit->walk_speed < 0.5f)
					t.Throw("Invalid walk speed %g.", unit->walk_speed);
				crc.Update(unit->walk_speed);
				break;
			case P_RUN_SPEED:
				unit->run_speed = t.MustGetNumberFloat();
				if(unit->run_speed < 0)
					t.Throw("Invalid run speed %g.", unit->run_speed);
				crc.Update(unit->run_speed);
				break;
			case P_ROT_SPEED:
				unit->rot_speed = t.MustGetNumberFloat();
				if(unit->rot_speed < 0.5f)
					t.Throw("Invalid rot speed %g.", unit->rot_speed);
				crc.Update(unit->rot_speed);
				break;
			case P_BLOOD:
				unit->blood = (BLOOD)t.MustGetKeywordId(G_BLOOD);
				crc.Update(unit->blood);
				break;
			case P_SOUNDS:
				if(t.IsSymbol('{'))
				{
					if(!LoadSounds(t, crc, &unit->sounds))
						t.Throw("Failed to load inline sound pack.");
				}
				else
				{
					const string& id = t.MustGetItemKeyword();
					unit->sounds = nullptr;
					for(SoundPack* s : sound_packs)
					{
						if(s->id == id)
						{
							unit->sounds = s;
							break;
						}
					}
					if(!unit->sounds)
						t.Throw("Missing sound pack '%s'.", id.c_str());
					crc.Update(id);
				}
				break;
			case P_FRAMES:
				if(t.IsSymbol('{'))
				{
					if(!LoadFrames(t, crc, &unit->frames))
						t.Throw("Failed to load inline frame infos.");
				}
				else
				{
					const string& id = t.MustGetItemKeyword();
					unit->frames = nullptr;
					for(FrameInfo* fi : frame_infos)
					{
						if(fi->id == id)
						{
							unit->frames = fi;
							break;
						}
					}
					if(!unit->frames)
						t.Throw("Missing frame infos '%s'.", id.c_str());
					crc.Update(id);
				}
				break;
			case P_TEX:
				if(t.IsSymbol('{'))
				{
					TexPack* tex;
					if(!LoadTex(t, crc, &tex))
						t.Throw("Failed to load inline tex pack.");
					else
						unit->tex = &tex->textures;
				}
				else
				{
					const string& id = t.MustGetItemKeyword();
					TexPack* tex = nullptr;
					for(TexPack* tp : tex_packs)
					{
						if(tp->id == id)
						{
							tex = tp;
							break;
						}
					}
					if(tex)
						unit->tex = &tex->textures;
					else
						t.Throw("Missing tex pack '%s'.", id.c_str());
					crc.Update(id);
				}
				break;
			case P_IDLES:
				if(t.IsSymbol('{'))
				{
					IdlePack* idles;
					if(!LoadIdles(t, crc, &idles))
						t.Throw("Failed to load inline idles pack.");
					else
						unit->idles = &idles->anims;
				}
				else
				{
					const string& id = t.MustGetItemKeyword();
					IdlePack* idles = nullptr;
					for(IdlePack* ip : idle_packs)
					{
						if(ip->id == id)
						{
							idles = ip;
							break;
						}
					}
					if(idles)
						unit->idles = &idles->anims;
					else
						t.Throw("Missing idles pack '%s'.", id.c_str());
					crc.Update(id);
				}
				break;
			case P_WIDTH:
				unit->width = t.MustGetNumberFloat();
				if(unit->width < 0.1f)
					t.Throw("Invalid width %g.", unit->width);
				crc.Update(unit->width);
				break;
			case P_ATTACK_RANGE:
				unit->attack_range = t.MustGetNumberFloat();
				if(unit->attack_range < 0.1f)
					t.Throw("Invalid attack range %g.", unit->attack_range);
				crc.Update(unit->attack_range);
				break;
			case P_ARMOR_TYPE:
				unit->armor_type = (ArmorUnitType)t.MustGetKeywordId(G_ARMOR_TYPE);
				crc.Update(unit->armor_type);
				break;
			default:
				t.Unexpected();
				break;
			}

			t.Next();
		}

		// add
		for(UnitData* ud : unit_datas)
		{
			if(ud->id == unit->id)
				t.Throw("Unit with that id already exists.");
		}

		unit_datas.push_back(unit);
		return true;
	}
	catch(const Tokenizer::Exception& e)
	{
		ERROR(Format("Failed to load unit data '%s': %s", unit->id.c_str(), e.ToString()));
		delete unit;
		return false;
	}
}

//=================================================================================================
void LoadUnits(uint& out_crc)
{
	Tokenizer t(Tokenizer::F_UNESCAPE | Tokenizer::F_MULTI_KEYWORDS);
	if(!t.FromFile(Format("%s/units.txt", g_system_dir.c_str())))
		throw "Failed to open units.txt.";

	t.AddKeywords(G_TYPE, {
		{ "unit", T_UNIT },
		{ "items", T_ITEMS },
		{ "profile", T_PROFILE },
		{ "spells", T_SPELLS },
		{ "sounds", T_SOUNDS },
		{ "frames", T_FRAMES },
		{ "tex", T_TEX },
		{ "idles", T_IDLES }
	});

	t.AddKeywords(G_PROPERTY, {
		{ "mesh", P_MESH },
		{ "mat", P_MAT },
		{ "level", P_LEVEL },
		{ "profile", P_PROFILE },
		{ "flags", P_FLAGS },
		{ "hp", P_HP },
		{ "def", P_DEF },
		{ "items", P_ITEMS },
		{ "spells", P_SPELLS },
		{ "gold", P_GOLD },
		{ "dialog", P_DIALOG },
		{ "group", P_GROUP },
		{ "dmg_type", P_DMG_TYPE },
		{ "walk_speed", P_WALK_SPEED },
		{ "run_speed", P_RUN_SPEED },
		{ "rot_speed", P_ROT_SPEED },
		{ "blood", P_BLOOD },
		{ "sounds", P_SOUNDS },
		{ "frames", P_FRAMES },
		{ "tex", P_TEX },
		{ "idles", P_IDLES },
		{ "width", P_WIDTH },
		{ "attack_range", P_ATTACK_RANGE },
		{ "armor_type", P_ARMOR_TYPE }
	});

	t.AddKeywords(G_MATERIAL, {
		{ "wood", MAT_WOOD },
		{ "skin", MAT_SKIN },
		{ "metal", MAT_IRON },
		{ "crystal", MAT_CRYSTAL },
		{ "cloth", MAT_CLOTH },
		{ "rock", MAT_ROCK },
		{ "body", MAT_BODY },
		{ "bone", MAT_BONE }
	});

	t.AddKeywords(G_FLAGS, {
		{ "human", F_HUMAN },
		{ "humanoid", F_HUMANOID },
		{ "coward", F_COWARD },
		{ "dont_escape", F_DONT_ESCAPE },
		{ "archer", F_ARCHER },
		{ "leader", F_LEADER },
		{ "pierce_res25", F_PIERCE_RES25 },
		{ "slash_res25", F_SLASH_RES25 },
		{ "blunt_res25", F_BLUNT_RES25 },
		{ "pierce_weak25", F_PIERCE_WEAK25 },
		{ "slash_weak25", F_SLASH_WEAK25 },
		{ "blunt_weak25", F_BLUNT_WEAK25 },
		{ "undead", F_UNDEAD },
		{ "slow", F_SLOW },
		{ "poison_attack", F_POISON_ATTACK },
		{ "immortal", F_IMMORTAL },
		{ "tomashu", F_TOMASHU },
		{ "crazy", F_CRAZY },
		{ "dont_open", F_DONT_OPEN },
		{ "slight", F_SLIGHT },
		{ "secret", F_SECRET },
		{ "dont_suffer", F_DONT_SUFFER },
		{ "mage", F_MAGE },
		{ "poison_res", F_POISON_RES },
		{ "gray_hair", F_GRAY_HAIR },
		{ "no_power_attack", F_NO_POWER_ATTACK },
		{ "ai_clerk", F_AI_CLERK },
		{ "ai_guard", F_AI_GUARD },
		{ "ai_stay", F_AI_STAY },
		{ "ai_wanders", F_AI_WANDERS },
		{ "ai_drunkman", F_AI_DRUNKMAN },
		{ "hero", F_HERO }
	});

	t.AddKeywords(G_FLAGS2, {
		{ "ai_train", F2_AI_TRAIN },
		{ "specific_name", F2_SPECIFIC_NAME },
		{ "no_class", F2_NO_CLASS },
		{ "contest", F2_CONTEST },
		{ "contest_50", F2_CONTEST_50 },
		{ "class_flag", F2_CLASS_FLAG },
		{ "warrior", F2_WARRIOR },
		{ "hunter", F2_HUNTER },
		{ "rogue", F2_ROGUE },
		{ "old", F2_OLD },
		{ "melee", F2_MELEE },
		{ "melee_50", F2_MELEE_50 },
		{ "boss", F2_BOSS },
		{ "bloodless", F2_BLOODLESS },
		{ "limited_rot", F2_LIMITED_ROT },
		{ "cleric", F2_CLERIC },
		{ "update_v0_items", F2_UPDATE_V0_ITEMS },
		{ "sit_on_throne", F2_SIT_ON_THRONE },
		{ "orc_sounds", F2_ORC_SOUNDS },
		{ "goblin_sounds", F2_GOBLIN_SOUNDS },
		{ "xar", F2_XAR },
		{ "golem_sounds", F2_GOLEM_SOUNDS },
		{ "tournament", F2_TOURNAMENT },
		{ "yell", F2_YELL },
		{ "backstab", F2_BACKSTAB },
		{ "ignore_block", F2_IGNORE_BLOCK },
		{ "backstab_res", F2_BACKSTAB_RES },
		{ "magic_res50", F2_MAGIC_RES50 },
		{ "magic_res25", F2_MAGIC_RES25 },
		{ "mark", F2_MARK },
		{ "guarded", F2_GUARDED },
		{ "not_goblin", F2_NOT_GOBLIN }
	});

	t.AddKeywords(G_FLAGS3, {
		{ "contest_25", F3_CONTEST_25 },
		{ "drunk_mage", F3_DRUNK_MAGE },
		{ "drunkman_after_contest", F3_DRUNKMAN_AFTER_CONTEST },
		{ "dont_eat", F3_DONT_EAT },
		{ "orc_food", F3_ORC_FOOD },
		{ "miner", F3_MINER },
		{ "talk_at_competition", F3_TALK_AT_COMPETITION }
	});

	t.AddKeywords(G_GROUP, {
		{ "player", G_PLAYER },
		{ "team", G_TEAM },
		{ "citizens", G_CITIZENS },
		{ "goblins", G_GOBLINS },
		{ "orcs", G_ORCS },
		{ "undead", G_UNDEAD },
		{ "mages", G_MAGES },
		{ "animals", G_ANIMALS },
		{ "crazies", G_CRAZIES },
		{ "bandits", G_BANDITS }
	});

	t.AddKeywords(G_DMG_TYPE, {
		{ "slash", DMG_SLASH },
		{ "pierce", DMG_PIERCE },
		{ "blunt", DMG_BLUNT }
	});

	t.AddKeywords(G_BLOOD, {
		{ "red", BLOOD_RED },
		{ "green", BLOOD_GREEN },
		{ "black", BLOOD_BLACK },
		{ "bone", BLOOD_BONE },
		{ "rock", BLOOD_ROCK },
		{ "iron", BLOOD_IRON }
	});

	t.AddKeywords(G_ARMOR_TYPE, {
		{ "human", (int)ArmorUnitType::HUMAN },
		{ "goblin", (int)ArmorUnitType::GOBLIN },
		{ "orc", (int)ArmorUnitType::ORC },
		{ "none", (int)ArmorUnitType::NONE }
	});

	t.AddKeywords(G_ATTRIBUTE, {
		{ "strength", (int)Attribute::STR },
		{ "endurance", (int)Attribute::END },
		{ "dexterity", (int)Attribute::DEX },
		{ "inteligence", (int)Attribute::INT },
		{ "wisdom", (int)Attribute::WIS },
		{ "charisma", (int)Attribute::CHA }
	});

	t.AddKeywords(G_SKILL, {
		{ "one_handed", (int)Skill::ONE_HANDED_WEAPON },
		{ "short_blade", (int)Skill::SHORT_BLADE },
		{ "long_blade", (int)Skill::LONG_BLADE },
		{ "blunt", (int)Skill::BLUNT },
		{ "axe", (int)Skill::AXE },
		{ "bow", (int)Skill::BOW },
		{ "unarmed", (int)Skill::UNARMED },
		{ "shield", (int)Skill::SHIELD },
		{ "light_armor", (int)Skill::LIGHT_ARMOR },
		{ "medium_armor", (int)Skill::MEDIUM_ARMOR },
		{ "heavy_armor", (int)Skill::HEAVY_ARMOR },
		{ "nature_magic", (int)Skill::NATURE_MAGIC },
		{ "gods_magic", (int)Skill::GODS_MAGIC },
		{ "mystic_magic", (int)Skill::MYSTIC_MAGIC },
		{ "spellcraft", (int)Skill::SPELLCRAFT },
		{ "concentration", (int)Skill::CONCENTRATION },
		{ "identification", (int)Skill::IDENTIFICATION },
		{ "lockpick", (int)Skill::LOCKPICK },
		{ "sneak", (int)Skill::SNEAK },
		{ "traps", (int)Skill::TRAPS },
		{ "steal", (int)Skill::STEAL },
		{ "animal_empathy", (int)Skill::ANIMAL_EMPATHY },
		{ "survival", (int)Skill::SURVIVAL },
		{ "persuasion", (int)Skill::PERSUASION },
		{ "alchemy", (int)Skill::ALCHEMY },
		{ "crafting", (int)Skill::CRAFTING },
		{ "healing", (int)Skill::HEALING },
		{ "athletics", (int)Skill::ATHLETICS },
		{ "rage", (int)Skill::RAGE }
	});

	t.AddKeyword("fixed", PK_FIXED, G_PROFILE_KEYWORD);

	t.AddKeywords(G_SOUND_TYPE, {
		{ "see_enemy", ST_SEE_ENEMY },
		{ "pain", ST_PAIN },
		{ "death", ST_DEATH },
		{ "attack", ST_ATTACK }
	});

	t.AddKeywords(G_FRAME_KEYWORD, {
		{ "attacks", FK_ATTACKS },
		{ "simple_attacks", FK_SIMPLE_ATTACKS },
		{ "cast", FK_CAST },
		{ "take_weapon", FK_TAKE_WEAPON },
		{ "bash", FK_BASH }
	});

	t.AddKeywords(G_WEAPON_FLAG, {
		{ "short_blade", A_SHORT_BLADE },
		{ "long_blade", A_LONG_BLADE },
		{ "blunt", A_BLUNT },
		{ "axe", A_AXE }
	});

	t.AddKeyword("null", 0, G_NULL);

	t.AddKeywords(G_SPELL, {
		{ "magic_bolt", 0 },
		{ "xmagic_bolt", 1 },
		{ "fireball", 2 },
		{ "split_poison", 3 },
		{ "raise", 4 },
		{ "thunder_bolt", 5 },
		{ "drain", 6 },
		{ "drain2", 7 },
		{ "exploding_skull", 8 },
		{ "heal", 9 },
		{ "mystic_ball", 10 }
	});

	t.AddKeywords(G_SPELL_KEYWORD, {
		{ "non_combat", SK_NON_COMBAT },
		{ "null", SK_NULL }
	});

	t.AddKeywords(G_ITEM_KEYWORD, {
		{ "chance", IK_CHANCE },
		{ "random", IK_RANDOM },
		{ "if", IK_IF },
		{ "else", IK_ELSE },
		{ "level", IK_LEVEL }
	});

	dialogs_map["dialog_kowal"] = dialog_kowal;
	dialogs_map["dialog_kupiec"] = dialog_kupiec;
	dialogs_map["dialog_alchemik"] = dialog_alchemik;
	dialogs_map["dialog_burmistrz"] = dialog_burmistrz;
	dialogs_map["dialog_mieszkaniec"] = dialog_mieszkaniec;
	dialogs_map["dialog_widz"] = dialog_widz;
	dialogs_map["dialog_straznik"] = dialog_straznik;
	dialogs_map["dialog_trener"] = dialog_trener;
	dialogs_map["dialog_dowodca_strazy"] = dialog_dowodca_strazy;
	dialogs_map["dialog_karczmarz"] = dialog_karczmarz;
	dialogs_map["dialog_urzednik"] = dialog_urzednik;
	dialogs_map["dialog_mistrz_areny"] = dialog_mistrz_areny;
	dialogs_map["dialog_hero"] = dialog_hero;
	dialogs_map["dialog_hero_przedmiot"] = dialog_hero_przedmiot;
	dialogs_map["dialog_hero_przedmiot_kup"] = dialog_hero_przedmiot_kup;
	dialogs_map["dialog_hero_pvp"] = dialog_hero_pvp;
	dialogs_map["dialog_szalony"] = dialog_szalony;
	dialogs_map["dialog_szalony_przedmiot"] = dialog_szalony_przedmiot;
	dialogs_map["dialog_szalony_przedmiot_kup"] = dialog_szalony_przedmiot_kup;
	dialogs_map["dialog_szalony_pvp"] = dialog_szalony_pvp;
	dialogs_map["dialog_bandyci"] = dialog_bandyci;
	dialogs_map["dialog_bandyta"] = dialog_bandyta;
	dialogs_map["dialog_szalony_mag"] = dialog_szalony_mag;
	dialogs_map["dialog_porwany"] = dialog_porwany;
	dialogs_map["dialog_straznicy_nagroda"] = dialog_straznicy_nagroda;
	dialogs_map["dialog_zadanie"] = dialog_zadanie;
	dialogs_map["dialog_artur_drwal"] = dialog_artur_drwal;
	dialogs_map["dialog_drwal"] = dialog_drwal;
	dialogs_map["dialog_inwestor"] = dialog_inwestor;
	dialogs_map["dialog_gornik"] = dialog_gornik;
	dialogs_map["dialog_pijak"] = dialog_pijak;
	dialogs_map["dialog_q_bandyci"] = dialog_q_bandyci;
	dialogs_map["dialog_q_magowie"] = dialog_q_magowie;
	dialogs_map["dialog_q_magowie2"] = dialog_q_magowie2;
	dialogs_map["dialog_q_orkowie"] = dialog_q_orkowie;
	dialogs_map["dialog_q_orkowie2"] = dialog_q_orkowie2;
	dialogs_map["dialog_q_gobliny"] = dialog_q_gobliny;
	dialogs_map["dialog_q_zlo"] = dialog_q_zlo;
	dialogs_map["dialog_tut_czlowiek"] = dialog_tut_czlowiek;
	dialogs_map["dialog_q_szaleni"] = dialog_q_szaleni;
	dialogs_map["dialog_szaleni"] = dialog_szaleni;
	dialogs_map["dialog_tomashu"] = dialog_tomashu;
	dialogs_map["dialog_ochroniarz"] = dialog_ochroniarz;
	dialogs_map["dialog_mag_obstawa"] = dialog_mag_obstawa;
	dialogs_map["dialog_sprzedawca_jedzenia"] = dialog_sprzedawca_jedzenia;

	int errors = 0;
	CRC32 crc;

	try
	{
		t.Next();

		while(!t.IsEof())
		{
			bool skip = false;

			if(t.IsKeywordGroup(G_TYPE))
			{
				Type type = (Type)t.MustGetKeywordId(G_TYPE);
				t.Next();

				bool ok = true;

				switch(type)
				{
				case T_UNIT:
					if(!LoadUnit(t, crc))
						ok = false;
					break;
				case T_PROFILE:
					if(!LoadProfile(t, crc))
						ok = false;
					break;
				case T_ITEMS:
					if(!LoadItems(t, crc))
						ok = false;
					break;
				case T_SPELLS:
					if(!LoadSpells(t, crc))
						ok = false;
					break;
				case T_SOUNDS:
					if(!LoadSounds(t, crc))
						ok = false;
					break;
				case T_FRAMES:
					if(!LoadFrames(t, crc))
						ok = false;
					break;
				case T_TEX:
					if(!LoadTex(t, crc))
						ok = false;
					break;
				case T_IDLES:
					if(!LoadIdles(t, crc))
						ok = false;
					break;
				default:
					ERROR(Format("Invalid type %d.", type));
					ok = false;
					break;
				}

				if(!ok)
				{
					++errors;
					skip = true;
				}
			}
			else
			{
				int group = G_TYPE;
				ERROR(t.FormatUnexpected(Tokenizer::T_KEYWORD_GROUP, &group));
				++errors;
				skip = true;
			}

			if(skip)
				t.SkipToKeywordGroup(G_TYPE);
			else
				t.Next();
		}
	}
	catch(const Tokenizer::Exception& e)
	{
		ERROR(Format("Failed to load items: %s", e.ToString()));
		++errors;
	}

	if(errors > 0)
		throw Format("Failed to load units (%d errors), check log for details.", errors);

	out_crc = crc.Get();
}

//=================================================================================================
void InitUnits()
{
	for(UnitData& ud : g_base_units)
	{
		if(!ud.stat_profile)
			ud.stat_profile = &g_stat_profiles[(int)ud.profile];
		if(ud.spells)
		{
			for(int j = 0; j<3; ++j)
			{
				if(ud.spells->name[j])
					ud.spells->spell[j] = FindSpell(ud.spells->name[j]);
			}
		}
	}
}

//=================================================================================================
void ClearUnits()
{

}

//=================================================================================================
void TestUnits()
{
	int errors = 0;

	for(UnitData& ud : g_base_units)
	{
		UnitData* find = nullptr;
		for(UnitData* u : unit_datas)
		{
			if(u->id == ud.id)
			{
				find = u;
				break;
			}
		}

		if(!find)
		{
			ERROR(Format("Missing unit '%s'.", ud.id.c_str()));
			continue;
		}

		UnitData& ud2 = *find;
		
		if(ud.mesh != ud2.mesh)
		{
			ERROR(Format("Unit '%s': invalid mesh (%s, %s).", ud.id.c_str(), ud.mesh.c_str(), ud2.mesh.c_str()));
			++errors;
		}
		if(ud.mat != ud2.mat)
		{
			ERROR(Format("Unit '%s': invalid mat (%d, %d).", ud.id.c_str(), ud.mat, ud2.mat));
			++errors;
		}
		if(ud.level != ud2.level)
		{
			ERROR(Format("Unit '%s': invalid level ((%d,%d), (%d,%d)).", ud.id.c_str(), ud.level.x, ud.level.y, ud2.level.x, ud2.level.y));
			++errors;
		}
		if(!ud.stat_profile != !ud2.stat_profile)
		{
			ERROR(Format("Unit '%s': invalid profile (%p, %p).", ud.stat_profile, ud2.stat_profile));
			++errors;
		}
		else if(*ud.stat_profile != *ud2.stat_profile)
		{
			ERROR(Format("Unit '%s': invalid profile data.", ud.id.c_str()));
			++errors;
		}
		if(ud.hp_bonus != ud2.hp_bonus)
		{
			ERROR(Format("Unit '%s': invalid hp (%d, %d).", ud.id.c_str(), ud.hp_bonus, ud2.hp_bonus));
			++errors;
		}
		if(ud.def_bonus != ud2.def_bonus)
		{
			ERROR(Format("Unit '%s': invalid def (%d, %d).", ud.id.c_str(), ud.def_bonus, ud2.def_bonus));
			++errors;
		}
		if(ud.dmg_type != ud2.dmg_type)
		{
			ERROR(Format("Unit '%s': invalid dmg type (%d, %d).", ud.id.c_str(), ud.dmg_type, ud2.dmg_type));
			++errors;
		}
		if(ud.flags != ud2.flags)
		{
			ERROR(Format("Unit '%s': invalid flags (%d, %d).", ud.id.c_str(), ud.flags, ud2.flags));
			++errors;
		}
		if(ud.flags2 != ud2.flags2)
		{
			ERROR(Format("Unit '%s': invalid flags2 (%d, %d).", ud.id.c_str(), ud.flags2, ud2.flags2));
			++errors;
		}
		if(ud.flags3 != ud2.flags3)
		{
			ERROR(Format("Unit '%s': invalid flags3 (%d, %d).", ud.id.c_str(), ud.flags3, ud2.flags3));
			++errors;
		}
		if(ud.items != ud2.items)
		{
			if(!ud.items != !ud2.items)
			{
				ERROR(Format("Unit '%s': invalid items (%p, %p).", ud.id.c_str(), ud.items, ud2.items));
				++errors;
			}
			else
			{
				uint crc1, crc2;
				string e;
				uint e_count = 0;
				TestItemScript(ud.items, e, e_count, ud.new_items, crc1);
				TestItemScript(ud2.items, e, e_count, ud2.new_items, crc2);
				if(crc1 != crc2)
				{
					LogItemScript(ud.items, ud.new_items);
					LogItemScript(ud2.items, ud2.new_items);
					ERROR(Format("Unit '%s' invalid items data (%u %u).", ud.id.c_str(), crc1, crc2));
					++errors;
				}
			}
		}
		if(ud.spells != ud2.spells)
		{
			if(!ud.spells != !ud2.spells)
			{
				ERROR(Format("Unit '%s': invalid spells (%p, %p).", ud.id.c_str(), ud.spells, ud2.spells));
				++errors;
			}
			else if(*ud.spells != *ud2.spells)
			{
				ERROR(Format("Unit '%s': invalid spells data.", ud.id.c_str()));
			}
		}
		if(ud.gold != ud2.gold)
		{
			ERROR(Format("Unit '%s': invalid gold ((%d,%d), (%d,%d)).", ud.id.c_str(), ud.gold.x, ud.gold.y, ud2.gold.x, ud2.gold.y));
			++errors;
		}
		if(ud.gold2 != ud2.gold2)
		{
			ERROR(Format("Unit '%s': invalid gold2 ((%d,%d), (%d,%d)).", ud.id.c_str(), ud.gold2.x, ud.gold2.y, ud2.gold2.x, ud2.gold2.y));
			++errors;
		}
		if(!ud.dialog != !ud2.dialog)
		{
			ERROR(Format("Unit '%s': invalid dialog (%p, %p).", ud.id.c_str(), ud.dialog, ud2.dialog));
			++errors;
		}
		if(ud.group != ud2.group)
		{
			ERROR(Format("Unit '%s': invalid group (%d, %d).", ud.id.c_str(), ud.group, ud2.group));
			++errors;
		}
		if(ud.walk_speed != ud2.walk_speed)
		{
			ERROR(Format("Unit '%s': invalid walk speed (%g, %g).", ud.id.c_str(), ud.walk_speed, ud2.walk_speed));
			++errors;
		}
		if(ud.run_speed != ud2.run_speed)
		{
			ERROR(Format("Unit '%s': invalid run speed (%g, %g).", ud.id.c_str(), ud.run_speed, ud2.run_speed));
			++errors;
		}
		if(ud.rot_speed != ud2.rot_speed)
		{
			ERROR(Format("Unit '%s': invalid rot speed (%g, %g).", ud.id.c_str(), ud.rot_speed, ud2.rot_speed));
			++errors;
		}
		if(ud.width != ud2.width)
		{
			ERROR(Format("Unit '%s': invalid width (%g, %g).", ud.id.c_str(), ud.width, ud2.width));
			++errors;
		}
		if(ud.attack_range != ud2.attack_range)
		{
			ERROR(Format("Unit '%s': invalid attack range (%g, %g).", ud.id.c_str(), ud.attack_range, ud2.attack_range));
			++errors;
		}
		if(ud.blood != ud2.blood)
		{
			ERROR(Format("Unit '%s': invalid blood (%d, %d).", ud.id.c_str(), ud.blood, ud2.blood));
			++errors;
		}
		if(!ud.sounds != !ud2.sounds)
		{
			ERROR(Format("Unit '%s': invalid sounds (%p, %p).", ud.id.c_str(), ud.sounds, ud2.sounds));
			++errors;
		}
		else if(*ud.sounds != *ud2.sounds)
		{
			ERROR(Format("Unit '%s': invalid sounds data.", ud.id.c_str()));
			++errors;
		}
		if(!ud.frames != !ud2.frames)
		{
			ERROR(Format("Unit '%s': invalid frames (%p, %p).", ud.id.c_str(), ud.frames, ud2.frames));
			++errors;
		}
		else if(*ud.frames != *ud2.frames)
		{
			ERROR(Format("Unit '%s': invalid frames data.", ud.id.c_str()));
			++errors;
		}
		if(!ud.tex != !ud2.tex)
		{
			ERROR(Format("Unit '%s': invalid tex (%p, %p).", ud.id.c_str(), ud.tex, ud2.tex));
			++errors;
		}
		else if(ud.tex != ud2.tex)
		{
			if(ud.tex->size() != ud2.tex->size())
			{
				ERROR(Format("Unit '%s': invalid tex data.", ud.id.c_str()));
				++errors;
			}
			else
			{
				for(uint i = 0; i < ud.tex->size(); ++i)
				{
					if(ud.tex->at(i).id != ud2.tex->at(i).id)
					{
						ERROR(Format("Unit '%s': invalid tex data.", ud.id.c_str()));
						++errors;
						break;
					}
				}
			}
		}
		if(!ud.idles != !ud2.idles)
		{
			ERROR(Format("Unit '%s': invalid idles (%p, %p).", ud.id.c_str(), ud.idles, ud2.idles));
			++errors;
		}
		else if(ud.idles != ud2.idles)
		{
			if(ud.idles->size() != ud2.idles->size())
			{
				ERROR(Format("Unit '%s': invalid idles data.", ud.id.c_str()));
				++errors;
			}
			else
			{
				for(uint i = 0; i < ud.idles->size(); ++i)
				{
					if(ud.idles->at(i) != ud2.idles->at(i))
					{
						ERROR(Format("Unit '%s': invalid idles data.", ud.id.c_str()));
						++errors;
						break;
					}
				}
			}
		}
		if(ud.armor_type != ud2.armor_type)
		{
			ERROR(Format("Unit '%s': invalid armor type (%d, %d).", ud.id.c_str(), ud.armor_type, ud2.armor_type));
			++errors;
		}
	}

	if(errors > 0)
		throw Format("Failed to validated units with old data, %d errors.", errors);
}

#undef S
#define S(x) (*(cstring*)(x))

//=================================================================================================
void CheckItem(const int*& ps, string& errors, uint& count, bool is_new)
{
	if(!is_new)
	{
		ItemListResult lis;
		const Item* item = FindItem(S(ps), false, &lis);
		if(!item && !lis.lis)
		{
			errors += Format("\tMissing item '%s'.\n", S(ps));
			++count;
		}
	}
	else
	{
		ParseScript type = (ParseScript)*ps;
		++ps;

		if(type == PS_ITEM || type == PS_LIST || type == PS_LEVELED_LIST)
		{
			if(*ps == 0)
			{
				errors += Format("\tMissing new item value %p.\n", *ps);
				++count;
			}
		}
		else if(type == PS_LEVELED_LIST_MOD)
		{
			int count = *ps;
			if(count == 0 || count > 9 || count < -9)
			{
				errors += Format("\tInvalid leveled list mod %d.\n", count);
				++count;
			}
			++ps;
			if(*ps == 0)
			{
				errors += Format("\tMissing leveled list value %p.\n", *ps);
				++count;
			}
		}
		else
		{
			errors += Format("\tMissing new item declaration.\n");
			++count;
		}
	}

	++ps;
}

//=================================================================================================
void TestItemScript(const int* script, string& errors, uint& count, bool is_new, uint& crc)
{
	assert(script);

	const int* ps = script;
	int a, b, depth = 0, elsel = 0;
	crc = 0;

	while(*ps != PS_END)
	{
		ParseScript type = (ParseScript)*ps;
		++ps;

		switch(type)
		{
		case PS_ONE:
			CheckItem(ps, errors, count, is_new);
			crc += 1;
			break;
		case PS_ONE_OF_MANY:
			if((a = *ps) == 0)
			{
				errors += "\tOne from many: 0.\n";
				++count;
			}
			else
			{
				crc += (a << 2);
				++ps;
				for(int i = 0; i<a; ++i)
					CheckItem(ps, errors, count, is_new);
			}
			break;
		case PS_CHANCE:
			a = *ps;
			if(a <= 0 || a >= 100)
			{
				errors += Format("\tChance %d.\n", a);
				++count;
			}
			crc += (a << 6);
			++ps;
			CheckItem(ps, errors, count, is_new);
			break;
		case PS_CHANCE2:
			a = *ps;
			if(a <= 0 || a >= 100)
			{
				errors += Format("\tChance2 %d.\n", a);
				++count;
			}
			++ps;
			crc += (a << 10);
			CheckItem(ps, errors, count, is_new);
			CheckItem(ps, errors, count, is_new);
			break;
		case PS_IF_CHANCE:
			a = *ps;
			if(a <= 0 || a >= 100)
			{
				errors += Format("\tIf chance %d.\n", a);
				++count;
			}
			CLEAR_BIT(elsel, BIT(depth));
			crc += (a << 14);
			++depth;
			++ps;
			break;
		case PS_IF_LEVEL:
			a = *ps;
			if(a <= 1)
			{
				errors += Format("\tLevel %d.\n", a);
				++count;
			}
			CLEAR_BIT(elsel, BIT(depth));
			crc += (a << 18);
			++depth;
			++ps;
			break;
		case PS_ELSE:
			if(depth == 0)
			{
				errors += "\tElse without if.\n";
				++count;
			}
			else
			{
				if(IS_SET(elsel, BIT(depth - 1)))
				{
					errors += "\tNext else.\n";
					++count;
				}
				else
					SET_BIT(elsel, BIT(depth - 1));
			}
			crc += (1 << 22);
			break;
		case PS_END_IF:
			if(depth == 0)
			{
				errors += "\tEnd if without if.\n";
				++count;
			}
			else
				--depth;
			crc += (1 << 23);
			break;
		case PS_MANY:
			a = *ps;
			if(a < 0)
			{
				errors += Format("\tGive some %d.\n", a);
				++count;
			}
			++ps;
			CheckItem(ps, errors, count, is_new);
			crc += (a << 24);
			break;
		case PS_RANDOM:
			a = *ps;
			++ps;
			b = *ps;
			++ps;
			if(a < 0 || b < 0 || a >= b)
			{
				errors += Format("\tGive random %d, %d.", a, b);
				++count;
			}
			CheckItem(ps, errors, count, is_new);
			crc += (a << 28);
			crc += (b << 30);
			break;
		}
	}

	if(depth != 0)
	{
		errors += Format("\tUnclosed ifs %d.\n", depth);
		++count;
	}
}

//=================================================================================================
void LogItem(string& s, const int*& ps, bool is_new)
{
	if(!is_new)
		s += S(ps);
	else
	{
		ParseScript type = (ParseScript)*ps;
		++ps;

		switch(type)
		{
		case PS_ITEM:
			s += ((const Item*)(*ps))->id;
			break;
		case PS_LIST:
			s += Format("!%s", ((ItemList*)(*ps))->id.c_str());
			break;
		case PS_LEVELED_LIST:
			s += Format("!%s", ((LeveledItemList*)(*ps))->id.c_str());
			break;
		case PS_LEVELED_LIST_MOD:
			{
				int mod = *ps;
				++ps;
				s += Format("!%+d%s", mod, ((LeveledItemList*)(*ps))->id.c_str());
			}
			break;
		default:
			assert(0);
			break;
		}
	}

	++ps;
}

//=================================================================================================
void LogItemScript(const int* script, bool is_new)
{
	assert(script);

	const int* ps = script;
	int a, b, depth = 0, elsel = 0;
	string s = "Item script:\n";

	while(*ps != PS_END)
	{
		ParseScript type = (ParseScript)*ps;
		++ps;

		for(int i = 0; i < depth; ++i)
			s += "  ";

		switch(type)
		{
		case PS_ONE:
			s += "one ";
			LogItem(s, ps, is_new);
			s += "\n";
			break;
		case PS_ONE_OF_MANY:
			a = *ps;
			++ps;
			s += Format("one_of_many %d [", a);
			for(int i = 0; i<a; ++i)
			{
				LogItem(s, ps, is_new);
				s += "; ";
			}
			s += "]\n";
			break;
		case PS_CHANCE:
			a = *ps;
			++ps;
			s += Format("chance %d ", a);
			LogItem(s, ps, is_new);
			s += "\n";
			break;
		case PS_CHANCE2:
			a = *ps;
			++ps;
			s += Format("chance2 %d [", a);
			LogItem(s, ps, is_new);
			s += "; ";
			LogItem(s, ps, is_new);
			s += "]\n";
			break;
		case PS_IF_CHANCE:
			a = *ps;
			s += Format("if chance %d\n", a);
			CLEAR_BIT(elsel, BIT(depth));
			++depth;
			++ps;
			break;
		case PS_IF_LEVEL:
			a = *ps;
			CLEAR_BIT(elsel, BIT(depth));
			s += Format("if level %d\n", a);
			++depth;
			++ps;
			break;
		case PS_ELSE:
			SET_BIT(elsel, BIT(depth - 1));
			s.pop_back();
			s.pop_back();
			s += "else\n";
			break;
		case PS_END_IF:
			s.pop_back();
			s.pop_back();
			s += "end_if\n";
			--depth;
			break;
		case PS_MANY:
			a = *ps;
			++ps;
			s += Format("many %d ", a);
			LogItem(s, ps, is_new);
			s += "\n";
			break;
		case PS_RANDOM:
			a = *ps;
			++ps;
			b = *ps;
			++ps;
			s += Format("random %d %d ", a, b);
			LogItem(s, ps, is_new);
			s += "\n";
			break;
		}
	}

	LOG(s.c_str());
}

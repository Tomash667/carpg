#include "Pch.h"
#include "Base.h"
#include "UnitData.h"
#include "ItemScript.h"
#include "Dialog.h"
#include "DamageTypes.h"

//=================================================================================================
// PRZEDMIOTY POSTACI
//=================================================================================================
#define S(x) ((int)x)

int base_warrior_items[] = {
	PS_DAJ_KILKA, 4, S("p_hp"),
	PS_DAJ_KILKA, 2, S("p_nreg"),
	PS_KONIEC
};

int base_hunter_items[] = {
	PS_DAJ_KILKA, 2, S("p_hp"),
	PS_DAJ_KILKA, 3, S("p_nreg"),
	PS_KONIEC
};

int base_rogue_items[] = {
	PS_DAJ_KILKA, 3, S("p_hp"),
	PS_DAJ_KILKA, 2, S("p_nreg"),
	PS_KONIEC
};

int blacksmith_items[] = {
	PS_DAJ, S("al_blacksmith"),
	PS_DAJ, S("blunt_blacksmith"),
	PS_LOSOWO, 1, 3, S("!normal_food"),
	PS_KONIEC
};

int merchant_items[] = {
	PS_JEDEN_Z_WIELU, 2, S("al_clothes2"), S("al_clothes2b"),
	PS_DAJ, S("dagger_sword"),
	PS_LOSOWO, 1, 3, S("!normal_food"),
	PS_KONIEC
}; 

int alchemist_items[] = {
	PS_JEDEN_Z_WIELU, 2, S("al_clothes2"), S("al_clothes2b"),
	PS_DAJ, S("dagger_short"),
	PS_DAJ, S("p_hp"),
	PS_DAJ, S("ladle"),
	PS_LOSOWO, 1, 3, S("!normal_food"),
	PS_KONIEC
};

int goblin_items[] = {
	PS_SZANSA, 20, S("bow_short"),
	PS_SZANSA, 20, S("shield_wood"),
	PS_SZANSA, 5, S("p_hp"),
	PS_JEDEN_Z_WIELU, 3, S("dagger_short"), S("axe_small"), S("blunt_club"),
	PS_SZANSA, 25, S("al_goblin"),
	PS_LOSOWO, 0, 2, S("!orc_food"),
	PS_KONIEC
};

int tut_goblin_items[] = {
	PS_SZANSA, 20, S("shield_wood"),
	PS_JEDEN_Z_WIELU, 3, S("dagger_short"), S("axe_small"), S("blunt_club"),
	PS_SZANSA, 25, S("al_goblin"),
	PS_LOSOWO, 0, 2, S("!orc_food"),
	PS_KONIEC
};

int goblin_hunter_items[] = {
	PS_POZIOM, 5,
		PS_DAJ, S("bow_long"),
	PS_ELSE,
		PS_DAJ, S("bow_short"),
	PS_END_IF,
	PS_SZANSA, 10, S("p_hp"),
	PS_JEDEN_Z_WIELU, 3, S("dagger_short"), S("axe_small"), S("blunt_club"),
	PS_SZANSA, 50, S("al_goblin"),
	PS_SZANSA, 25, S("shield_wood"),
	PS_LOSOWO, 0, 2, S("!orc_food"),
	PS_KONIEC
};

int goblin_fighter_items[] = {
	PS_POZIOM, 5,
		PS_SZANSA2, 25, S("shield_iron"), S("shield_wood"),
		PS_JEDEN_Z_WIELU, 4, S("dagger_sword"), S("axe_small"), S("blunt_club"), S("sword_long"),
	PS_ELSE,
		PS_SZANSA, 75, S("shield_wood"),
		PS_JEDEN_Z_WIELU, 3, S("dagger_short"), S("axe_small"), S("blunt_club"),
	PS_END_IF,
	PS_SZANSA, 15, S("p_hp"),
	PS_DAJ, S("al_goblin"),
	PS_LOSOWO, 0, 2, S("!orc_food"),
	PS_KONIEC
};

int goblin_chief_items[] = {
	PS_SZANSA, 25, S("p_hp"),
	PS_DAJ, S("al_goblin"),
	PS_SZANSA2, 75, S("shield_iron"), S("shield_wood"),
	PS_JEDEN_Z_WIELU, 5, S("dagger_sword"), S("axe_small"), S("blunt_mace"), S("sword_long"), S("sword_scimitar"),
	PS_LOSOWO, 1, 3, S("!orc_food"),
	PS_KONIEC
};

int orc_items[] = {
	PS_SZANSA, 10, S("p_hp"),
	PS_SZANSA, 33, S("al_orc"),
	PS_SZANSA, 75, S("shield_wood"),
	PS_JEDEN_Z_WIELU, 3, S("blunt_mace"), S("axe_small"), S("sword_long"),
	PS_LOSOWO, 1, 3, S("!orc_food"),
	PS_KONIEC
};

int orc_hunter_items[] = {
	PS_POZIOM, 7,
		PS_DAJ, S("bow_long"),
		PS_DAJ, S("shield_wood"),
		PS_IF_SZANSA, 50,
			PS_JEDEN_Z_WIELU, 3, S("blunt_orcish"), S("axe_orcish"), S("sword_orcish"),
		PS_ELSE,
			PS_JEDEN_Z_WIELU, 3, S("blunt_mace"), S("axe_small"), S("sword_long"),
		PS_END_IF,
	PS_ELSE,
		PS_SZANSA2, 50, S("bow_long"), S("bow_short"),
		PS_JEDEN_Z_WIELU, 3, S("blunt_mace"), S("axe_small"), S("sword_long"),
		PS_SZANSA, 75, S("shield_wood"),
	PS_END_IF,
	PS_SZANSA, 15, S("p_hp"),
	PS_SZANSA, 66, S("al_orc"),
	PS_LOSOWO, 1, 3, S("!orc_food"),
	PS_KONIEC
};

int orc_fighter_items[] = {
	PS_POZIOM, 7,
		PS_SZANSA2, 50, S("am_orc"), S("al_orc"),
		PS_JEDEN_Z_WIELU, 3, S("blunt_orcish"), S("axe_orcish"), S("sword_orcish"),
		PS_DAJ, S("shield_iron"),
	PS_ELSE,
		PS_IF_SZANSA, 50,
			PS_JEDEN_Z_WIELU, 3, S("blunt_orcish"), S("axe_orcish"), S("sword_orcish"),
			PS_DAJ, S("shield_iron"),
		PS_ELSE,
			PS_JEDEN_Z_WIELU, 3, S("blunt_mace"), S("axe_small"), S("sword_long"),
			PS_SZANSA2, 50, S("shield_iron"), S("shield_wood"),
		PS_END_IF,
		PS_SZANSA2, 20, S("am_orc"), S("al_orc"),
	PS_END_IF,
	PS_SZANSA, 25, S("p_hp"),
	PS_LOSOWO, 1, 3, S("!orc_food"),
	PS_KONIEC
};

int orc_chief_items[] = {
	PS_POZIOM, 10,
		PS_DAJ, S("am_orc"),
		PS_SZANSA2, 50, S("shield_steel"), S("shield_iron"),
		PS_IF_SZANSA, 25,
			PS_JEDEN_Z_WIELU, 3, S("blunt_dwarven"), S("axe_crystal"), S("sword_serrated"),
		PS_ELSE,
			PS_JEDEN_Z_WIELU, 3, S("blunt_orcish"), S("axe_orcish"), S("sword_orcish"),
		PS_END_IF,
	PS_ELSE,
		PS_SZANSA2, 75, S("am_orc"), S("al_orc"),
		PS_DAJ, S("shield_iron"),
		PS_JEDEN_Z_WIELU, 3, S("blunt_orcish"), S("axe_orcish"), S("sword_orcish"),
	PS_END_IF,
	PS_DAJ, S("p_hp2"),
	PS_LOSOWO, 2, 4, S("!orc_food"),
	PS_KONIEC
};

int orc_shaman_items[] = {
	PS_DAJ, S("al_orc_shaman"),
	PS_DAJ, S("blunt_orcish_shaman"),
	PS_SZANSA, 15, S("p_hp"),
	PS_LOSOWO, 1, 3, S("!orc_food"),
	PS_KONIEC
};

int skeleton_items[] = {
	PS_SZANSA2, 50, S("bow_short"), S("shield_wood"),
	PS_JEDEN_Z_WIELU, 5, S("dagger_short"), S("dagger_sword"), S("sword_long"), S("axe_small"), S("blunt_club"),
	PS_KONIEC
};

int skeleton_archer_items[] = {
	PS_SZANSA2, 50, S("bow_long"), S("bow_short"),
	PS_JEDEN_Z_WIELU, 5, S("dagger_short"), S("dagger_sword"), S("sword_long"), S("axe_small"), S("blunt_club"),
	PS_SZANSA, 50, S("shield_wood"),
	PS_KONIEC
};

int skeleton_fighter_items[] = {
	PS_SZANSA2, 50, S("shield_wood"), S("shield_iron"),
	PS_SZANSA, 50, S("bow_short"),
	PS_IF_SZANSA, 50,
		PS_JEDEN_Z_WIELU, 4, S("dagger_rapier"), S("sword_scimitar"), S("axe_battle"), S("blunt_mace"),
	PS_ELSE,
		PS_JEDEN_Z_WIELU, 5, S("dagger_short"), S("dagger_sword"), S("sword_long"), S("axe_small"), S("blunt_club"),
	PS_END_IF,
	PS_KONIEC
};

int skeleton_mage_items[] = {
	PS_DAJ, S("wand_1"),
	PS_KONIEC
};

int bandit_items[] = {
	PS_IF_SZANSA, 50,
		PS_DAJ, S("!medium_armor"),
		PS_DAJ, S("!shield"),
		PS_JEDEN_Z_WIELU, 3, S("!long_blade"), S("!axe"), S("!blunt"),
		PS_SZANSA, 50, S("!-2bow"),
	PS_ELSE,
		PS_DAJ, S("!light_armor"),
		PS_DAJ, S("!-2shield"),
		PS_JEDEN_Z_WIELU, 4, S("!long_blade"), S("!axe"), S("!blunt"), S("!short_blade"),
		PS_SZANSA, 50, S("!bow"),
	PS_END_IF,
	PS_SZANSA, 10, S("p_hp"),
	PS_LOSOWO, 1, 3, S("!normal_food"),
	PS_KONIEC
};

int bandit_archer_items[] = {
	PS_DAJ, S("!light_armor"),
	PS_DAJ, S("!bow"),
	PS_JEDEN_Z_WIELU, 4, S("!-2long_blade"), S("!-2axe"), S("!-2blunt"), S("!-2short_blade"),
	PS_SZANSA, 50, S("!-2bow"),
	PS_SZANSA, 10, S("p_hp"),
	PS_LOSOWO, 1, 3, S("!normal_food"),
	PS_KONIEC
};

int bandit_hegemon_items[] = {
	PS_JEDEN_Z_WIELU, 2, S("!medium_armor"), S("!heavy_armor"),
	PS_JEDEN_Z_WIELU, 4, S("!long_blade"), S("!axe"), S("!blunt"), S("!short_blade"),
	PS_DAJ, S("!shield"),
	PS_SZANSA, 50, S("!-2bow"),
	PS_DAJ, S("p_hp2"),
	PS_LOSOWO, 2, 4, S("!normal_food"),
	PS_KONIEC
};

int mage_items[] = {
	PS_DAJ, S("!wand"),
	PS_DAJ, S("!mage_armor"),
	PS_POZIOM, 12,
		PS_SZANSA, 50, S("p_hp"),
	PS_ELSE,
		PS_POZIOM, 8,
			PS_SZANSA, 25, S("p_hp"),
		PS_ELSE,
			PS_SZANSA, 10, S("p_hp"),
		PS_END_IF,
	PS_END_IF,
	PS_LOSOWO, 1, 3, S("!normal_food"),
	PS_KONIEC
};

int guard_items[] = {
	PS_IF_SZANSA, 33,
		PS_DAJ, S("!light_armor"),
		PS_JEDEN_Z_WIELU, 4, S("!-1long_blade"), S("!-1axe"), S("!-1blunt"), S("!-1short_blade"),
		PS_DAJ, S("!bow"),
	PS_ELSE,
		PS_IF_SZANSA, 33,
			PS_DAJ, S("!medium_armor"),
			PS_JEDEN_Z_WIELU, 4, S("!long_blade"), S("!axe"), S("!blunt"), S("!short_blade"),
			PS_JEDEN_Z_WIELU, 2, S("!-2bow"), S("!-2shield"),
		PS_ELSE,
			PS_DAJ, S("!heavy_armor"),
			PS_JEDEN_Z_WIELU, 3, S("!long_blade"), S("!axe"), S("!blunt"),
			PS_DAJ, S("!shield"),
		PS_END_IF,
	PS_END_IF,
	PS_POZIOM, 9,
		PS_SZANSA, 50, S("p_hp2"),
	PS_ELSE,
		PS_SZANSA, 50, S("p_hp"),
	PS_END_IF,
	PS_LOSOWO, 1, 3, S("!normal_food"),
	PS_KONIEC
};

int q_guard_items[] = {
	PS_DAJ_KILKA, 3, S("p_hp2"),
	PS_IF_SZANSA, 33,
		PS_DAJ, S("!light_armor"),
		PS_JEDEN_Z_WIELU, 4, S("!-1long_blade"), S("!-1axe"), S("!-1blunt"), S("!-1short_blade"),
		PS_DAJ, S("!bow"),
	PS_ELSE,
		PS_IF_SZANSA, 33,
			PS_DAJ, S("!medium_armor"),
			PS_JEDEN_Z_WIELU, 4, S("!long_blade"), S("!axe"), S("!blunt"), S("!short_blade"),
			PS_JEDEN_Z_WIELU, 2, S("!-2bow"), S("!-2shield"),
		PS_ELSE,
			PS_DAJ, S("!heavy_armor"),
			PS_JEDEN_Z_WIELU, 3, S("!long_blade"), S("!axe"), S("!blunt"),
			PS_DAJ, S("!shield"),
		PS_END_IF,
	PS_END_IF,
	PS_POZIOM, 9,
		PS_SZANSA, 50, S("p_hp2"),
	PS_ELSE,
		PS_SZANSA, 50, S("p_hp"),
	PS_END_IF,
	PS_LOSOWO, 1, 3, S("!normal_food"),
	PS_KONIEC
};

int necromancer_items[] = {
	PS_DAJ, S("al_necromancer"),
	PS_DAJ, S("!wand"),
	PS_LOSOWO, 1, 3, S("f_raw_meat"),
	PS_KONIEC
};

int evil_cleric_items[] = {
	PS_DAJ, S("!blunt"),
	PS_DAJ, S("!shield"),
	PS_POZIOM, 10,
		PS_DAJ, S("!heavy_armor"),
	PS_ELSE,
		PS_DAJ, S("!medium_armor"),
	PS_END_IF,
	PS_DAJ, S("p_hp2"),
	PS_LOSOWO, 1, 3, S("f_raw_meat"),
	PS_KONIEC
};

int warrior_items[] = {
	PS_IF_SZANSA, 50,
		PS_DAJ, S("!heavy_armor"),
		PS_JEDEN_Z_WIELU, 3, S("!long_blade"), S("!axe"), S("!blunt"),
		PS_DAJ, S("!shield"),
		PS_DAJ, S("!-4bow"),
	PS_ELSE,
		PS_DAJ, S("!medium_armor"),
		PS_JEDEN_Z_WIELU, 4, S("!long_blade"), S("!axe"), S("!blunt"), S("!short_blade"),
		PS_DAJ, S("!-2shield"),
		PS_DAJ, S("!-2bow"),
	PS_END_IF,
	PS_POZIOM, 15,
		PS_DAJ, S("p_hp3"),
		PS_DAJ_KILKA, 3, S("p_hp2"),
	PS_ELSE,
		PS_POZIOM, 12,
			PS_DAJ_KILKA, 2, S("p_hp2"),
		PS_ELSE,
			PS_POZIOM, 9,
				PS_DAJ, S("p_hp2"),
				PS_DAJ_KILKA, 2, S("p_hp"),				
			PS_ELSE,
				PS_DAJ, S("p_hp"),
				PS_POZIOM, 6,
					PS_DAJ, S("p_hp"),					
				PS_END_IF,
			PS_END_IF,
		PS_END_IF,
	PS_END_IF,
	PS_LOSOWO, 1, 3, S("!normal_food"),
	PS_KONIEC
};

int hunter_items[] = {
	PS_JEDEN_Z_WIELU, 2, S("!-1light_armor"), S("!-1medium_armor"),
	PS_DAJ, S("!bow"),
	PS_DAJ, S("!-2shield"),
	PS_JEDEN_Z_WIELU, 4, S("!-2long_blade"), S("!-2axe"), S("!-2blunt"), S("!-2short_blade"),
	PS_POZIOM, 15,
		PS_DAJ, S("p_hp3"),
		PS_DAJ, S("p_hp2"),
	PS_ELSE,
		PS_POZIOM, 12,
			PS_DAJ_KILKA, 2, S("p_hp2"),
		PS_ELSE,
			PS_POZIOM, 9,
				PS_DAJ, S("p_hp2"),
				PS_DAJ, S("p_hp"),
			PS_ELSE,
				PS_DAJ, S("p_hp"),
				PS_POZIOM, 6,
					PS_DAJ, S("p_hp"),
				PS_END_IF,
			PS_END_IF,
		PS_END_IF,
	PS_END_IF,
	PS_LOSOWO, 1, 3, S("!normal_food"),
	PS_KONIEC
};

int rogue_items[] = {
	PS_JEDEN_Z_WIELU, 2, S("!short_blade"), S("!long_blade"),
	PS_DAJ, S("!light_armor"),
	PS_JEDEN_Z_WIELU, 2, S("!-2bow"), S("!-2shield"),
	PS_POZIOM, 15,
		PS_DAJ, S("p_hp3"),
		PS_DAJ, S("p_hp2"),
	PS_ELSE,
		PS_POZIOM, 12,
			PS_DAJ_KILKA, 2, S("p_hp2"),
		PS_ELSE,
			PS_POZIOM, 9,
				PS_DAJ, S("p_hp2"),
				PS_DAJ, S("p_hp"),
			PS_ELSE,
				PS_DAJ, S("p_hp"),
				PS_POZIOM, 6,
					PS_DAJ, S("p_hp"),
				PS_END_IF,
			PS_END_IF,
		PS_END_IF,
	PS_END_IF,
	PS_LOSOWO, 1, 3, S("!normal_food"),
	PS_KONIEC
};

int szaleniec_items[] = {
	PS_DAJ, S("p_hp2"),
	PS_JEDEN_Z_WIELU, 4, S("!-2long_blade"), S("!-2axe"), S("!-2blunt"), S("!-2short_blade"),
	PS_DAJ, S("!bow"),
	PS_DAJ, S("!light_armor"),
	PS_DAJ, S("!-2shield"),
	PS_DAJ, S("q_szaleni_kamien"),
	PS_LOSOWO, 1, 3, S("!normal_food"),
	PS_KONIEC
};

int xar_items[] = {
	PS_DAJ, S("blunt_skullsmasher"),
	PS_DAJ, S("ah_black_armor"),
	PS_DAJ, S("shield_unique"),
	PS_DAJ_KILKA, 3, S("p_hp3"),
	PS_DAJ, S("p_str"),
	PS_DAJ, S("p_end"),
	PS_LOSOWO, 2, 4, S("f_raw_meat"),
	PS_KONIEC
};

int tomash_items[] = {
	PS_DAJ, S("sword_forbidden"),
	PS_DAJ, S("ah_black_armor"),
	PS_DAJ, S("shield_adamantine"),
	PS_DAJ, S("bow_dragonbone"),
	PS_DAJ_KILKA, 3, S("p_hp3"),
	PS_DAJ_KILKA, 5, S("vs_krystal"),
	PS_KONIEC
};

int citzen_items[] = {
	PS_IF_SZANSA, 40,
		PS_JEDEN_Z_WIELU, 2, S("al_clothes2"), S("al_clothes2b"),
	PS_ELSE,
		PS_DAJ, S("al_clothes"),
	PS_END_IF,
	PS_JEDEN_Z_WIELU, 3, S("dagger_short"), S("axe_small"), S("blunt_club"),
	PS_LOSOWO, 1, 3, S("!normal_food"),
	PS_KONIEC
};

int poslaniec_items[] = {
	PS_DAJ, S("al_clothes"),
	PS_JEDEN_Z_WIELU, 3, S("dagger_short"), S("axe_small"), S("blunt_club"),
	PS_LOSOWO, 1, 3, S("!normal_food"),
	PS_KONIEC
};

int captive_items[] = {
	PS_IF_SZANSA, 40,
		PS_JEDEN_Z_WIELU, 2, S("al_clothes3"), S("al_clothes3b"),
	PS_ELSE,
		PS_JEDEN_Z_WIELU, 2, S("al_clothes2"), S("al_clothes2b"),
	PS_END_IF,
	PS_JEDEN_Z_WIELU, 2, S("dagger_short"), S("dagger_rapier"),
	PS_LOSOWO, 0, 2, S("!normal_food"),
	PS_KONIEC
};

int burmistrz_items[] = {
	PS_JEDEN_Z_WIELU, 2, S("al_clothes3"), S("al_clothes3b"),
	PS_JEDEN_Z_WIELU, 4, S("dagger_sword"), S("dagger_rapier"), S("sword_long"), S("sword_scimitar"),
	PS_LOSOWO, 1, 3, S("!normal_food"),
	PS_KONIEC
};

int karczmarz_items[] = {
	PS_DAJ, S("al_innkeeper"),
	PS_JEDEN_Z_WIELU, 3, S("blunt_mace"), S("axe_small"), S("sword_long"),
	PS_DAJ_KILKA, 5, S("p_beer"),
	PS_DAJ_KILKA, 5, S("p_water"),
	PS_DAJ_KILKA, 3, S("p_vodka"),
	PS_LOSOWO, 1, 3, S("!normal_food"),
	PS_KONIEC
};

int podrozny_items[] = {
	PS_JEDEN_Z_WIELU, 2, S("al_clothes"), S("al_leather"),
	PS_JEDEN_Z_WIELU, 3, S("blunt_mace"), S("axe_small"), S("sword_long"),
	PS_SZANSA, 50, S("shield_wood"),
	PS_SZANSA, 50, S("bow_short"),
	PS_LOSOWO, 1, 3, S("!normal_food"),
	PS_KONIEC
};

int artur_drwal_items[] = {
	PS_DAJ, S("al_studded"),
	PS_DAJ, S("axe_battle"),
	PS_LOSOWO, 1, 3, S("!normal_food"),
	PS_KONIEC
};

int drwal_items[] = {
	PS_JEDEN_Z_WIELU, 2, S("al_leather"), S("al_clothes"),
	PS_JEDEN_Z_WIELU, 2, S("axe_small"), S("axe_battle"),
	PS_LOSOWO, 1, 3, S("!normal_food"),
	PS_KONIEC
};

int gornik_items[] = {
	PS_DAJ, S("pickaxe"),
	PS_JEDEN_Z_WIELU, 2, S("al_leather"), S("al_clothes"),
	PS_LOSOWO, 1, 3, S("!normal_food"),
	PS_KONIEC
};

int inwestor_items[] = {
	PS_JEDEN_Z_WIELU, 2, S("al_clothes3"), S("al_clothes3b"),
	PS_DAJ, S("dagger_rapier"),
	PS_LOSOWO, 1, 3, S("!normal_food"),
	PS_KONIEC
};

int q_bandyci_szef_items[] = {
	PS_SZANSA2, 25, S("bow_elven"), S("bow_composite"),
	PS_DAJ, S("al_dragonskin"),
	PS_DAJ, S("dagger_spinesheath"),
	PS_SZANSA2, 25, S("shield_mithril"), S("shield_steel"),
	PS_DAJ, S("p_hp2"),
	PS_DAJ, S("p_dex"),
	PS_LOSOWO, 2, 4, S("!normal_food"),
	PS_KONIEC
};

int q_magowie_boss_items[] = {
	PS_DAJ, S("sword_unique"),
	PS_DAJ, S("al_mage4"),
	PS_DAJ, S("p_hp2"),
	PS_DAJ, S("q_magowie_kula2"),
	PS_DAJ, S("p_end"),
	PS_LOSOWO, 2, 4, S("!normal_food"),
	PS_KONIEC
};

int gorush_items[] = {
	PS_DAJ, S("p_hp"),
	PS_DAJ, S("al_orc"),
	PS_DAJ, S("shield_wood"),
	PS_DAJ, S("axe_orcish"),
	PS_DAJ, S("bow_short"),
	PS_LOSOWO, 0, 2, S("!orc_food"),
	PS_KONIEC
};

int gorush_woj_items[] = {
	PS_DAJ, S("p_hp2"),
	PS_DAJ, S("am_orc"),
	PS_DAJ, S("shield_steel"),
	PS_DAJ, S("axe_crystal"),
	PS_DAJ, S("bow_long"),
	PS_LOSOWO, 3, 5, S("!orc_food"),
	PS_KONIEC
};

int gorush_lowca_items[] = {
	PS_DAJ, S("p_hp2"),
	PS_DAJ, S("bow_composite"),
	PS_DAJ, S("shield_iron"),
	PS_DAJ, S("axe_orcish"),
	PS_DAJ, S("al_orc"),
	PS_LOSOWO, 3, 5, S("!orc_food"),
	PS_KONIEC
};

int gorush_szaman_items[] = {
	PS_DAJ, S("al_orc_shaman"),
	PS_DAJ, S("blunt_orcish_shaman"),
	PS_DAJ, S("p_hp"),
	PS_DAJ, S("shield_iron"),
	PS_LOSOWO, 3, 5, S("!orc_food"),
	PS_KONIEC
};

int orkowie_boss_items[] = {
	PS_DAJ, S("al_orc_shaman"),
	PS_DAJ, S("axe_ripper"),
	PS_DAJ_KILKA, 3, S("p_hp3"),
	PS_DAJ, S("shield_adamantine"),
	PS_DAJ, S("bow_composite"),
	PS_DAJ, S("p_str"),
	PS_LOSOWO, 3, 5, S("!orc_food"),
	PS_KONIEC
};

int orkowie_slaby_items[] = {
	PS_JEDEN_Z_WIELU, 3, S("blunt_club"), S("axe_small"), S("dagger_short"),
	PS_LOSOWO, 0, 2, S("!orc_food"),
	PS_KONIEC
};

int szlachcic_items[] = {
	PS_DAJ, S("dagger_assassin"),
	PS_DAJ, S("bow_unique"),
	PS_DAJ, S("al_chain_shirt_mith"),
	PS_DAJ, S("p_hp3"),
	PS_DAJ_KILKA, 2, S("p_hp2"),
	PS_DAJ, S("p_dex"),
	PS_LOSOWO, 1, 3, S("!normal_food"),
	PS_KONIEC
};

int kaplan_items[] = {
	PS_DAJ, S("blunt_morningstar"),
	PS_DAJ, S("shield_iron"),
	PS_DAJ, S("am_breastplate"),
	PS_DAJ, S("p_hp2"),
	PS_LOSOWO, 1, 3, S("!normal_food"),
	PS_KONIEC
};

int wolf_items[] = {
	PS_LOSOWO, 2, 3, S("f_raw_meat"),
	PS_KONIEC
};

//=================================================================================================
// ZAKL CIA POSTACI
//=================================================================================================
SpellList orc_shaman_spells(0, "magic_bolt", 9, "fireball", 0, NULL, false);
SpellList zombie_rotting_spells(0, "spit_poison", 0, NULL, 0, NULL, false);
SpellList skeleton_mage_spells(0, "magic_bolt", 8, "fireball", 10, "raise", true);
SpellList mage_spells(0, "magic_bolt", 8, "fireball", 12, "thunder_bolt", false);
SpellList necromancer_spells(0, "magic_bolt", 5, "raise", 10, "exploding_skull", true);
SpellList evil_cleric_spells(0, NULL, 5, "drain", 10, "raise", true);
SpellList xar_spells(0, "exploding_skull", 0, "drain2", 0, NULL, false);
SpellList mage_boss_spells(0, "xmagic_bolt", 0, "fireball", 0, "thunder_bolt", false);
SpellList tomash_spells(0, "mistic_ball", 0, NULL, 0, NULL, false);
SpellList jozan_spells(0, "heal", 0, NULL, 0, NULL, true);

//=================================================================================================
// DèWI KI POSTACI
//=================================================================================================
SoundPak sounds_def("hey.mp3", "aahhzz.mp3", "ahhh.mp3", NULL);
SoundPak sounds_wolf("wolf_x.mp3", "dog_whine.mp3", "dog_whine.mp3", "wolf_attack.mp3");
SoundPak sounds_spider("spider_tarantula_hiss.mp3", NULL, NULL, "spider_bite_and_suck.mp3");
SoundPak sounds_rat("rat.wav", NULL, "ratdeath.mp3", NULL);
SoundPak sounds_zombie("zombie groan 1.wav", "zombie.mp3", "zombie2.mp3", "zombie attack.wav");
SoundPak sounds_skeleton("skeleton_alert.mp3", "bone_break.mp3", "falling-bones.mp3", NULL);
SoundPak sounds_golem("golem_alert.mp3", NULL, "earthquake.wav", NULL);
SoundPak sounds_golem_iron("golem_alert.mp3", NULL, "irongolem_break.mp3", NULL);
SoundPak sounds_goblin("goblin-3.wav", "goblin-11.wav", "goblin-15.wav", NULL);
SoundPak sounds_orc("ogre2.wav", "ogre5.wav", "ogre3.wav", NULL);
SoundPak sounds_orc_boss("snarl.mp3", "ogre5.wav", "ogre3.wav", NULL);
SoundPak sounds_boss("shade8.wav", "shade5.wav", "scream.wav", NULL);
SoundPak sounds_undead("shade8.wav", "shade5.wav", "shade12.wav", NULL);
SoundPak sounds_nieznany(NULL, "mnstr15.wav", "mnstr7.wav", "mnstr2.wav");

//=================================================================================================
// ATAKI GRACZA
//=================================================================================================
AttackFrameInfo human_attacks = {
	10.f/30, 21.f/30, A_LONG_BLADE|A_AXE|A_BLUNT|A_SHORT_BLADE,
	9.f/30, 22.f/30, A_LONG_BLADE|A_AXE|A_BLUNT,
	7.f/30, 21.f/30, A_LONG_BLADE|A_SHORT_BLADE|A_BLUNT,
	11.f/30, 21.f/30, A_LONG_BLADE|A_AXE|A_BLUNT,
	9.f/25, 18.f/25, A_SHORT_BLADE|A_LONG_BLADE
};

//=================================================================================================
// ATAKI POSTACI
//=================================================================================================
FrameInfo fi_human = {&human_attacks, 30.f/35, 10.f/25, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 20.f/25, 5};
FrameInfo fi_skeleton = {NULL, 1.f, 10.f/20, 6.f/15, 1.f, 11.f/23, 1.f, 8.f/17, 1.f, 20.f/30, 3};
FrameInfo fi_zombie = {NULL, 20.f/30.f, 0.f, 15.f/20, 1.f, 9.f/15, 1.f, 0.f, 0.f, 0.f, 2};
FrameInfo fi_goblin = {NULL, 0.f, 10.f/20, 5.f/15, 10.f/15, 5.f/15, 10.f/15, 0.f, 0.f, 10.f/15, 2};
FrameInfo fi_ork = {NULL, 1.f, 12.f/24, 7.f/20, 13.f/20, 7.f/20, 13.f/20, 0.f, 0.f, 15.f/21, 2};
FrameInfo fi_wilk = {NULL, 0.f, 0.f, 12.f/17, 1.f, 13.f/20, 15.f/20, 0.f, 0.f, 0.f, 2};
FrameInfo fi_szczur = {NULL, 0.f, 0.f, 7.f/15, 10.f/15, 0.f, 0.f, 0.f, 0.f, 0.f, 1};
FrameInfo fi_golem = {NULL, 0.f, 0.f, 20.f/40, 30.f/40, 20.f/40, 30.f/40, 0.f, 0.f, 0.f, 2};
FrameInfo fi_spider = {NULL, 0.f, 0.f, 5.f/20, 10.f/20, 0.f, 0.f, 0.f, 0.f, 0.f, 1};
FrameInfo fi_nieznany = {NULL, 0.f, 0.f, 7.f/13, 1.f, 6.f/11, 1.f, 0.f, 0.f, 0.f, 2};

//=================================================================================================
// TEKSTURY POSTACI
//=================================================================================================
TexId ti_zombie_rotting[] = {TexId(NULL), TexId("glowa_zombie2.jpg"), TexId(NULL)};
TexId ti_zombie_ancient[] = {TexId("zombif.jpg"), TexId("glowa4.jpg"), TexId(NULL)};
TexId ti_worg[] = {TexId("wilk_czarny.jpg"), TexId(NULL), TexId(NULL)};
TexId ti_golem_iron[] = {TexId("golem2.jpg"), TexId(NULL), TexId(NULL)};
TexId ti_golem_adamantite[] = {TexId("golem3.jpg"), TexId(NULL), TexId(NULL)};
TexId ti_crazy_hunter[] = {TexId(NULL), TexId("crazy_hunter.jpg"), TexId(NULL)};
TexId ti_necromant[] = {TexId(NULL), TexId("necromant.jpg"), TexId(NULL)};
TexId ti_undead[] = {TexId(NULL), TexId("undead.jpg"), TexId(NULL)};
TexId ti_evil[] = {TexId(NULL), TexId("evil.jpg"), TexId(NULL)};
TexId ti_evil_boss[] = {TexId(NULL), TexId("evil_boss.jpg"), TexId(NULL)};
TexId ti_orc_shaman[] = {TexId("ork_szaman.jpg"), TexId(NULL), TexId(NULL)};

//=================================================================================================
// ANIMACJE IDLE
//=================================================================================================
cstring idle_zombie[] = {"wygina"};
cstring idle_szkielet[] = {"rozglada", "przysiad", "wygina"};
cstring idle_czlowiek[] = {"wygina", "rozglada", "drapie"};
cstring idle_goblin[] = {"rozglada"};
cstring idle_wilk[] = {"rozglada", "wygina"};
cstring idle_szczur[] = {"idle", "idle2"};
cstring idle_golem[] = {"idle"};
cstring idle_spider[] = {"idle"};
cstring idle_nieznany[] = {"idle"};

//=================================================================================================
// STATYSTYKI POSTACI
// hp_bonus to teraz bazowe hp (500 cz≥owiek, 600 ork, 400 goblin itp)
//=================================================================================================
UnitData g_base_units[] = {
	// id imiÍ model materia≥ poziom
	// attribute profile, skill profile
	// hp+ def+ przedmioty czary z≥oto[2] dialog grupa
	// flagi zasÛb typ.obraøeÒ szy.chodzenia szy.biegania szy.obrotu krew
	// düwiÍki info.animacje tekstury animacje.idle ile.animacji promieÒ zasiÍg.ataku flagi2 flagi3
	//---- STARTOWE KLASY POSTACI ----
	"base_warrior", NULL, NULL, MAT_BODY, INT2(0),
		StatProfileType::WARRIOR,
		500, 0, base_warrior_items, NULL, INT2(85), INT2(85), NULL, G_GRACZ,
		F_CZLOWIEK, NULL, 0, 1.5f, 5.f, 3.f, BLOOD_RED,
		&sounds_def, &fi_human, NULL, idle_czlowiek, countof(idle_czlowiek), 0.3f, 1.f, 0, 0, ArmorUnitType::HUMAN,
	"base_hunter", NULL, NULL, MAT_BODY, INT2(0),
		StatProfileType::HUNTER,
		500, 0, base_hunter_items, NULL, INT2(60), INT2(60), NULL, G_GRACZ,
		F_CZLOWIEK, NULL, 0, 1.5f, 5.f, 3.f, BLOOD_RED,
		&sounds_def, &fi_human, NULL, idle_czlowiek, countof(idle_czlowiek), 0.3f, 1.f, 0, 0, ArmorUnitType::HUMAN,
	"base_rogue", NULL, NULL, MAT_BODY, INT2(0),
		StatProfileType::ROGUE,
		500, 0, base_rogue_items, NULL, INT2(100), INT2(100), NULL, G_GRACZ,
		F_CZLOWIEK, NULL, 0, 1.5f, 5.f, 3.f, BLOOD_RED,
		&sounds_def, &fi_human, NULL, idle_czlowiek, countof(idle_czlowiek), 0.3f, 1.f, F2_CIOS_W_PLECY, 0, ArmorUnitType::HUMAN,

	//---- NPC ----
	"blacksmith", NULL, NULL, MAT_BODY, INT2(5),
		StatProfileType::BLACKSMITH,
		500, 0, blacksmith_items, NULL, INT2(500,1000), INT2(500,1000), dialog_kowal, G_MIESZKANCY,
		F_CZLOWIEK|F_AI_STOI, NULL, 0, 1.5f, 5.f, 3.f, BLOOD_RED,
		&sounds_def, &fi_human, NULL, idle_czlowiek, countof(idle_czlowiek), 0.3f, 1.f, 0, F3_NIE_JE, ArmorUnitType::HUMAN,
	"merchant", NULL, NULL, MAT_BODY, INT2(4),
		StatProfileType::MERCHANT,
		500, 0, merchant_items, NULL, INT2(500,1000), INT2(500,1000), dialog_kupiec, G_MIESZKANCY,
		F_CZLOWIEK|F_AI_STRAZNIK|F_AI_STOI, NULL, 0, 1.5f, 5.f, 3.f, BLOOD_RED,
		&sounds_def, &fi_human, NULL, idle_czlowiek, countof(idle_czlowiek), 0.3f, 1.f, 0, F3_NIE_JE, ArmorUnitType::HUMAN,
	"food_seller", NULL, NULL, MAT_BODY, INT2(4),
		StatProfileType::MERCHANT,
		500, 0, merchant_items, NULL, INT2(500,1000), INT2(500,1000), dialog_sprzedawca_jedzenia, G_MIESZKANCY,
		F_CZLOWIEK|F_AI_STRAZNIK|F_AI_STOI, NULL, 0, 1.5f, 5.f, 3.f, BLOOD_RED,
		&sounds_def, &fi_human, NULL, idle_czlowiek, countof(idle_czlowiek), 0.3f, 1.f, 0, F3_NIE_JE, ArmorUnitType::HUMAN,
	"alchemist", NULL, NULL, MAT_BODY, INT2(4),
		StatProfileType::ALCHEMIST,
		500, 0, alchemist_items, NULL, INT2(500,1000), INT2(500,1000), dialog_alchemik, G_MIESZKANCY,
		F_CZLOWIEK|F_AI_STOI, NULL, 0, 1.5f, 5.f, 3.f, BLOOD_RED,
		&sounds_def, &fi_human, NULL, idle_czlowiek, countof(idle_czlowiek), 0.3f, 1.f, 0, F3_NIE_JE, ArmorUnitType::HUMAN,
	"citzen", NULL, NULL, MAT_BODY, INT2(1,5),
		StatProfileType::COMMONER,
		500, 0, citzen_items, NULL, INT2(10,50), INT2(20,60), dialog_mieszkaniec, G_MIESZKANCY,
		F_CZLOWIEK|F_AI_CHODZI|F_AI_PIJAK, NULL, 0, 1.5f, 5.f, 3.f, BLOOD_RED,
		&sounds_def, &fi_human, NULL, idle_czlowiek, countof(idle_czlowiek), 0.3f, 1.f, 0, F3_ZAWODY_W_PICIU_25, ArmorUnitType::HUMAN,
	"tut_czlowiek", NULL, NULL, MAT_BODY, INT2(1,5),
		StatProfileType::COMMONER,
		500, 0, citzen_items, NULL, INT2(10,50), INT2(20,60), dialog_tut_czlowiek, G_MIESZKANCY,
		F_CZLOWIEK|F_AI_STOI, NULL, 0, 1.5f, 5.f, 3.f, BLOOD_RED,
		&sounds_def, &fi_human, NULL, idle_czlowiek, countof(idle_czlowiek), 0.3f, 1.f, 0, 0, ArmorUnitType::HUMAN,
	"villager", NULL, NULL, MAT_BODY, INT2(1,5),
		StatProfileType::COMMONER,
		500, 0, citzen_items, NULL, INT2(10,50), INT2(20,60), dialog_mieszkaniec, G_MIESZKANCY,
		F_CZLOWIEK|F_AI_CHODZI|F_AI_PIJAK, NULL, 0, 1.5f, 5.f, 3.f, BLOOD_RED,
		&sounds_def, &fi_human, NULL, idle_czlowiek, countof(idle_czlowiek), 0.3f, 1.f, 0, F3_ZAWODY_W_PICIU_25, ArmorUnitType::HUMAN,
	"mayor", NULL, NULL, MAT_BODY, INT2(1,5),
		StatProfileType::CLERK,
		500, 0, burmistrz_items, NULL, INT2(100,500), INT2(200,600), dialog_burmistrz, G_MIESZKANCY,
		F_CZLOWIEK|F_AI_URZEDNIK|F_AI_STOI, NULL, 0, 1.5f, 5.f, 3.f, BLOOD_RED,
		&sounds_def, &fi_human, NULL, idle_czlowiek, countof(idle_czlowiek), 0.3f, 1.f, 0, F3_NIE_JE, ArmorUnitType::HUMAN,
	"soltys", NULL, NULL, MAT_BODY, INT2(1,5),
		StatProfileType::CLERK,
		500, 0, burmistrz_items, NULL, INT2(100,500), INT2(200,600), dialog_burmistrz, G_MIESZKANCY,
		F_CZLOWIEK|F_AI_URZEDNIK|F_AI_STOI, NULL, 0, 1.5f, 5.f, 3.f, BLOOD_RED,
		&sounds_def, &fi_human, NULL, idle_czlowiek, countof(idle_czlowiek), 0.3f, 1.f, 0, F3_NIE_JE, ArmorUnitType::HUMAN,
	"trainer", NULL, NULL, MAT_BODY, INT2(10,15),
		StatProfileType::FIGHTER,
		500, 0, guard_items, NULL, INT2(50,200), INT2(100,300), dialog_trener, G_MIESZKANCY,
		F_CZLOWIEK|F_AI_STRAZNIK|F_AI_STOI, NULL, 0, 1.5f, 5.f, 3.f, BLOOD_RED,
		&sounds_def, &fi_human, NULL, idle_czlowiek, countof(idle_czlowiek), 0.3f, 1.f, 0, F3_NIE_JE, ArmorUnitType::HUMAN,
	"guard", NULL, NULL, MAT_BODY, INT2(2,6),
		StatProfileType::FIGHTER,
		500, 0, guard_items, NULL, INT2(5,25), INT2(20,100), dialog_straznik, G_MIESZKANCY,
		F_CZLOWIEK|F_AI_STRAZNIK|F_AI_STOI, NULL, 0, 1.5f, 5.f, 3.f, BLOOD_RED,
		&sounds_def, &fi_human, NULL, idle_czlowiek, countof(idle_czlowiek), 0.3f, 1.f, F2_OGRANICZONY_OBROT, F3_NIE_JE, ArmorUnitType::HUMAN,
	"guard2", NULL, NULL, MAT_BODY, INT2(2,6),
		StatProfileType::FIGHTER,
		500, 0, guard_items, NULL, INT2(5,25), INT2(20,100), dialog_straznicy_nagroda, G_MIESZKANCY,
		F_CZLOWIEK, NULL, 0, 1.5f, 5.f, 3.f, BLOOD_RED,
		&sounds_def, &fi_human, NULL, idle_czlowiek, countof(idle_czlowiek), 0.3f, 1.f, 0, F3_NIE_JE, ArmorUnitType::HUMAN,
	"guard3", NULL, NULL, MAT_BODY, INT2(2,6),
		StatProfileType::FIGHTER,
		500, 0, guard_items, NULL, INT2(5,25), INT2(20,100), dialog_straznik, G_MIESZKANCY,
		F_CZLOWIEK, NULL, 0, 1.5f, 5.f, 3.f, BLOOD_RED,
		&sounds_def, &fi_human, NULL, idle_czlowiek, countof(idle_czlowiek), 0.3f, 1.f, 0, F3_NIE_JE, ArmorUnitType::HUMAN,
	"guard_move", NULL, NULL, MAT_BODY, INT2(2,6),
		StatProfileType::FIGHTER,
		500, 0, guard_items, NULL, INT2(5,25), INT2(20,100), dialog_straznik, G_MIESZKANCY,
		F_CZLOWIEK|F_AI_CHODZI, NULL, 0, 1.5f, 5.f, 3.f, BLOOD_RED,
		&sounds_def, &fi_human, NULL, idle_czlowiek, countof(idle_czlowiek), 0.3f, 1.f, F2_AI_TRENUJE, F3_NIE_JE, ArmorUnitType::HUMAN,
	"guard_captain", NULL, NULL, MAT_BODY, INT2(6,12),
		StatProfileType::FIGHTER,
		500, 0, guard_items, NULL, INT2(10,50), INT2(20,60), dialog_dowodca_strazy, G_MIESZKANCY,
		F_CZLOWIEK|F_AI_STRAZNIK|F_AI_STOI, NULL, 0, 1.5f, 5.f, 3.f, BLOOD_RED,
		&sounds_def, &fi_human, NULL, idle_czlowiek, countof(idle_czlowiek), 0.3f, 1.f, 0, F3_NIE_JE, ArmorUnitType::HUMAN,
	"clerk", NULL, NULL, MAT_BODY, INT2(1,5),
		StatProfileType::CLERK,
		500, 0, citzen_items, NULL, INT2(100,500), INT2(200,600), dialog_urzednik, G_MIESZKANCY,
		F_CZLOWIEK|F_AI_URZEDNIK|F_AI_STOI, NULL, 0, 1.5f, 5.f, 3.f, BLOOD_RED,
		&sounds_def, &fi_human, NULL, idle_czlowiek, countof(idle_czlowiek), 0.3f, 1.f, 0, F3_NIE_JE, ArmorUnitType::HUMAN,
	"innkeeper", NULL, NULL, MAT_BODY, INT2(4),
		StatProfileType::MERCHANT,
		500, 0, karczmarz_items, NULL, INT2(500,1000), INT2(500,1000), dialog_karczmarz, G_MIESZKANCY,
		F_CZLOWIEK|F_AI_STRAZNIK, NULL, 0, 1.5f, 5.f, 3.f, BLOOD_RED,
		&sounds_def, &fi_human, NULL, idle_czlowiek, countof(idle_czlowiek), 0.3f, 1.f, F2_OGRANICZONY_OBROT, F3_NIE_JE | F3_GADA_NA_ZAWODACH, ArmorUnitType::HUMAN,
	"arena_master", NULL,  NULL, MAT_BODY, INT2(10,15),
		StatProfileType::WARRIOR,
		500, 0, guard_items, NULL, INT2(50,200), INT2(100,300), dialog_mistrz_areny, G_MIESZKANCY,
		F_CZLOWIEK|F_AI_STRAZNIK|F_AI_STOI, NULL, 0, 1.5f, 5.f, 3.f, BLOOD_RED,
		&sounds_def, &fi_human, NULL, idle_czlowiek, countof(idle_czlowiek), 0.3f, 1.f, F2_OGRANICZONY_OBROT, F3_NIE_JE | F3_GADA_NA_ZAWODACH, ArmorUnitType::HUMAN,
	"viewer", NULL, NULL, MAT_BODY, INT2(1,5),
		StatProfileType::COMMONER,
		500, 0, citzen_items, NULL, INT2(10,50), INT2(20,60), dialog_widz, G_MIESZKANCY,
		F_CZLOWIEK|F_AI_STRAZNIK|F_AI_STOI, NULL, 0, 1.5f, 5.f, 3.f, BLOOD_RED,
		&sounds_def, &fi_human, NULL, idle_czlowiek, countof(idle_czlowiek), 0.3f, 1.f, F2_OGRANICZONY_OBROT, F3_NIE_JE, ArmorUnitType::HUMAN,
	"captive", NULL, NULL, MAT_BODY, INT2(1,5),
		StatProfileType::COMMONER,
		500, 0, captive_items, NULL, INT2(10,50), INT2(20,60), dialog_porwany, G_MIESZKANCY,
		F_CZLOWIEK|F_AI_CHODZI|F_BOHATER, NULL, 0, 1.5f, 5.f, 3.f, BLOOD_RED,
		&sounds_def, &fi_human, NULL, idle_czlowiek, countof(idle_czlowiek), 0.3f, 1.f, F2_OKRESLONE_IMIE | F2_BEZ_KLASY, 0, ArmorUnitType::HUMAN,
	"traveler", NULL, NULL, MAT_BODY, INT2(2,6),
		StatProfileType::COMMONER,
		500, 0, podrozny_items, NULL, INT2(50,100), INT2(100,200), dialog_zadanie, G_MIESZKANCY,
		F_CZLOWIEK|F_AI_CHODZI, NULL, 0, 1.5f, 5.f, 3.f, BLOOD_RED,
		&sounds_def, &fi_human, NULL, idle_czlowiek, countof(idle_czlowiek), 0.3f, 1.f, 0, F3_ZAWODY_W_PICIU_25, ArmorUnitType::HUMAN,
	"artur_drwal", NULL, NULL, MAT_BODY, INT2(5),
		StatProfileType::WORKER,
		500, 0, artur_drwal_items, NULL, INT2(500,1000), INT2(500,1000), dialog_artur_drwal, G_MIESZKANCY,
		F_CZLOWIEK|F_AI_STOI|F_BOHATER, NULL, 0, 1.5f, 5.f, 3.f, BLOOD_RED,
		&sounds_def, &fi_human, NULL, idle_czlowiek, countof(idle_czlowiek), 0.3f, 1.f, F2_OKRESLONE_IMIE | F2_BEZ_KLASY | F2_ZAWODY_W_PICIU_50, 0, ArmorUnitType::HUMAN,
	"poslaniec_tartak", NULL, NULL, MAT_BODY, INT2(1,5),
		StatProfileType::WORKER,
		500, 0, poslaniec_items, NULL, INT2(10,50), INT2(20,60), dialog_artur_drwal, G_MIESZKANCY,
		F_CZLOWIEK|F_AI_CHODZI, NULL, 0, 1.5f, 5.f, 3.f, BLOOD_RED,
		&sounds_def, &fi_human, NULL, idle_czlowiek, countof(idle_czlowiek), 0.3f, 1.f, 0, 0, ArmorUnitType::HUMAN,
	"drwal", NULL, NULL, MAT_BODY, INT2(1,5),
		StatProfileType::WORKER,
		500, 0, drwal_items, NULL, INT2(20,50), INT2(30,60), dialog_drwal, G_MIESZKANCY,
		F_CZLOWIEK, NULL, 0, 1.5f, 5.f, 3.f, BLOOD_RED,
		&sounds_def, &fi_human, NULL, idle_czlowiek, countof(idle_czlowiek), 0.3f, 1.f, 0, 0, ArmorUnitType::HUMAN,
	"inwestor", NULL, NULL, MAT_BODY, INT2(10),
		StatProfileType::CLERK,
		500, 0, inwestor_items, NULL, INT2(1000,1200), INT2(1000,1200), dialog_inwestor, G_MIESZKANCY,
		F_CZLOWIEK|F_AI_STOI|F_BOHATER, NULL, 0, 1.5f, 5.f, 3.f, BLOOD_RED,
		&sounds_def, &fi_human, NULL, idle_czlowiek, countof(idle_czlowiek), 0.3f, 1.f, F2_OKRESLONE_IMIE | F2_BEZ_KLASY, 0, ArmorUnitType::HUMAN,
	"poslaniec_kopalnia", NULL, NULL, MAT_BODY, INT2(1,5),
		StatProfileType::COMMONER,
		500, 0, poslaniec_items, NULL, INT2(10,50), INT2(20,60), dialog_inwestor, G_MIESZKANCY,
		F_CZLOWIEK|F_AI_CHODZI, NULL, 0, 1.5f, 5.f, 3.f, BLOOD_RED,
		&sounds_def, &fi_human, NULL, idle_czlowiek, countof(idle_czlowiek), 0.3f, 1.f, 0, 0, ArmorUnitType::HUMAN,
	"gornik", NULL, NULL, MAT_BODY, INT2(1,5),
		StatProfileType::WORKER,
		500, 0, gornik_items, NULL, INT2(20,50), INT2(30,60), dialog_gornik, G_MIESZKANCY,
		F_CZLOWIEK, NULL, 0, 1.5f, 5.f, 3.f, BLOOD_RED,
		&sounds_def, &fi_human, NULL, idle_czlowiek, countof(idle_czlowiek), 0.3f, 1.f, F2_AKTUALIZUJ_PRZEDMIOTY_V0, F3_GORNIK, ArmorUnitType::HUMAN,
	"gornik_szef", NULL, NULL, MAT_BODY, INT2(5),
		StatProfileType::WORKER,
		500, 0, gornik_items, NULL, INT2(50,100), INT2(50,100), dialog_inwestor, G_MIESZKANCY,
		F_CZLOWIEK|F_AI_STOI, NULL, 0, 1.5f, 5.f, 3.f, BLOOD_RED,
		&sounds_def, &fi_human, NULL, idle_czlowiek, countof(idle_czlowiek), 0.3f, 1.f, F2_AKTUALIZUJ_PRZEDMIOTY_V0, 0, ArmorUnitType::HUMAN,
	"pijak", NULL, NULL, MAT_BODY, INT2(2,20),
		StatProfileType::COMMONER,
		500, 0, poslaniec_items, NULL, INT2(8,22), INT2(80,220), dialog_pijak, G_MIESZKANCY,
		F_CZLOWIEK, NULL, 0, 1.5f, 5.f, 3.f, BLOOD_RED,
		&sounds_def, &fi_human, NULL, idle_czlowiek, countof(idle_czlowiek), 0.3f, 1.f, F2_ZAWODY_W_PICIU, F3_PIJAK_PO_ZAWODACH, ArmorUnitType::HUMAN,
	"mistrz_agentow", NULL,  NULL, MAT_BODY, INT2(10,15),
		StatProfileType::ROGUE,
		500, 0, rogue_items, NULL, INT2(50,200), INT2(100,300), dialog_q_bandyci, G_MIESZKANCY,
		F_CZLOWIEK|F_AI_STOI|F_BOHATER, NULL, 0, 1.5f, 5.f, 3.f, BLOOD_RED,
		&sounds_def, &fi_human, NULL, idle_czlowiek, countof(idle_czlowiek), 0.3f, 1.f, F2_OKRESLONE_IMIE | F2_BEZ_KLASY | F2_CIOS_W_PLECY, F3_NIE_JE, ArmorUnitType::HUMAN,
	"guard_q_bandyci", NULL, NULL, MAT_BODY, INT2(2,6),
		StatProfileType::FIGHTER,
		100, 0, q_guard_items, NULL, INT2(5, 25), INT2(20, 100), dialog_q_bandyci, G_MIESZKANCY,
		F_CZLOWIEK|F_AI_CHODZI, NULL, 0, 1.5f, 5.f, 3.f, BLOOD_RED,
		&sounds_def, &fi_human, NULL, idle_czlowiek, countof(idle_czlowiek), 0.3f, 1.f, F2_AI_TRENUJE, F3_NIE_JE, ArmorUnitType::HUMAN,
	"agent", NULL, NULL, MAT_BODY, INT2(6,12),
		StatProfileType::ROGUE,
		500, 0, rogue_items, NULL, INT2(10,50), INT2(20,60), dialog_q_bandyci, G_MIESZKANCY,
		F_CZLOWIEK|F_AI_STOI|F_BOHATER, NULL, 0, 1.5f, 5.f, 3.f, BLOOD_RED,
		&sounds_def, &fi_human, NULL, idle_czlowiek, countof(idle_czlowiek), 0.3f, 1.f, F2_BEZ_KLASY | F2_OKRESLONE_IMIE, F3_NIE_JE, ArmorUnitType::HUMAN,
	"q_magowie_uczony", NULL, NULL, MAT_BODY, INT2(1,5),
		StatProfileType::MAGE,
		500, 0, burmistrz_items, NULL, INT2(100,500), INT2(200,600), dialog_q_magowie, G_MIESZKANCY,
		F_CZLOWIEK|F_AI_STOI, NULL, 0, 1.5f, 5.f, 3.f, BLOOD_RED,
		&sounds_def, &fi_human, NULL, idle_czlowiek, countof(idle_czlowiek), 0.3f, 1.f, 0, 0, ArmorUnitType::HUMAN,
	"q_magowie_stary", NULL, NULL, MAT_BODY, INT2(3,15),
		StatProfileType::MAGE,
		500, 0, mage_items, &mage_spells, INT2(15,30), INT2(150,300), dialog_q_magowie2, G_MIESZKANCY,
		F_CZLOWIEK|F_MAG|F_BOHATER, NULL, 0, 1.5f, 5.f, 3.f, BLOOD_RED,
		&sounds_def, &fi_human, NULL, idle_czlowiek, countof(idle_czlowiek), 0.3f, 1.f, F2_STARY | F2_FLAGA_KLASY, F3_MAG_PIJAK, ArmorUnitType::HUMAN,
	"q_orkowie_straznik", NULL, NULL, MAT_BODY, INT2(2,6),
		StatProfileType::FIGHTER,
		500, 0, guard_items, NULL, INT2(5,25), INT2(20,100), dialog_q_orkowie, G_MIESZKANCY,
		F_CZLOWIEK|F_AI_CHODZI, NULL, 0, 1.5f, 5.f, 3.f, BLOOD_RED,
		&sounds_def, &fi_human, NULL, idle_czlowiek, countof(idle_czlowiek), 0.3f, 1.f, F2_AI_TRENUJE, F3_NIE_JE, ArmorUnitType::HUMAN,
	"q_orkowie_gorush", NULL, "ork.qmsh", MAT_BODY, INT2(5),
		StatProfileType::ORC,
		600, 5, gorush_items, NULL, INT2(5,20), INT2(10,30), dialog_q_orkowie2, G_MIESZKANCY,
		F_HUMANOID|F_BOHATER, NULL, 0, 1.5f, 5.f, 3.f, BLOOD_RED,
		&sounds_orc, &fi_ork, NULL, idle_goblin, countof(idle_goblin), 0.38f, 1.f, F2_AI_TRENUJE | F2_OKRESLONE_IMIE | F2_FLAGA_KLASY | F2_WOJOWNIK | F2_ZAWODY_W_PICIU_50 | F2_DZWIEK_ORK | F2_ZAWODY, F3_ORKOWE_JEDZENIE, ArmorUnitType::ORC,
	"q_orkowie_gorush_woj", NULL, "ork.qmsh", MAT_BODY, INT2(10),
		StatProfileType::GORUSH_WARRIOR,
		600, 10, gorush_woj_items, NULL, INT2(5,20), INT2(10,30), dialog_q_orkowie2, G_MIESZKANCY,
		F_HUMANOID|F_BOHATER, NULL, 0, 1.5f, 5.f, 3.f, BLOOD_RED,
		&sounds_orc, &fi_ork, NULL, idle_goblin, countof(idle_goblin), 0.38f, 1.f, F2_AI_TRENUJE | F2_OKRESLONE_IMIE | F2_FLAGA_KLASY | F2_WOJOWNIK | F2_ZAWODY_W_PICIU | F2_WALKA_WRECZ | F2_UZYWA_TRONU | F2_DZWIEK_ORK | F2_ZAWODY, F3_ORKOWE_JEDZENIE, ArmorUnitType::ORC,
	"q_orkowie_gorush_lowca", NULL, "ork.qmsh", MAT_BODY, INT2(10),
		StatProfileType::GORUSH_HUNTER,
		600, 10, gorush_lowca_items, NULL, INT2(5,20), INT2(10,30), dialog_q_orkowie2, G_MIESZKANCY,
		F_HUMANOID|F_BOHATER|F_LUCZNIK, NULL, 0, 1.5f, 5.f, 3.f, BLOOD_RED,
		&sounds_orc, &fi_ork, NULL, idle_goblin, countof(idle_goblin), 0.38f, 1.f, F2_AI_TRENUJE | F2_OKRESLONE_IMIE | F2_FLAGA_KLASY | F2_LOWCA | F2_ZAWODY_W_PICIU_50 | F2_UZYWA_TRONU | F2_DZWIEK_ORK | F2_ZAWODY, F3_ORKOWE_JEDZENIE, ArmorUnitType::ORC,
	"q_orkowie_gorush_szaman", NULL, "ork.qmsh", MAT_BODY, INT2(10),
		StatProfileType::SHAMAN,
		600, 5, gorush_szaman_items, &orc_shaman_spells, INT2(5,20), INT2(10,30), dialog_q_orkowie2, G_MIESZKANCY,
		F_HUMANOID|F_BOHATER|F_MAG, NULL, 0, 1.5f, 5.f, 3.f, BLOOD_RED,
		&sounds_orc, &fi_ork, ti_orc_shaman, idle_goblin, countof(idle_goblin), 0.41f, 1.f, F2_AI_TRENUJE | F2_OKRESLONE_IMIE | F2_FLAGA_KLASY | F2_UZYWA_TRONU | F2_DZWIEK_ORK | F2_ZAWODY, F3_ORKOWE_JEDZENIE, ArmorUnitType::ORC,
	"q_orkowie_slaby", NULL, "ork.qmsh", MAT_BODY, INT2(0),
		StatProfileType::ORC,
		600, 5, orkowie_slaby_items, NULL, INT2(5,10), INT2(5,10), dialog_q_orkowie2, G_MIESZKANCY,
		F_HUMANOID|F_TCHORZLIWY, NULL, 0, 1.5f, 5.f, 3.f, BLOOD_RED,
		&sounds_orc, &fi_ork, NULL, idle_goblin, countof(idle_goblin), 0.38f, 1.f, F2_DZWIEK_ORK, F3_ORKOWE_JEDZENIE, ArmorUnitType::ORC,
	"q_orkowie_kowal", NULL, "ork.qmsh", MAT_BODY, INT2(5,9),
		StatProfileType::ORC_BLACKSMITH,
		600, 10, orc_fighter_items, NULL, INT2(100,200), INT2(100,200), dialog_q_orkowie2, G_MIESZKANCY,
		F_HUMANOID|F_AI_STOI, NULL, 0, 1.5f, 5.f, 3.f, BLOOD_RED,
		&sounds_orc, &fi_ork, NULL, idle_goblin, countof(idle_goblin), 0.41f, 1.f, F2_DZWIEK_ORK, F3_ORKOWE_JEDZENIE, ArmorUnitType::ORC,
	"q_orkowie_orc", NULL, "ork.qmsh", MAT_BODY, INT2(1,4),
		StatProfileType::ORC,
		600, 5, orc_items, NULL, INT2(5,20), INT2(10,30), dialog_q_orkowie2, G_MIESZKANCY,
		F_HUMANOID, NULL, 0, 1.5f, 5.f, 3.f, BLOOD_RED,
		&sounds_orc, &fi_ork, NULL, idle_goblin, countof(idle_goblin), 0.41f, 1.f, F2_AI_TRENUJE | F2_DZWIEK_ORK, F3_ORKOWE_JEDZENIE, ArmorUnitType::ORC,
	"q_orkowie_orc_hunter", NULL, "ork.qmsh", MAT_BODY, INT2(5,9),
		StatProfileType::ORC,
		600, 10, orc_hunter_items, NULL, INT2(12,32), INT2(25,50), dialog_q_orkowie2, G_MIESZKANCY,
		F_HUMANOID|F_LUCZNIK, NULL, 0, 1.5f, 5.f, 3.f, BLOOD_RED,
		&sounds_orc, &fi_ork, NULL, idle_goblin, countof(idle_goblin), 0.41f, 1.f, F2_AI_TRENUJE | F2_DZWIEK_ORK, F3_ORKOWE_JEDZENIE, ArmorUnitType::ORC,
	"q_orkowie_orc_fighter", NULL, "ork.qmsh", MAT_BODY, INT2(5,9),
		StatProfileType::ORC,
		600, 10, orc_fighter_items, NULL, INT2(12,32), INT2(25,50), dialog_q_orkowie2, G_MIESZKANCY,
		F_HUMANOID, NULL, 0, 1.5f, 5.f, 3.f, BLOOD_RED,
		&sounds_orc, &fi_ork, NULL, idle_goblin, countof(idle_goblin), 0.41f, 1.f, F2_AI_TRENUJE | F2_DZWIEK_ORK, F3_ORKOWE_JEDZENIE, ArmorUnitType::ORC,
	"q_orkowie_orc_chief", NULL, "ork.qmsh", MAT_BODY, INT2(10,15),
		StatProfileType::ORC,
		600, 15, orc_chief_items, NULL, INT2(50,100), INT2(100,200), dialog_q_orkowie2, G_MIESZKANCY,
		F_HUMANOID|F_DOWODCA, NULL, 0, 1.5f, 5.f, 3.f, BLOOD_RED,
		&sounds_orc, &fi_ork, NULL, idle_goblin, countof(idle_goblin), 0.41f, 1.f, F2_DZWIEK_ORK, F3_ORKOWE_JEDZENIE, ArmorUnitType::ORC,
	"q_orkowie_orc_shaman", NULL, "ork.qmsh", MAT_BODY, INT2(6,11),
		StatProfileType::SHAMAN,
		600, 5, orc_shaman_items, &orc_shaman_spells, INT2(20,40), INT2(30,60), dialog_q_orkowie2, G_MIESZKANCY,
		F_HUMANOID|F_MAG, NULL, 0, 1.5f, 5.f, 3.f, BLOOD_RED,
		&sounds_orc, &fi_ork, ti_orc_shaman, idle_goblin, countof(idle_goblin), 0.41f, 1.f, F2_DZWIEK_ORK, F3_ORKOWE_JEDZENIE, ArmorUnitType::ORC,
	"q_gobliny_szlachcic", NULL, NULL, MAT_BODY, INT2(18),
		StatProfileType::ROGUE,
		500, 0, burmistrz_items, NULL, INT2(100,500), INT2(200,600), dialog_q_gobliny, G_MIESZKANCY,
		F_CZLOWIEK|F_AI_STOI|F_BOHATER, NULL, 0, 1.5f, 5.f, 3.f, BLOOD_RED,
		&sounds_def, &fi_human, NULL, idle_czlowiek, countof(idle_czlowiek), 0.3f, 1.f, F2_OKRESLONE_IMIE | F2_BEZ_KLASY, 0, ArmorUnitType::HUMAN,
	"q_gobliny_poslaniec", NULL, NULL, MAT_BODY, INT2(1,5),
		StatProfileType::COMMONER,
		500, 0, poslaniec_items, NULL, INT2(10,50), INT2(20,60), dialog_q_gobliny, G_MIESZKANCY,
		F_CZLOWIEK|F_AI_CHODZI, NULL, 0, 1.5f, 5.f, 3.f, BLOOD_RED,
		&sounds_def, &fi_human, NULL, idle_czlowiek, countof(idle_czlowiek), 0.3f, 1.f, 0, 0, ArmorUnitType::HUMAN,
	"q_gobliny_mag",NULL, NULL, MAT_BODY, INT2(3,15),
		StatProfileType::MAGE,
		500, 0, mage_items, &mage_spells, INT2(15,30), INT2(150,300), dialog_q_gobliny, G_MIESZKANCY,
		F_CZLOWIEK|F_MAG, NULL, 0, 1.5f, 5.f, 3.f, BLOOD_RED,
		&sounds_def, &fi_human, NULL, idle_czlowiek, countof(idle_czlowiek), 0.3f, 1.f, 0, 0, ArmorUnitType::HUMAN,
	"q_zlo_kaplan", NULL, NULL, MAT_BODY, INT2(0,20),
		StatProfileType::CLERIC,
		500, 0, kaplan_items, &jozan_spells, INT2(8,22), INT2(80,220), dialog_q_zlo, G_MIESZKANCY,
		F_CZLOWIEK|F_BOHATER|F_AI_STOI, NULL, 0, 1.5f, 5.f, 3.f, BLOOD_RED,
		&sounds_def, &fi_human, NULL, idle_czlowiek, countof(idle_czlowiek), 0.3f, 1.f, F2_AI_TRENUJE | F2_FLAGA_KLASY | F2_OKRESLONE_IMIE | F2_KAPLAN, 0, ArmorUnitType::HUMAN,
	"q_zlo_mag", NULL, NULL, MAT_BODY, INT2(3,15),
		StatProfileType::MAGE,
		500, 0, mage_items, &mage_spells, INT2(15,30), INT2(150,300), dialog_q_zlo, G_MIESZKANCY,
		F_CZLOWIEK|F_MAG, NULL, 0, 1.5f, 5.f, 3.f, BLOOD_RED,
		&sounds_def, &fi_human, NULL, idle_czlowiek, countof(idle_czlowiek), 0.3f, 1.f, 0, 0, ArmorUnitType::HUMAN,

	//---- GOBLINY ----
	"goblin", NULL, "goblin.qmsh", MAT_BODY, INT2(1,3),
		StatProfileType::GOBLIN,
		400, 0, goblin_items, NULL, INT2(1,10), INT2(3,14), NULL, G_GOBLINY,
		F_HUMANOID|F_TCHORZLIWY, NULL, 0, 1.75f, 6.f, 3.5f, BLOOD_RED,
		&sounds_goblin, &fi_goblin, NULL, idle_goblin, countof(idle_goblin), 0.3f, 0.8f, F2_AI_TRENUJE | F2_DZWIEK_GOBLIN, F3_ORKOWE_JEDZENIE, ArmorUnitType::GOBLIN,
	"goblin_hunter", NULL, "goblin.qmsh", MAT_BODY, INT2(4,6),
		StatProfileType::GOBLIN,
		400, 0, goblin_hunter_items, NULL, INT2(4,16), INT2(6,20), NULL, G_GOBLINY,
		F_HUMANOID|F_TCHORZLIWY|F_LUCZNIK, NULL, 0, 1.75f, 6.f, 3.5f, BLOOD_RED,
		&sounds_goblin, &fi_goblin, NULL, idle_goblin, countof(idle_goblin), 0.3f, 0.8f, F2_AI_TRENUJE | F2_DZWIEK_GOBLIN, F3_ORKOWE_JEDZENIE, ArmorUnitType::GOBLIN,
	"goblin_fighter", NULL, "goblin.qmsh", MAT_BODY, INT2(4,6),
		StatProfileType::GOBLIN,
		400, 0, goblin_fighter_items, NULL, INT2(4,16), INT2(6,20), NULL, G_GOBLINY,
		F_HUMANOID|F_TCHORZLIWY, NULL, 0, 1.75f, 6.f, 3.5f, BLOOD_RED,
		&sounds_goblin, &fi_goblin, NULL, idle_goblin, countof(idle_goblin), 0.3f, 0.8f, F2_AI_TRENUJE | F2_DZWIEK_GOBLIN, F3_ORKOWE_JEDZENIE, ArmorUnitType::GOBLIN,
	"goblin_chief", NULL, "goblin.qmsh", MAT_BODY, INT2(7,10),
		StatProfileType::GOBLIN,
		400, 0, goblin_chief_items, NULL, INT2(20,50), INT2(50,100), NULL, G_GOBLINY,
		F_HUMANOID|F_TCHORZLIWY|F_DOWODCA, NULL, 0, 1.75f, 6.f, 3.5f, BLOOD_RED,
		&sounds_goblin, &fi_goblin, NULL, idle_goblin, countof(idle_goblin), 0.3f, 0.8f, F2_DZWIEK_GOBLIN, F3_ORKOWE_JEDZENIE, ArmorUnitType::GOBLIN,
	"goblin_chief_q", NULL, "goblin.qmsh", MAT_BODY, INT2(7,10),
		StatProfileType::GOBLIN,
		400, 0, goblin_chief_items, NULL, INT2(20,50), INT2(50,100), NULL, G_GOBLINY,
		F_HUMANOID|F_TCHORZLIWY|F_DOWODCA, NULL, 0, 1.75f, 6.f, 3.5f, BLOOD_RED,
		&sounds_goblin, &fi_goblin, NULL, idle_goblin, countof(idle_goblin), 0.3f, 0.8f, F2_DZWIEK_GOBLIN | F2_OZNACZ, F3_ORKOWE_JEDZENIE, ArmorUnitType::GOBLIN,
	"q_gobliny_szlachcic2", NULL, NULL, MAT_BODY, INT2(18),
		StatProfileType::ROGUE,
		600, 25, szlachcic_items, NULL, INT2(1000,2000), INT2(1000,2000), dialog_q_gobliny, G_GOBLINY,
		F_CZLOWIEK|F_LUCZNIK|F_BOHATER|F_AI_CHODZI|F_DOWODCA, NULL, 0, 1.5f, 5.f, 3.f, BLOOD_RED,
		&sounds_def, &fi_human, NULL, idle_czlowiek, countof(idle_czlowiek), 0.3f, 1.f, F2_OKRESLONE_IMIE | F2_BEZ_KLASY | F2_BOSS | F2_UZYWA_TRONU | F2_OZNACZ | F2_NIE_GOBLIN, 0, ArmorUnitType::HUMAN,
	"q_gobliny_ochroniarz", NULL, NULL, MAT_BODY, INT2(5,10),
		StatProfileType::FIGHTER,
		500, 0, guard_items, NULL, INT2(10,20), INT2(50,100), dialog_ochroniarz, G_GOBLINY,
		F_CZLOWIEK, NULL, 0, 1.5f, 5.f, 3.f, BLOOD_RED,
		&sounds_def, &fi_human, NULL, idle_czlowiek, countof(idle_czlowiek), 0.3f, 1.f, F2_AI_TRENUJE | F2_NIE_GOBLIN, 0, ArmorUnitType::HUMAN,
	"tut_goblin", NULL, "goblin.qmsh", MAT_BODY, INT2(1,3),
		StatProfileType::GOBLIN,
		400, 0, tut_goblin_items, NULL, INT2(1,10), INT2(3,14), NULL, G_GOBLINY,
		F_HUMANOID|F_TCHORZLIWY|F_AI_STOI|F_AI_STRAZNIK, NULL, 0, 1.75f, 6.f, 3.5f, BLOOD_RED,
		&sounds_goblin, &fi_goblin, NULL, idle_goblin, countof(idle_goblin), 0.3f, 0.8f, F2_DZWIEK_GOBLIN | F2_OZNACZ, F3_ORKOWE_JEDZENIE, ArmorUnitType::GOBLIN,

	//---- ORKOWIE ----
	"orc", NULL, "ork.qmsh", MAT_BODY, INT2(1,4),
		StatProfileType::ORC,
		600, 5, orc_items, NULL, INT2(5,20), INT2(10,30), NULL, G_ORKOWIE,
		F_HUMANOID, NULL, 0, 1.5f, 5.f, 3.f, BLOOD_RED,
		&sounds_orc, &fi_ork, NULL, idle_goblin, countof(idle_goblin), 0.41f, 1.f, F2_AI_TRENUJE | F2_DZWIEK_ORK, F3_ORKOWE_JEDZENIE, ArmorUnitType::ORC,
	"orc_hunter", NULL, "ork.qmsh", MAT_BODY, INT2(5,9),
		StatProfileType::ORC,
		600, 10, orc_hunter_items, NULL, INT2(12,32), INT2(25,50), NULL, G_ORKOWIE,
		F_HUMANOID|F_LUCZNIK, NULL, 0, 1.5f, 5.f, 3.f, BLOOD_RED,
		&sounds_orc, &fi_ork, NULL, idle_goblin, countof(idle_goblin), 0.41f, 1.f, F2_AI_TRENUJE | F2_DZWIEK_ORK, F3_ORKOWE_JEDZENIE, ArmorUnitType::ORC,
	"orc_fighter", NULL, "ork.qmsh", MAT_BODY, INT2(5,9),
		StatProfileType::ORC,
		600, 10, orc_fighter_items, NULL, INT2(12,32), INT2(25,50), NULL, G_ORKOWIE,
		F_HUMANOID, NULL, 0, 1.5f, 5.f, 3.f, BLOOD_RED,
		&sounds_orc, &fi_ork, NULL, idle_goblin, countof(idle_goblin), 0.41f, 1.f, F2_AI_TRENUJE | F2_WALKA_WRECZ | F2_DZWIEK_ORK, F3_ORKOWE_JEDZENIE, ArmorUnitType::ORC,
	"orc_chief", NULL, "ork.qmsh", MAT_BODY, INT2(10,15),
		StatProfileType::ORC,
		600, 15, orc_chief_items, NULL, INT2(50,100), INT2(100,200), NULL, G_ORKOWIE,
		F_HUMANOID|F_DOWODCA, NULL, 0, 1.5f, 5.f, 3.f, BLOOD_RED,
		&sounds_orc, &fi_ork, NULL, idle_goblin, countof(idle_goblin), 0.41f, 1.f, F2_WALKA_WRECZ | F2_DZWIEK_ORK, F3_ORKOWE_JEDZENIE, ArmorUnitType::ORC,
	"orc_chief_q", NULL, "ork.qmsh", MAT_BODY, INT2(10,15),
		StatProfileType::ORC,
		600, 15, orc_chief_items, NULL, INT2(50,100), INT2(100,200), NULL, G_ORKOWIE,
		F_HUMANOID|F_DOWODCA, NULL, 0, 1.5f, 5.f, 3.f, BLOOD_RED,
		&sounds_orc, &fi_ork, NULL, idle_goblin, countof(idle_goblin), 0.41f, 1.f, F2_WALKA_WRECZ | F2_DZWIEK_ORK | F2_OZNACZ, F3_ORKOWE_JEDZENIE, ArmorUnitType::ORC,
	"orc_shaman", NULL, "ork.qmsh", MAT_BODY, INT2(6,11),
		StatProfileType::SHAMAN,
		600, 5, orc_shaman_items, &orc_shaman_spells, INT2(20,40), INT2(30,60), NULL, G_ORKOWIE,
		F_HUMANOID|F_MAG, NULL, 0, 1.5f, 5.f, 3.f, BLOOD_RED,
		&sounds_orc, &fi_ork, ti_orc_shaman, idle_goblin, countof(idle_goblin), 0.41f, 1.f, F2_DZWIEK_ORK, F3_ORKOWE_JEDZENIE, ArmorUnitType::ORC,
	"q_orkowie_boss", NULL, "ork.qmsh", MAT_BODY, INT2(18),
		StatProfileType::ORC,
		800, 30, orkowie_boss_items, NULL, INT2(1000,2000), INT2(1000,2000), NULL, G_ORKOWIE,
		F_HUMANOID|F_DOWODCA, NULL, 0, 1.5f, 5.f, 3.f, BLOOD_RED,
		&sounds_orc_boss, &fi_ork, NULL, idle_goblin, countof(idle_goblin), 0.41f, 1.f, F2_WALKA_WRECZ | F2_BOSS | F2_UZYWA_TRONU | F2_DZWIEK_ORK | F2_OKRZYK | F2_OZNACZ | F2_OBSTAWA, F3_ORKOWE_JEDZENIE, ArmorUnitType::ORC,

	//---- NIEUMARLI I Z£O ----
	"zombie", NULL, "zombie.qmsh", MAT_BODY, INT2(2,4),
		StatProfileType::ZOMBIE,
		800, 25, NULL, NULL, INT2(0,0), INT2(0,0), NULL, G_NIEUMARLI,
		F_NIEUMARLY|F_NIE_UCIEKA|F_POWOLNY|F_KLUTE25|F_OBUCHOWE25|F_NIE_OTWIERA|F_ODPORNOSC_NA_TRUCIZNY|F_BRAK_POTEZNEGO_ATAKU, NULL, DMG_BLUNT, 1.5f, 0.f, 2.5f, BLOOD_GREEN,
		&sounds_zombie, &fi_zombie, NULL, idle_zombie, countof(idle_zombie), 0.3f, 1.f, F2_OMIJA_BLOK | F2_ODPORNOSC_NA_CIOS_W_PLECY, F3_NIE_JE, ArmorUnitType::NONE,
	"zombie_rotting", NULL, "zombie.qmsh", MAT_BODY, INT2(4,8),
		StatProfileType::ZOMBIE,
		800, 50, NULL, &zombie_rotting_spells, INT2(0,0), INT2(0,0), NULL, G_NIEUMARLI,
		F_NIEUMARLY|F_NIE_UCIEKA|F_POWOLNY|F_KLUTE25|F_OBUCHOWE25|F_NIE_OTWIERA|F_ODPORNOSC_NA_TRUCIZNY|F_BRAK_POTEZNEGO_ATAKU, NULL, DMG_BLUNT, 1.5f, 0.f, 2.5f, BLOOD_GREEN,
		&sounds_zombie, &fi_zombie, ti_zombie_rotting, idle_zombie, countof(idle_zombie), 0.3f, 1.f, F2_OMIJA_BLOK | F2_ODPORNOSC_NA_CIOS_W_PLECY, F3_NIE_JE, ArmorUnitType::NONE,
	"zombie_ancient", NULL, "zombie.qmsh", MAT_BODY, INT2(8,12),
		StatProfileType::ZOMBIE,
		800, 75, NULL, NULL, INT2(0,0), INT2(0,0), NULL, G_NIEUMARLI,
		F_NIEUMARLY|F_NIE_UCIEKA|F_POWOLNY|F_KLUTE25|F_OBUCHOWE25|F_NIE_OTWIERA|F_ODPORNOSC_NA_TRUCIZNY|F_BRAK_POTEZNEGO_ATAKU, NULL, DMG_BLUNT, 1.3f, 0.f, 2.2f, BLOOD_BLACK,
		&sounds_zombie, &fi_zombie, ti_zombie_ancient, idle_zombie, countof(idle_zombie), 0.3f, 1.f, F2_OMIJA_BLOK | F2_ODPORNOSC_NA_CIOS_W_PLECY | F2_ODPORNOSC_NA_MAGIE_25, F3_NIE_JE, ArmorUnitType::NONE,
	//
	"skeleton", NULL, "skeleton.qmsh", MAT_BONE, INT2(2,4),
		StatProfileType::SKELETON,
		450, 20, skeleton_items, NULL, INT2(0,0), INT2(0,0), NULL, G_NIEUMARLI,
		F_HUMANOID|F_NIEUMARLY|F_NIE_UCIEKA|F_KLUTE25|F_OBUCHOWE_MINUS25|F_ODPORNOSC_NA_TRUCIZNY, NULL, 0, 1.5f, 5.f, 3.f, BLOOD_BONE,
		&sounds_skeleton, &fi_skeleton, NULL, idle_szkielet, countof(idle_szkielet), 0.3f, 1.f, F2_BRAK_KRWII | F2_ODPORNOSC_NA_CIOS_W_PLECY, F3_NIE_JE, ArmorUnitType::NONE,
	"skeleton_archer", NULL, "skeleton.qmsh", MAT_BONE, INT2(3,6),
		StatProfileType::SKELETON,
		450, 20, skeleton_archer_items, NULL, INT2(0,0), INT2(0,0), NULL, G_NIEUMARLI,
		F_HUMANOID|F_NIEUMARLY|F_NIE_UCIEKA|F_KLUTE25|F_OBUCHOWE_MINUS25|F_LUCZNIK|F_ODPORNOSC_NA_TRUCIZNY, NULL, 0, 1.5f, 5.f, 3.f, BLOOD_BONE,
		&sounds_skeleton, &fi_skeleton, NULL, idle_szkielet, countof(idle_szkielet), 0.3f, 1.f, F2_BRAK_KRWII | F2_ODPORNOSC_NA_CIOS_W_PLECY, F3_NIE_JE, ArmorUnitType::NONE,
	"skeleton_fighter", NULL, "skeleton.qmsh", MAT_BONE, INT2(3,6),
		StatProfileType::SKELETON,
		450, 25, skeleton_fighter_items, NULL, INT2(0,0), INT2(0,0), NULL, G_NIEUMARLI,
		F_HUMANOID|F_NIEUMARLY|F_NIE_UCIEKA|F_KLUTE25|F_OBUCHOWE_MINUS25|F_ODPORNOSC_NA_TRUCIZNY, NULL, 0, 1.5f, 5.f, 3.f, BLOOD_BONE,
		&sounds_skeleton, &fi_skeleton, NULL, idle_szkielet, countof(idle_szkielet), 0.3f, 1.f, F2_WALKA_WRECZ | F2_BRAK_KRWII | F2_ODPORNOSC_NA_CIOS_W_PLECY, F3_NIE_JE, ArmorUnitType::NONE,
	"skeleton_mage", NULL, "skeleton.qmsh", MAT_BONE, INT2(4,12),
		StatProfileType::SKELETON_MAGE,
		450, 20, skeleton_mage_items, &skeleton_mage_spells, INT2(0,0), INT2(0,0), NULL, G_NIEUMARLI,
		F_HUMANOID|F_NIEUMARLY|F_NIE_UCIEKA|F_KLUTE25|F_OBUCHOWE_MINUS25|F_MAG|F_ODPORNOSC_NA_TRUCIZNY, NULL, 0, 1.5f, 5.f, 3.f, BLOOD_BONE,
		&sounds_skeleton, &fi_skeleton, NULL, idle_szkielet, countof(idle_szkielet), 0.3f, 1.f, F2_BRAK_KRWII | F2_ODPORNOSC_NA_CIOS_W_PLECY, F3_NIE_JE, ArmorUnitType::NONE,
	//
	"necromancer", NULL, NULL, MAT_BODY, INT2(3,15),
		StatProfileType::MAGE,
		500, 0, necromancer_items, &necromancer_spells, INT2(15,30), INT2(120,240), NULL, G_NIEUMARLI,
		F_CZLOWIEK|F_MAG|F_DOWODCA|F_SZARE_WLOSY, NULL, 0, 1.5f, 5.f, 3.f, BLOOD_BLACK,
		&sounds_def, &fi_human, ti_necromant, idle_czlowiek, countof(idle_czlowiek), 0.3f, 1.f, 0, 0, ArmorUnitType::HUMAN,
	"undead_guard", NULL, NULL, MAT_BODY, INT2(2,10),
		StatProfileType::WARRIOR,
		525, 10, warrior_items, NULL, INT2(10,20), INT2(40,80), NULL, G_NIEUMARLI,
		F_CZLOWIEK|F_NIEUMARLY|F_NIE_UCIEKA|F_OBUCHOWE25|F_ODPORNOSC_NA_TRUCIZNY|F_SZARE_WLOSY, NULL, 0, 1.5f, 5.f, 3.f, BLOOD_BLACK,
		&sounds_undead, &fi_human, ti_undead, idle_czlowiek, countof(idle_czlowiek), 0.3f, 1.f, 0, F3_NIE_JE, ArmorUnitType::HUMAN,
	"undead_archer", NULL, NULL, MAT_BODY, INT2(2,10),
		StatProfileType::HUNTER,
		525, 10, hunter_items, NULL, INT2(10,20), INT2(40,80), NULL, G_NIEUMARLI,
		F_CZLOWIEK|F_NIEUMARLY|F_NIE_UCIEKA|F_OBUCHOWE25|F_ODPORNOSC_NA_TRUCIZNY|F_SZARE_WLOSY, NULL, 0, 1.5f, 5.f, 3.f, BLOOD_BLACK,
		&sounds_undead, &fi_human, ti_undead, idle_czlowiek, countof(idle_czlowiek), 0.3f, 1.f, 0, F3_NIE_JE, ArmorUnitType::HUMAN,
	//
	"evil_cleric", NULL, NULL, MAT_BODY, INT2(4,15),
		StatProfileType::CLERIC,
		525, 0, evil_cleric_items, &evil_cleric_spells, INT2(25,50), INT2(160,320), NULL, G_NIEUMARLI,
		F_CZLOWIEK|F_DOWODCA|F_SZARE_WLOSY, NULL, 0, 1.5f, 5.f, 3.f, BLOOD_BLACK,
		&sounds_def, &fi_human, ti_evil, idle_czlowiek, countof(idle_czlowiek), 0.3f, 1.f, F2_AI_TRENUJE, 0, ArmorUnitType::HUMAN,
	"evil_cleric_q", NULL, NULL, MAT_BODY, INT2(4,15),
		StatProfileType::CLERIC,
		525, 0, evil_cleric_items, &evil_cleric_spells, INT2(25,50), INT2(160,320), NULL, G_NIEUMARLI,
		F_CZLOWIEK|F_DOWODCA|F_SZARE_WLOSY, NULL, 0, 1.5f, 5.f, 3.f, BLOOD_BLACK,
		&sounds_def, &fi_human, ti_evil, idle_czlowiek, countof(idle_czlowiek), 0.3f, 1.f, F2_AI_TRENUJE | F2_OZNACZ, 0, ArmorUnitType::HUMAN,
	"q_zlo_boss", NULL, NULL, MAT_BODY, INT2(20),
		StatProfileType::EVIL_BOSS,
		800, 50, xar_items, &xar_spells, INT2(4000,6000), INT2(4000,6000), dialog_q_zlo, G_NIEUMARLI,
		F_CZLOWIEK|F_DOWODCA|F_NIE_UCIEKA|F_ODPORNOSC_NA_TRUCIZNY|F_SZARE_WLOSY, NULL, 0, 1.5f, 5.f, 3.f, BLOOD_BLACK,
		&sounds_boss, &fi_human, ti_evil_boss, idle_czlowiek, countof(idle_czlowiek), 0.3f, 1.f, F2_WALKA_WRECZ | F2_BOSS | F2_XAR | F2_ODPORNOSC_NA_MAGIE_25 | F2_ODPORNOSC_NA_CIOS_W_PLECY | F2_OZNACZ, F3_NIE_JE, ArmorUnitType::HUMAN,

	//---- BANDYCI ----
	"bandit", NULL, NULL, MAT_BODY, INT2(2,6),
		StatProfileType::FIGHTER,
		500, 0, bandit_items, NULL, INT2(15,30), INT2(20,40), dialog_bandyta, G_BANDYCI,
		F_CZLOWIEK, NULL, 0, 1.5f, 5.f, 3.f, BLOOD_RED,
		&sounds_def, &fi_human, NULL, idle_czlowiek, countof(idle_czlowiek), 0.3f, 1.f, F2_AI_TRENUJE, 0, ArmorUnitType::HUMAN,
	"bandit_archer", NULL, NULL, MAT_BODY, INT2(3,7),
		StatProfileType::FIGHTER,
		500, 0, bandit_archer_items, NULL, INT2(25,50), INT2(30,60), dialog_bandyta, G_BANDYCI,
		F_CZLOWIEK|F_LUCZNIK, NULL, 0, 1.5f, 5.f, 3.f, BLOOD_RED,
		&sounds_def, &fi_human, NULL, idle_czlowiek, countof(idle_czlowiek), 0.3f, 1.f, F2_AI_TRENUJE, 0, ArmorUnitType::HUMAN,
	"bandit_hegemon", NULL, NULL, MAT_BODY, INT2(8,12),
		StatProfileType::FIGHTER,
		500, 0, bandit_hegemon_items, NULL, INT2(75,150), INT2(100,200), dialog_bandyta, G_BANDYCI,
		F_CZLOWIEK|F_DOWODCA, NULL, 0, 1.5f, 5.f, 3.f, BLOOD_RED,
		&sounds_def, &fi_human, NULL, idle_czlowiek, countof(idle_czlowiek), 0.3f, 1.f, F2_CIOS_W_PLECY, 0, ArmorUnitType::HUMAN,
	"bandit_hegemon_q", NULL, NULL, MAT_BODY, INT2(8,12),
		StatProfileType::FIGHTER,
		500, 0, bandit_hegemon_items, NULL, INT2(75,150), INT2(100,200), dialog_bandyta, G_BANDYCI,
		F_CZLOWIEK|F_DOWODCA, NULL, 0, 1.5f, 5.f, 3.f, BLOOD_RED,
		&sounds_def, &fi_human, NULL, idle_czlowiek, countof(idle_czlowiek), 0.3f, 1.f, F2_CIOS_W_PLECY | F2_OZNACZ, 0, ArmorUnitType::HUMAN,
	"q_bandyci_szef", NULL, NULL, MAT_BODY, INT2(18),
		StatProfileType::FIGHTER,
		600, 25, q_bandyci_szef_items, NULL, INT2(1000,2000), INT2(1000,2000), dialog_q_bandyci, G_BANDYCI,
		F_CZLOWIEK|F_AI_CHODZI, NULL, 0, 1.5f, 5.f, 3.f, BLOOD_RED,
		&sounds_def, &fi_human, NULL, idle_czlowiek, countof(idle_czlowiek), 0.3f, 1.f, F2_WALKA_WRECZ | F2_BOSS | F2_CIOS_W_PLECY | F2_OZNACZ | F2_OBSTAWA, 0, ArmorUnitType::HUMAN,

	//---- MAGOWIE I GOLEMY ----
	"mage", NULL, NULL, MAT_BODY, INT2(3, 15),
		StatProfileType::MAGE,
		500, 0, mage_items, &mage_spells, INT2(15,30), INT2(150,300), dialog_mag_obstawa, G_MAGOWIE,
		F_CZLOWIEK|F_MAG, NULL, 0, 1.5f, 5.f, 3.f, BLOOD_RED,
		&sounds_def, &fi_human, NULL, idle_czlowiek, countof(idle_czlowiek), 0.3f, 1.f, 0, 0, ArmorUnitType::HUMAN,
	"mage_q", NULL, NULL, MAT_BODY, INT2(3,15),
		StatProfileType::MAGE,
		500, 0, mage_items, &mage_spells, INT2(15,30), INT2(150,300), dialog_mag_obstawa, G_MAGOWIE,
		F_CZLOWIEK|F_MAG, NULL, 0, 1.5f, 5.f, 3.f, BLOOD_RED,
		&sounds_def, &fi_human, NULL, idle_czlowiek, countof(idle_czlowiek), 0.3f, 1.f, F2_OZNACZ, 0, ArmorUnitType::HUMAN,
	"mage_guard", NULL, NULL, MAT_BODY, INT2(2,10),
		StatProfileType::FIGHTER,
		500, 0, guard_items, NULL, INT2(10,20), INT2(50,100), dialog_mag_obstawa, G_MAGOWIE,
		F_CZLOWIEK, NULL, 0, 1.5f, 5.f, 3.f, BLOOD_RED,
		&sounds_def, &fi_human, NULL, idle_czlowiek, countof(idle_czlowiek), 0.3f, 1.f, F2_AI_TRENUJE, 0, ArmorUnitType::HUMAN,
	"golem_stone", NULL, "golem.qmsh", MAT_ROCK, INT2(8),
		StatProfileType::GOLEM,
		800, 50, NULL, NULL, INT2(0,0), INT2(0,0), NULL, G_MAGOWIE,
		F_NIE_UCIEKA|F_KLUTE25|F_CIETE25|F_OBUCHOWE_MINUS25|F_NIE_CIERPI|F_ODPORNOSC_NA_TRUCIZNY, NULL, DMG_BLUNT, 1.5f, 3.f, 2.f, BLOOD_ROCK,
		&sounds_golem, &fi_golem, NULL, idle_golem, countof(idle_golem), 0.5f, 1.2f, F2_BRAK_KRWII | F2_OMIJA_BLOK | F2_ODPORNOSC_NA_MAGIE_25 | F2_ODPORNOSC_NA_CIOS_W_PLECY, F3_NIE_JE, ArmorUnitType::NONE,
	"golem_iron", NULL, "golem.qmsh", MAT_IRON, INT2(12),
		StatProfileType::GOLEM,
		900, 100, NULL, NULL, INT2(0,0), INT2(0,0), NULL, G_MAGOWIE,
		F_NIE_UCIEKA|F_KLUTE25|F_CIETE25|F_NIE_CIERPI|F_ODPORNOSC_NA_TRUCIZNY, NULL, DMG_BLUNT, 1.5f, 3.f, 2.f, BLOOD_IRON,
		&sounds_golem_iron, &fi_golem, ti_golem_iron, idle_golem, countof(idle_golem), 0.5f, 1.2f, F2_BRAK_KRWII | F2_OMIJA_BLOK | F2_ODPORNOSC_NA_MAGIE_25 | F2_ODPORNOSC_NA_CIOS_W_PLECY, F3_NIE_JE, ArmorUnitType::NONE,
	"golem_adamantine", NULL, "golem.qmsh", MAT_IRON, INT2(20),
		StatProfileType::GOLEM,
		1000, 150, NULL, NULL, INT2(0), INT2(0), NULL, G_MAGOWIE,
		F_NIE_UCIEKA|F_KLUTE25|F_CIETE25|F_OBUCHOWE25|F_NIE_CIERPI|F_ODPORNOSC_NA_TRUCIZNY, NULL, DMG_BLUNT, 1.5f, 3.f, 2.f, BLOOD_IRON,
		&sounds_def, &fi_golem, ti_golem_adamantite, idle_golem, countof(idle_golem), 0.5f, 1.2f, F2_BRAK_KRWII | F2_OMIJA_BLOK | F2_ODPORNOSC_NA_MAGIE_50 | F2_ODPORNOSC_NA_CIOS_W_PLECY, F3_NIE_JE, ArmorUnitType::NONE,
	"q_magowie_golem", NULL, "golem.qmsh", MAT_ROCK, INT2(8,8),
		StatProfileType::GOLEM,
		800, 50, NULL, NULL, INT2(0,0), INT2(0,0), dialog_q_magowie, G_MAGOWIE,
		F_NIE_UCIEKA|F_KLUTE25|F_CIETE25|F_OBUCHOWE_MINUS25|F_NIE_CIERPI|F_ODPORNOSC_NA_TRUCIZNY, NULL, DMG_BLUNT, 1.5f, 3.f, 2.f, BLOOD_ROCK,
		&sounds_golem, &fi_golem, NULL, idle_golem, countof(idle_golem), 0.5f, 1.2f, F2_BRAK_KRWII | F2_DZWIEK_GOLEM | F2_OMIJA_BLOK | F2_ODPORNOSC_NA_MAGIE_25 | F2_ODPORNOSC_NA_CIOS_W_PLECY, F3_NIE_JE, ArmorUnitType::NONE,
	"q_magowie_boss", NULL, NULL, MAT_BODY, INT2(20),
		StatProfileType::MAGE,
		600, 25, q_magowie_boss_items, &mage_boss_spells, INT2(1000,2000), INT2(1000,2000), dialog_q_magowie2, G_MAGOWIE,
		F_CZLOWIEK|F_MAG, NULL, 0, 1.5f, 5.f, 3.f, BLOOD_RED,
		&sounds_def, &fi_human, NULL, idle_czlowiek, countof(idle_czlowiek), 0.3f, 1.f, F2_STARY | F2_BOSS | F2_OZNACZ | F2_OBSTAWA, 0, ArmorUnitType::HUMAN,

	//---- ZWIERZ TA I £OWCY ----
	"wolf", NULL, "wilk.qmsh", MAT_SKIN, INT2(1,3),
		StatProfileType::ANIMAL,
		300, 5, wolf_items, NULL, INT2(0,0), INT2(0,0), NULL, G_ZWIERZETA,
		F_NIE_OTWIERA|F_BRAK_POTEZNEGO_ATAKU, NULL, DMG_SLASH, 1.5f, 6.f, 3.5f, BLOOD_RED,
		&sounds_wolf, &fi_wilk, NULL, idle_wilk, countof(idle_wilk), 0.4f, 1.f, F2_OMIJA_BLOK, F3_NIE_JE, ArmorUnitType::NONE,
	"worg", NULL, "wilk.qmsh", MAT_SKIN, INT2(4,8),
		StatProfileType::ANIMAL,
		300, 10, wolf_items, NULL, INT2(0,0), INT2(0,0), NULL, G_ZWIERZETA,
		F_NIE_OTWIERA|F_BRAK_POTEZNEGO_ATAKU, NULL, DMG_SLASH, 1.5f, 6.f, 3.5f, BLOOD_RED,
		&sounds_wolf, &fi_wilk, ti_worg, idle_wilk, countof(idle_wilk), 0.4f, 1.f, F2_OMIJA_BLOK, F3_NIE_JE, ArmorUnitType::NONE,
	//
	"spider", NULL, "pajak.qmsh", MAT_BODY, INT2(1,3),
		StatProfileType::ANIMAL,
		100, 5, NULL, NULL, INT2(0,0), INT2(0,0), NULL, G_ZWIERZETA,
		F_TRUJACY_ATAK|F_NIE_OTWIERA|F_ZA_LEKKI|F_ODPORNOSC_NA_TRUCIZNY|F_BRAK_POTEZNEGO_ATAKU, NULL, DMG_PIERCE, 1.f, 4.f, 3.5f, BLOOD_GREEN,
		&sounds_spider, &fi_spider, NULL, idle_spider, countof(idle_spider), 0.3f, 1.f, F2_OMIJA_BLOK, F3_NIE_JE, ArmorUnitType::NONE,
	"spider_big", NULL, "pajak2.qmsh", MAT_BODY, INT2(4,8),
		StatProfileType::ANIMAL,
		200, 10, NULL, NULL, INT2(0,0), INT2(0,0), NULL, G_ZWIERZETA,
		F_TRUJACY_ATAK|F_NIE_OTWIERA|F_ODPORNOSC_NA_TRUCIZNY|F_BRAK_POTEZNEGO_ATAKU, NULL, DMG_PIERCE, 1.f, 4.f, 3.5f, BLOOD_GREEN,
		&sounds_spider, &fi_spider, NULL, idle_spider, countof(idle_spider), 0.4f, 1.f, F2_OMIJA_BLOK, F3_NIE_JE, ArmorUnitType::NONE,
	//
	"rat", NULL, "szczur.qmsh", MAT_BODY, INT2(1,3),
		StatProfileType::ANIMAL,
		50, 0, NULL, NULL, INT2(0,0), INT2(0,0), NULL, G_ZWIERZETA,
		F_TCHORZLIWY|F_NIE_OTWIERA|F_ZA_LEKKI|F_BRAK_POTEZNEGO_ATAKU, NULL, DMG_PIERCE, 1.f, 5.f, 3.5f, BLOOD_RED,
		&sounds_rat, &fi_szczur, NULL, idle_szczur, countof(idle_szczur), 0.2f, 0.75f, F2_OMIJA_BLOK, F3_NIE_JE, ArmorUnitType::NONE,
	//
	"wild_hunter", NULL, NULL, MAT_BODY, INT2(5,13),
		StatProfileType::HUNTER,
		500, 5, hunter_items, NULL, INT2(4,10), INT2(40,80), NULL,
		G_ZWIERZETA, F_CZLOWIEK|F_LUCZNIK|F_DOWODCA, NULL, 0, 1.5f, 5.f, 3.f, BLOOD_RED,
		&sounds_def, &fi_human, ti_crazy_hunter, idle_czlowiek, countof(idle_czlowiek), 0.3f, 1.f, 0, 0, ArmorUnitType::HUMAN,
	"q_szaleni_szaleniec", NULL, NULL, MAT_BODY, INT2(5,13),
		StatProfileType::HUNTER,
		500, 5, szaleniec_items, NULL, INT2(4,10), INT2(40,80), dialog_q_szaleni,
		G_ZWIERZETA, F_CZLOWIEK|F_LUCZNIK|F_DOWODCA, NULL, 0, 1.5f, 5.f, 3.f, BLOOD_RED,
		&sounds_def, &fi_human, ti_crazy_hunter, idle_czlowiek, countof(idle_czlowiek), 0.3f, 1.f, F2_OZNACZ, 0, ArmorUnitType::HUMAN,

	//---- SZALE—CY ----
	"crazy_mage", NULL, NULL, MAT_BODY, INT2(0,25),
		StatProfileType::MAGE,
		510, 5, mage_items,&mage_spells, INT2(10,25), INT2(100,250), dialog_szalony, G_SZALENI,
		F_CZLOWIEK|F_MAG|F_SZALONY|F_BOHATER, NULL, 0, 1.55f, 5.1f, 3.1f, BLOOD_RED,
		&sounds_def, &fi_human, NULL, idle_czlowiek, countof(idle_czlowiek), 0.3f, 1.f, F2_ZAWODY, F3_ZAWODY_W_PICIU_25, ArmorUnitType::HUMAN,
	"crazy_warrior", NULL, NULL, MAT_BODY, INT2(0,25),
		StatProfileType::WARRIOR,
		510, 5, warrior_items, NULL, INT2(8,22), INT2(80,220), dialog_szalony, G_SZALENI,
		F_CZLOWIEK|F_SZALONY|F_BOHATER, NULL, 0, 1.55f, 5.1f, 3.1f, BLOOD_RED,
		&sounds_def, &fi_human, NULL, idle_czlowiek, countof(idle_czlowiek), 0.3f, 1.f, F2_AI_TRENUJE | F2_ZAWODY_W_PICIU | F2_WALKA_WRECZ | F2_ZAWODY, 0, ArmorUnitType::HUMAN,
	"crazy_hunter", NULL, NULL, MAT_BODY, INT2(0,25),
		StatProfileType::HUNTER,
		510, 5, hunter_items, NULL, INT2(6,18), INT2(60,180), dialog_szalony, G_SZALENI,
		F_CZLOWIEK|F_SZALONY|F_LUCZNIK|F_BOHATER, NULL, 0, 1.55f, 5.1f, 3.1f, BLOOD_RED,
		&sounds_def, &fi_human, NULL, idle_czlowiek, countof(idle_czlowiek), 0.3f, 1.f, F2_AI_TRENUJE | F2_ZAWODY_W_PICIU_50 | F2_ZAWODY, 0, ArmorUnitType::HUMAN,
	"crazy_rogue", NULL, NULL, MAT_BODY, INT2(0,25),
		StatProfileType::ROGUE,
		510, 5, rogue_items, NULL, INT2(7,20), INT2(70,200), dialog_szalony, G_SZALENI,
		F_CZLOWIEK|F_SZALONY|F_BOHATER, NULL, 0, 1.55f, 5.1f, 3.1f, BLOOD_RED,
		&sounds_def, &fi_human, NULL, idle_czlowiek, countof(idle_czlowiek), 0.3f, 1.f, F2_AI_TRENUJE | F2_ZAWODY_W_PICIU_50 | F2_WALKA_WRECZ_50 | F2_ZAWODY | F2_CIOS_W_PLECY, 0, ArmorUnitType::HUMAN,

	//---- HEROSI ----
	"hero_mage", NULL, NULL, MAT_BODY, INT2(0,25),
		StatProfileType::MAGE,
		500, 0, mage_items, &mage_spells, INT2(10,25), INT2(100,250), dialog_hero, G_MIESZKANCY,
		F_CZLOWIEK|F_MAG|F_BOHATER|F_AI_CHODZI, NULL, 0, 1.5f, 5.f, 3.f, BLOOD_RED,
		&sounds_def, &fi_human, NULL, idle_czlowiek, countof(idle_czlowiek), 0.3f, 1.f, F2_ZAWODY, F3_ZAWODY_W_PICIU_25, ArmorUnitType::HUMAN,
	"hero_warrior", NULL, NULL, MAT_BODY, INT2(0,25),
		StatProfileType::WARRIOR,
		500, 0, warrior_items, NULL, INT2(8,22), INT2(80,220), dialog_hero, G_MIESZKANCY,
		F_CZLOWIEK|F_BOHATER|F_AI_CHODZI, NULL, 0, 1.5f, 5.f, 3.f, BLOOD_RED,
		&sounds_def, &fi_human, NULL, idle_czlowiek, countof(idle_czlowiek), 0.3f, 1.f, F2_AI_TRENUJE | F2_ZAWODY_W_PICIU | F2_WALKA_WRECZ | F2_ZAWODY, 0, ArmorUnitType::HUMAN,
	"hero_hunter", NULL, NULL, MAT_BODY, INT2(0,25),
		StatProfileType::HUNTER,
		500, 0, hunter_items, NULL, INT2(6,18), INT2(60,180), dialog_hero, G_MIESZKANCY,
		F_CZLOWIEK|F_LUCZNIK|F_BOHATER|F_AI_CHODZI, NULL, 0, 1.5f, 5.f, 3.f, BLOOD_RED,
		&sounds_def, &fi_human, NULL, idle_czlowiek, countof(idle_czlowiek), 0.3f, 1.f, F2_AI_TRENUJE | F2_ZAWODY_W_PICIU_50 | F2_ZAWODY, 0, ArmorUnitType::HUMAN,
	"hero_rogue", NULL, NULL, MAT_BODY, INT2(0,25),
		StatProfileType::ROGUE,
		500, 0, rogue_items, NULL, INT2(7,20), INT2(70,200), dialog_hero, G_MIESZKANCY,
		F_CZLOWIEK|F_BOHATER|F_AI_CHODZI, NULL, 0, 1.5f, 5.f, 3.f, BLOOD_RED,
		&sounds_def, &fi_human, NULL, idle_czlowiek, countof(idle_czlowiek), 0.3f, 1.f, F2_AI_TRENUJE | F2_ZAWODY_W_PICIU_50 | F2_WALKA_WRECZ_50 | F2_ZAWODY | F2_CIOS_W_PLECY, 0, ArmorUnitType::HUMAN,

	//---- SPECJALNE POSTACIE ----	
	"tomashu", NULL, NULL, MAT_BODY, INT2(25),
		StatProfileType::TOMASHU,
		750, 50, tomash_items, &tomash_spells, INT2(100), INT2(100), dialog_tomashu, G_MIESZKANCY,
		F_CZLOWIEK|F_NIE_UCIEKA|F_TOMASH|F_DOWODCA|F_SEKRETNA|F_NIESMIERTELNY, NULL, 0, 1.f, 5.f, 3.f, BLOOD_RED,
		&sounds_def, &fi_human, NULL, idle_czlowiek, countof(idle_czlowiek), 0.3f, 1.f, F2_UZYWA_TRONU | F2_WALKA_WRECZ | F2_ODPORNOSC_NA_MAGIE_50 | F2_ODPORNOSC_NA_CIOS_W_PLECY, F3_NIE_JE, ArmorUnitType::HUMAN,

	"unk", NULL, "unk.qmsh", MAT_ROCK, INT2(13),
		StatProfileType::UNK,
		700, 50, NULL, NULL, INT2(0), INT2(0), NULL, G_NIEUMARLI,
		F_NIE_UCIEKA|F_BRAK_POTEZNEGO_ATAKU|F_NIE_CIERPI|F_ODPORNOSC_NA_TRUCIZNY|F_KLUTE25, NULL, DMG_SLASH, 0.5f, 10.f, 8.f, BLOOD_BLACK,
		&sounds_nieznany, &fi_nieznany, NULL, idle_nieznany, countof(idle_nieznany), 0.4f, 1.5f, F2_BRAK_KRWII | F2_OMIJA_BLOK | F2_ODPORNOSC_NA_MAGIE_25 | F2_CIOS_W_PLECY, F3_NIE_JE, ArmorUnitType::NONE,
};
const uint n_base_units = countof(g_base_units);

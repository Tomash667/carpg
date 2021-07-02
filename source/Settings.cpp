#include "Pch.h"
#include "Settings.h"

#include "GameKeys.h"
#include "Language.h"

#include <Config.h>

//=================================================================================================
void Settings::InitGameKeys()
{
	GKey[GK_MOVE_FORWARD].id = "keyMoveForward";
	GKey[GK_MOVE_BACK].id = "keyMoveBack";
	GKey[GK_MOVE_LEFT].id = "keyMoveLeft";
	GKey[GK_MOVE_RIGHT].id = "keyMoveRight";
	GKey[GK_ROTATE_LEFT].id = "keyRotateLeft";
	GKey[GK_ROTATE_RIGHT].id = "keyRotateRight";
	GKey[GK_TAKE_WEAPON].id = "keyTakeWeapon";
	GKey[GK_USE].id = "keyUse";
	GKey[GK_ATTACK_USE].id = "keyAttackUse";
	GKey[GK_SECONDARY_ATTACK].id = "keySecondaryAttack";
	GKey[GK_CANCEL_ATTACK].id = "keyCancelAttack";
	GKey[GK_BLOCK].id = "keyBlock";
	GKey[GK_WALK].id = "keyWalk";
	GKey[GK_TOGGLE_WALK].id = "keyToggleWalk";
	GKey[GK_AUTOWALK].id = "keyAutowalk";
	GKey[GK_STATS].id = "keyStats";
	GKey[GK_INVENTORY].id = "keyInventory";
	GKey[GK_TEAM_PANEL].id = "keyTeam";
	GKey[GK_ABILITY_PANEL].id = "keyAbility";
	GKey[GK_JOURNAL].id = "keyGameJournal";
	GKey[GK_MINIMAP].id = "keyMinimap";
	GKey[GK_TALK_BOX].id = "keyTalkBox";
	GKey[GK_QUICKSAVE].id = "keyQuicksave";
	GKey[GK_QUICKLOAD].id = "keyQuickload";
	GKey[GK_TAKE_ALL].id = "keyTakeAll";
	GKey[GK_MAP_SEARCH].id = "keyMapSearch";
	GKey[GK_SELECT_DIALOG].id = "keySelectDialog";
	GKey[GK_SKIP_DIALOG].id = "keySkipDialog";
	GKey[GK_PAUSE].id = "keyPause";
	GKey[GK_YELL].id = "keyYell";
	GKey[GK_ROTATE_CAMERA].id = "keyRotateCamera";
	GKey[GK_SHORTCUT1].id = "keyShortcut1";
	GKey[GK_SHORTCUT2].id = "keyShortcut2";
	GKey[GK_SHORTCUT3].id = "keyShortcut3";
	GKey[GK_SHORTCUT4].id = "keyShortcut4";
	GKey[GK_SHORTCUT5].id = "keyShortcut5";
	GKey[GK_SHORTCUT6].id = "keyShortcut6";
	GKey[GK_SHORTCUT7].id = "keyShortcut7";
	GKey[GK_SHORTCUT8].id = "keyShortcut8";
	GKey[GK_SHORTCUT9].id = "keyShortcut9";
	GKey[GK_SHORTCUT10].id = "keyShortcut10";
	GKey[GK_CONSOLE].id = "keyConsole";

	Language::Section s = Language::GetSection("GameKeys");
	for(int i = 0; i < GK_MAX; ++i)
		GKey[i].text = s.Get(GKey[i].id);
}

//=================================================================================================
void Settings::ResetGameKeys()
{
	GKey[GK_MOVE_FORWARD].Set(Key::W);
	GKey[GK_MOVE_BACK].Set(Key::S);
	GKey[GK_MOVE_LEFT].Set(Key::A);
	GKey[GK_MOVE_RIGHT].Set(Key::D);
	GKey[GK_ROTATE_LEFT].Set();
	GKey[GK_ROTATE_RIGHT].Set();
	GKey[GK_TAKE_WEAPON].Set(Key::Spacebar);
	GKey[GK_USE].Set(Key::E);
	GKey[GK_ATTACK_USE].Set(Key::LeftButton);
	GKey[GK_SECONDARY_ATTACK].Set(Key::Z, Key::X1Button);
	GKey[GK_CANCEL_ATTACK].Set(Key::X);
	GKey[GK_BLOCK].Set(Key::RightButton);
	GKey[GK_WALK].Set(Key::Shift);
	GKey[GK_TOGGLE_WALK].Set(Key::CapsLock);
	GKey[GK_AUTOWALK].Set(Key::F);
	GKey[GK_STATS].Set(Key::C);
	GKey[GK_INVENTORY].Set(Key::I);
	GKey[GK_TEAM_PANEL].Set(Key::T);
	GKey[GK_ABILITY_PANEL].Set(Key::K);
	GKey[GK_JOURNAL].Set(Key::J);
	GKey[GK_MINIMAP].Set(Key::Tab);
	GKey[GK_TALK_BOX].Set(Key::Quote);
	GKey[GK_QUICKSAVE].Set(Key::F5);
	GKey[GK_QUICKLOAD].Set(Key::F9);
	GKey[GK_TAKE_ALL].Set(Key::F);
	GKey[GK_MAP_SEARCH].Set(Key::F);
	GKey[GK_SELECT_DIALOG].Set(Key::Enter);
	GKey[GK_SKIP_DIALOG].Set(Key::Spacebar);
	GKey[GK_PAUSE].Set(Key::Pause);
	GKey[GK_YELL].Set(Key::Y);
	GKey[GK_ROTATE_CAMERA].Set(Key::V);
	GKey[GK_SHORTCUT1].Set(Key::N1);
	GKey[GK_SHORTCUT2].Set(Key::N2);
	GKey[GK_SHORTCUT3].Set(Key::N3);
	GKey[GK_SHORTCUT4].Set(Key::N4);
	GKey[GK_SHORTCUT5].Set(Key::N5);
	GKey[GK_SHORTCUT6].Set(Key::N6);
	GKey[GK_SHORTCUT7].Set(Key::N7);
	GKey[GK_SHORTCUT8].Set(Key::N8);
	GKey[GK_SHORTCUT9].Set(Key::N9);
	GKey[GK_SHORTCUT10].Set(Key::N0);
	GKey[GK_CONSOLE].Set(Key::Tilde);
}

//=================================================================================================
void Settings::SaveGameKeys(Config& cfg)
{
	for(int i = 0; i < GK_MAX; ++i)
	{
		GameKey& k = GKey[i];
		for(int j = 0; j < 2; ++j)
			cfg.Add(Format("%s%d", k.id, j), (int)k[j]);
	}
}

//=================================================================================================
void Settings::LoadGameKeys(Config& cfg)
{
	// pre V_0_13 compatibility
	cfg.Rename("keyActions0", "keyAbility0");
	cfg.Rename("keyActions1", "keyAbility1");

	for(int i = 0; i < GK_MAX; ++i)
	{
		GameKey& k = GKey[i];
		for(int j = 0; j < 2; ++j)
		{
			cstring s = Format("%s%d", k.id, j);
			int w = cfg.GetInt(s, -1);
			if(w == (int)Key::Escape || w < -1 || w > 255)
			{
				Warn("Config: Invalid value for %s: %d.", s, w);
				w = -1;
				cfg.Add(s, (int)k[j]);
			}
			if(w != -1)
				k[j] = (Key)w;
		}
	}
}

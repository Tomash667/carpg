#include "Pch.h"
#include "GameCore.h"
#include "Settings.h"
#include "GameKeys.h"
#include "Language.h"
#include "Config.h"

//=================================================================================================
void Settings::InitGameKeys()
{
	GKey[GK_MOVE_FORWARD].id = "keyMoveForward";
	GKey[GK_MOVE_BACK].id = "keyMoveBack";
	GKey[GK_MOVE_LEFT].id = "keyMoveLeft";
	GKey[GK_MOVE_RIGHT].id = "keyMoveRight";
	GKey[GK_WALK].id = "keyWalk";
	GKey[GK_ROTATE_LEFT].id = "keyRotateLeft";
	GKey[GK_ROTATE_RIGHT].id = "keyRotateRight";
	GKey[GK_TAKE_WEAPON].id = "keyTakeWeapon";
	GKey[GK_ATTACK_USE].id = "keyAttackUse";
	GKey[GK_USE].id = "keyUse";
	GKey[GK_BLOCK].id = "keyBlock";
	GKey[GK_STATS].id = "keyStats";
	GKey[GK_INVENTORY].id = "keyInventory";
	GKey[GK_TEAM_PANEL].id = "keyTeam";
	GKey[GK_ACTION_PANEL].id = "keyActions";
	GKey[GK_JOURNAL].id = "keyGameJournal";
	GKey[GK_MINIMAP].id = "keyMinimap";
	GKey[GK_QUICKSAVE].id = "keyQuicksave";
	GKey[GK_QUICKLOAD].id = "keyQuickload";
	GKey[GK_TAKE_ALL].id = "keyTakeAll";
	GKey[GK_SELECT_DIALOG].id = "keySelectDialog";
	GKey[GK_SKIP_DIALOG].id = "keySkipDialog";
	GKey[GK_TALK_BOX].id = "keyTalkBox";
	GKey[GK_PAUSE].id = "keyPause";
	GKey[GK_YELL].id = "keyYell";
	GKey[GK_CONSOLE].id = "keyConsole";
	GKey[GK_ROTATE_CAMERA].id = "keyRotateCamera";
	GKey[GK_AUTOWALK].id = "keyAutowalk";
	GKey[GK_TOGGLE_WALK].id = "keyToggleWalk";
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

	Language::Section& s = Language::GetSection("GameKeys");
	for(int i = 0; i < GK_MAX; ++i)
		GKey[i].text = s.Get(GKey[i].id);
}

//=================================================================================================
void Settings::ResetGameKeys()
{
	GKey[GK_MOVE_FORWARD].Set('W', VK_UP);
	GKey[GK_MOVE_BACK].Set('S', VK_DOWN);
	GKey[GK_MOVE_LEFT].Set('A', VK_LEFT);
	GKey[GK_MOVE_RIGHT].Set('D', VK_RIGHT);
	GKey[GK_WALK].Set(VK_SHIFT);
	GKey[GK_ROTATE_LEFT].Set('Q');
	GKey[GK_ROTATE_RIGHT].Set('E');
	GKey[GK_TAKE_WEAPON].Set(VK_SPACE);
	GKey[GK_ATTACK_USE].Set(VK_LBUTTON, 'Z');
	GKey[GK_USE].Set('R');
	GKey[GK_BLOCK].Set(VK_RBUTTON, 'X');
	GKey[GK_STATS].Set('C');
	GKey[GK_INVENTORY].Set('I');
	GKey[GK_TEAM_PANEL].Set('T');
	GKey[GK_ACTION_PANEL].Set('K');
	GKey[GK_JOURNAL].Set('J');
	GKey[GK_MINIMAP].Set('M');
	GKey[GK_QUICKSAVE].Set(VK_F5);
	GKey[GK_QUICKLOAD].Set(VK_F9);
	GKey[GK_TAKE_ALL].Set('F');
	GKey[GK_SELECT_DIALOG].Set(VK_RETURN);
	GKey[GK_SKIP_DIALOG].Set(VK_SPACE);
	GKey[GK_TALK_BOX].Set('N');
	GKey[GK_PAUSE].Set(VK_PAUSE);
	GKey[GK_YELL].Set('Y');
	GKey[GK_CONSOLE].Set(VK_OEM_3);
	GKey[GK_ROTATE_CAMERA].Set('V');
	GKey[GK_AUTOWALK].Set('F');
	GKey[GK_TOGGLE_WALK].Set(VK_CAPITAL);
	GKey[GK_SHORTCUT1].Set('1');
	GKey[GK_SHORTCUT2].Set('2');
	GKey[GK_SHORTCUT3].Set('3');
	GKey[GK_SHORTCUT4].Set('4');
	GKey[GK_SHORTCUT5].Set('5');
	GKey[GK_SHORTCUT6].Set('6');
	GKey[GK_SHORTCUT7].Set('7');
	GKey[GK_SHORTCUT8].Set('8');
	GKey[GK_SHORTCUT9].Set('9');
	GKey[GK_SHORTCUT10].Set('0');
}

//=================================================================================================
void Settings::SaveGameKeys(Config& cfg)
{
	for(int i = 0; i < GK_MAX; ++i)
	{
		GameKey& k = GKey[i];
		for(int j = 0; j < 2; ++j)
			cfg.Add(Format("%s%d", k.id, j), k[j]);
	}
}

//=================================================================================================
void Settings::LoadGameKeys(Config& cfg)
{
	for(int i = 0; i < GK_MAX; ++i)
	{
		GameKey& k = GKey[i];
		for(int j = 0; j < 2; ++j)
		{
			cstring s = Format("%s%d", k.id, j);
			int w = cfg.GetInt(s);
			if(w == VK_ESCAPE || w < -1 || w > 255)
			{
				Warn("Config: Invalid value for %s: %d.", s, w);
				w = -1;
				cfg.Add(s, k[j]);
			}
			if(w != -1)
				k[j] = (byte)w;
		}
	}
}

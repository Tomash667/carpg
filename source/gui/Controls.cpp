#include "Pch.h"
#include "Controls.h"

#include "Game.h"
#include "GameGui.h"
#include "GameKeys.h"
#include "Language.h"

//-----------------------------------------------------------------------------
// 0x01 - pickable key
// 0x02 - translated
const int keyFlags[] = {
	0,	// 0:VK_NONE

	3,	// 1:VK_LBUTTON
	3,	// 2:VK_RBUTTON
	2,	// 3:VK_CANCEL
	3,	// 4:VK_MBUTTON
	3,	// 5:VK_XBUTTON1
	3,	// 6:VK_XBUTTON2,

	0,	// 7:unassigned

	3,	// 8:VK_BACK
	3,	// 9:VK_TAB

	0,	// 10:reserved
	0,	// 11:reserved

	3,	// 12:VK_CLEAR
	3,	// 13:VK_RETURN

	0,	// 14:undefined
	0,	// 15:undefined

	3,	// 16:VK_SHIFT
	3,	// 17:VK_CONTROL
	3,	// 18:VK_MENU
	3,	// 19:VK_PAUSE
	3,	// 20:VK_CAPITAL

	0,	// 21:VK_KANA
	0,	// 22:undefined
	0,	// 23:VK_JUNJA
	0,	// 24:VK_FINAL
	0,	// 25:VK_HANJA

	0,	// 26:undefined

	2,	// 27:VK_ESCAPE

	0,	// 28:VK_CONVERT
	0,	// 29:VK_NONCONVERT
	0,	// 30:VK_ACCEPT
	0,	// 31:VK_MODECHANGE

	3,	// 32:VK_SPACE
	3,	// 33:VK_PRIOR
	3,	// 34:VK_NEXT
	3,	// 35:VK_END
	3,	// 36:VK_HOME
	3,	// 37:VK_LEFT
	3,	// 38:VK_UP
	3,	// 39:VK_RIGHT
	3,	// 40:VK_DOWN
	0,	// 41:VK_SELECT
	0,	// 42:VK_PRINT
	0,	// 43:VK_EXECUTE
	2,	// 44:VK_SNAPSHOT
	3,	// 45:VK_INSERT
	3,	// 46:VK_DELETE
	0,	// 47:VK_HELP

	3,	// 48:0,
	3,	// 49:3,
	3,	// 50:2
	3,	// 51:3
	3,	// 52:4
	3,	// 53:5
	3,	// 54:6
	3,	// 55:7
	3,	// 56:8
	3,	// 57:9

	0,	// 58:---
	0,	// 59:---
	0,	// 60:---
	0,	// 61:---
	0,	// 62:---
	0,	// 63:---
	0,	// 64:---

	3,	// 65:A
	3,	// 66:B
	3,	// 67:C
	3,	// 68:D
	3,	// 69:E
	3,	// 70:F
	3,	// 71:G
	3,	// 72:H
	3,	// 73:I
	3,	// 74:J
	3,	// 75:K
	3,	// 76:L
	3,	// 77:M
	3,	// 78:N
	3,	// 79:O
	3,	// 80:P
	3,	// 81:Q
	3,	// 82:R
	3,	// 83:S
	3,	// 84:T
	3,	// 85:U
	3,	// 86:V
	3,	// 87:W
	3,	// 88:X
	3,	// 89:Y
	3,	// 90:Z

	0,	// 91:VK_LWIN
	0,	// 92:VK_RWIN
	0,	// 93:VK_APPS

	0,	// 94:reserved

	0,	// 95:VK_SLEEP

	3,	// 96:VK_NUMPAD0
	3,	// 97:VK_NUMPAD1
	3,	// 98:VK_NUMPAD2
	3,	// 99:VK_NUMPAD3
	3,	// 100:VK_NUMPAD4
	3,	// 101:VK_NUMPAD5
	3,	// 102:VK_NUMPAD6
	3,	// 103:VK_NUMPAD7
	3,	// 104:VK_NUMPAD8
	3,	// 105:VK_NUMPAD9
	3,	// 106:VK_MULTIPLY
	3,	// 107:VK_ADD
	0,	// 108:VK_SEPARATOR
	3,	// 109:VK_SUBTRACT
	3,	// 110:VK_DECIMAL
	3,	// 111:VK_DIVIDE

	3,	// 112:F1
	3,	// 113:F2
	3,	// 114:F3
	3,	// 115:F4
	3,	// 116:F5
	3,	// 117:F6
	3,	// 118:F7
	3,	// 119:F8
	3,	// 120:F9
	3,	// 121:F10
	3,	// 122:F11
	3,	// 123:F12
	3,	// 124:F13
	3,	// 125:F14
	3,	// 126:F15
	3,	// 127:F16
	3,	// 128:F17
	3,	// 129:F18
	3,	// 130:F19
	3,	// 131:F20
	3,	// 132:F21
	3,	// 133:F22
	3,	// 134:F23
	3,	// 135:F24

	0,	// 136:unassigned
	0,	// 137:unassigned
	0,	// 138:unassigned
	0,	// 139:unassigned
	0,	// 149:unassigned
	0,	// 141:unassigned
	0,	// 142:unassigned
	0,	// 143:unassigned

	3,	// 144:VK_NUMLOCK
	3,	// 145:VK_SCROLL

	0,	// 146:VK_OEM_NEC_EQUAL

	0,	// 147:---
	0,	// 148:---
	0,	// 149:---
	0,	// 150:---
	0,	// 151:---
	0,	// 152:---
	0,	// 153:---
	0,	// 154:---
	0,	// 155:---
	0,	// 156:---
	0,	// 157:---
	0,	// 158:---
	0,	// 159:---
	0,	// 160:---
	0,	// 161:---
	0,	// 162:---
	0,	// 163:---
	0,	// 164:---
	0,	// 165:---
	0,	// 166:---
	0,	// 167:---
	0,	// 168:---
	0,	// 169:---
	0,	// 170:---
	0,	// 171:---
	0,	// 172:---
	0,	// 173:---
	0,	// 174:---
	0,	// 175:---
	0,	// 176:---
	0,	// 177:---
	0,	// 178:---
	0,	// 179:---
	0,	// 180:---
	0,	// 181:---
	0,	// 182:---
	0,	// 183:---
	0,	// 184:---
	0,	// 185:---

	3,	// 186:VK_OEM_1
	3,	// 187:VK_OEM_PLUS
	3,	// 188:VK_OEM_COMMA
	3,	// 189:VK_OEM_MINUS
	3,	// 190:VK_OEM_PERIOD
	3,	// 191:VK_OEM_2
	3,	// 192:VK_OEM_3

	0,	// 193:---
	0,	// 194:---
	0,	// 195:---
	0,	// 196:---
	0,	// 197:---
	0,	// 198:---
	0,	// 199:---
	0,	// 200:---
	0,	// 201:---
	0,	// 202:---
	0,	// 203:---
	0,	// 204:---
	0,	// 205:---
	0,	// 206:---
	0,	// 207:---
	0,	// 208:---
	0,	// 209:---
	0,	// 210:---
	0,	// 211:---
	0,	// 212:---
	0,	// 213:---
	0,	// 214:---
	0,	// 215:---
	0,	// 216:---
	0,	// 217:---
	0,	// 218:---

	3,	// 219:VK_OEM_4
	3,	// 220:VK_OEM_5
	3,	// 221:VK_OEM_6
	3	// 222:VK_OEM_7
};
const int keyFlagsCount = countof(keyFlags);

//-----------------------------------------------------------------------------
enum ButtonId
{
	Button_Ok = GuiEvent_Custom,
	Button_Reset
};

//=================================================================================================
Controls::Controls(const DialogInfo& info) : DialogBox(info), pickedKey(-1)
{
	size = Int2(680, 450);
	bts.resize(2);

	bts[0].size = Int2(180, 44);
	bts[0].pos = Int2(50, size.y - 64);
	bts[0].id = Button_Reset;
	bts[0].parent = this;

	bts[1].size = Int2(180, 44);
	bts[1].pos = Int2(size.x - 180 - 50, size.y - 64);
	bts[1].id = Button_Ok;
	bts[1].parent = this;

	grid.size = Int2(640, 300);
	grid.pos = Int2(20, 60);
	grid.items = GK_MAX;
	grid.event = GridEvent(this, &Controls::GetCell);
	grid.selectEvent = SelectGridEvent(this, &Controls::SelectCell);
	grid.allowSelect = false;
	grid.singleLine = true;
}

//=================================================================================================
void Controls::LoadLanguage()
{
	Language::Section s = Language::GetSection("Controls");

	bts[0].text = s.Get("resetKeys");
	bts[1].text = gui->txOk;

	grid.AddColumn(Grid::TEXT, 220, s.Get("action"));
	grid.AddColumn(Grid::TEXT, 200, s.Get("key1"));
	grid.AddColumn(Grid::TEXT, 200, s.Get("key2"));
	grid.Init();

	txResetConfirm = s.Get("resetConfirm");
	txInfo = s.Get("info");
	txTitle = s.Get("title");

	InitKeyText();
}

//=================================================================================================
void Controls::Draw()
{
	DrawPanel();

	// title
	Rect r = { globalPos.x, globalPos.y + 8, globalPos.x + size.x, globalPos.y + size.y };
	gui->DrawText(GameGui::fontBig, txTitle, DTF_TOP | DTF_CENTER, Color::Black, r);

	for(Button& button : bts)
		button.Draw();

	grid.Draw();

	r = { globalPos.x, globalPos.y + size.y - 85, globalPos.x + size.x, globalPos.y + size.y };
	gui->DrawText(GameGui::font, txInfo, DTF_CENTER, Color::Black, r);
}

//=================================================================================================
void Controls::Update(float dt)
{
	cursorTick += dt * 2;
	if(cursorTick >= 1.f)
		cursorTick = 0.f;

	if(pickedKey == -1)
	{
		for(Button& button : bts)
		{
			button.mouseFocus = focus;
			button.Update(dt);
		}

		grid.focus = focus;
		grid.mouseFocus = focus;
		grid.Update(dt);

		if(focus && input->Focus() && input->PressedRelease(Key::Escape))
			Event((GuiEvent)Button_Ok);
	}
	else
	{
		grid.focus = false;
		grid.Update(dt);
	}
}

//=================================================================================================
void Controls::Event(GuiEvent e)
{
	if(e == GuiEvent_Show || e == GuiEvent_WindowResize)
	{
		if(e == GuiEvent_Show)
		{
			visible = true;
			changed = false;
		}
		pos = globalPos = (gui->wndSize - size) / 2;
		for(int i = 0; i < 2; ++i)
			bts[i].globalPos = globalPos + bts[i].pos;
		grid.Move(globalPos);
	}
	else if(e == GuiEvent_LostFocus)
		grid.LostFocus();
	else if(e == GuiEvent_Close)
	{
		grid.LostFocus();
		visible = false;
	}
	else if(e == Button_Ok)
	{
		if(changed)
		{
			gameGui->ChangeControls();
			game->settings.SaveGameKeys(game->cfg);
			game->SaveCfg();
		}
		CloseDialog();
	}
	else if(e == Button_Reset)
	{
		DialogInfo info;
		info.event = [this](int r)
		{
			if(r == BUTTON_YES)
			{
				game->settings.ResetGameKeys();
				changed = true;
			}
		};
		info.name = "reset_ctrls";
		info.order = DialogOrder::Top;
		info.parent = this;
		info.pause = false;
		info.text = txResetConfirm;
		info.type = DIALOG_YESNO;
		gui->ShowDialog(info);
	}
}

//=================================================================================================
void Controls::GetCell(int item, int column, Cell& cell)
{
	GameKey& k = GKey[item];
	if(column == 0)
	{
		if(k.text)
			cell.text = k.text;
		else
			cell.text = "(missing)";
	}
	else
	{
		if(item == pickedKey && column - 1 == pickedIndex)
		{
			if(cursorTick <= 0.5f)
				cell.text = "|";
			else
				cell.text = "";
		}
		else
		{
			int n = (int)k[column - 1];
			if(n > keyFlagsCount)
				cell.text = "";
			else
			{
				cell.text = txKey[n];
				if(!cell.text)
					cell.text = "";
			}
		}
	}
}

//=================================================================================================
void Controls::InitKeyText()
{
	Language::Section s = Language::GetSection("Keys");
	for(int i = 0; i < keyFlagsCount; ++i)
	{
		if(IsSet(keyFlags[i], 0x02))
			txKey[i] = s.Get(Format("k%d", i));
		else
			txKey[i] = nullptr;
	}

	game->settings.InitGameKeys();
}

//=================================================================================================
void Controls::SelectCell(int item, int column, int button)
{
	if(button == 0)
	{
		input->SetState(Key::LeftButton, Input::IS_UP);
		pickedKey = item;
		pickedIndex = column - 1;
		cursorTick = 0.f;
		input->SetCallback(Input::Callback(this, &Controls::OnKey));
		gameGui->cursorAllowMove = false;
	}
	else
	{
		Key& key = GKey[item][column - 1];
		if(key != Key::None)
		{
			key = Key::None;
			changed = true;
		}
	}
}

//=================================================================================================
void Controls::OnKey(Key key)
{
	if(key == Key::Escape)
	{
		pickedKey = -1;
		input->SetCallback(nullptr);
		gameGui->cursorAllowMove = true;
	}
	else if((int)key < keyFlagsCount && IsSet(keyFlags[(int)key], 0x01))
	{
		GKey[pickedKey][pickedIndex] = key;
		pickedKey = -1;
		input->SetCallback(nullptr);
		gameGui->cursorAllowMove = true;
		changed = true;
	}
}

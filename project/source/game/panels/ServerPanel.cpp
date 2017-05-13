#include "Pch.h"
#include "Base.h"
#include "ServerPanel.h"
#include "Game.h"
#include "Language.h"
#include "InfoBox.h"
#include "BitStreamFunc.h"

//-----------------------------------------------------------------------------
enum ButtonId
{
	IdPickCharacter = GuiEvent_Custom,
	IdReady,
	IdKick,
	IdLeader,
	IdStart,
	IdCancel
};

//=================================================================================================
ServerPanel::ServerPanel(const DialogInfo& info) : Dialog(info)
{
	size = INT2(540,510);
	bts.resize(6);

	txReady = Str("ready");
	txNotReady = Str("notReady");
	txStart = Str("start");
	txStop = Str("stop");
	txStarting = Str("starting");
	txPickChar = Str("pickChar");
	txKick = Str("kick");
	txNone = Str("none");
	txSetLeader = Str("setLeader");
	txNick = Str("nick2");
	txChar = Str("char");
	txLoadedCharInfo = Str("loadedCharInfo");
	txNotLoadedCharInfo = Str("notLoadedCharInfo");
	txChangeChar = Str("changeChar");
	txCantKickMyself = Str("cantKickMyself2");
	txCantKickUnconnected = Str("cantKickUnconnected");
	txReallyKick = Str("reallyKick2");
	txAlreadyLeader = Str("alreadyLeader2");
	txLeaderChanged = Str("leaderChanged");
	txNotJoinedYet = Str("notJoinedYet");
	txNotAllReady = Str("notAllReady");
	txStartingIn = Str("startingIn");
	txStartingStop = Str("startingStop");
	txDisconnecting = Str("disconnecting");
	txYouAreLeader = Str("youAreLeader2");
	txJoined = Str("joined");
	txPlayerLeft = Str("playerLeft");
	txNeedSelectedPlayer = Str("needSelectedPlayer");
	txServerText = Str("serverText");

	const INT2 bt_size(180,44);

	bts[0].id = IdPickCharacter;
	bts[0].parent = this;
	bts[0].text = txPickChar; // Zmieñ postaæ
	bts[0].pos = INT2(340,30);
	bts[0].size = bt_size;

	bts[1].id = IdReady;
	bts[1].parent = this;
	bts[1].text = txReady; // Niegotowy
	bts[1].pos = INT2(340,80);
	bts[1].size = bt_size;

	bts[2].id = IdKick;
	bts[2].parent = this;
	bts[2].text = txKick; // Anuluj
	bts[2].pos = INT2(340,130);
	bts[2].size = bt_size;

	bts[3].id = IdLeader;
	bts[3].parent = this;
	bts[3].text = txSetLeader;
	bts[3].pos = INT2(340,180);
	bts[3].size = bt_size;

	bts[4].id = IdStart;
	bts[4].parent = this;
	bts[4].text = txStart;
	bts[4].pos = INT2(340,230);
	bts[4].size = bt_size;

	bts[5].id = IdCancel;
	bts[5].parent = this;
	bts[5].text = GUI.txCancel;
	bts[5].pos = INT2(340,280);
	bts[5].size = bt_size;

	grid.pos = INT2(10,10);
	grid.size = INT2(320,300);
	grid.event = GridEvent(this, &ServerPanel::GetCell);
	grid.selection_type = Grid::BACKGROUND;
	grid.selection_color = COLOR_RGBA(0,255,0,128);
	grid.AddColumn(Grid::IMG, 20);
	grid.AddColumn(Grid::TEXT_COLOR, 140, txNick);
	grid.AddColumn(Grid::TEXT, 140, txChar);
	grid.Init();

	itb.parent = this;
	itb.max_cache = 10;
	itb.max_lines = 100;
	itb.esc_clear = false;
	itb.lose_focus = false;
	itb.pos = INT2(10,320);
	itb.size = INT2(320,182);
	itb.event = InputEvent(this, &ServerPanel::OnInput);
	itb.Init();

	visible = false;
}

//=================================================================================================
void ServerPanel::Draw(ControlDrawData*)
{
	// t³o
	GUI.DrawSpriteFull(tBackground, COLOR_RGBA(255,255,255,128));

	// panel
	GUI.DrawItem(tDialog, global_pos, size, COLOR_RGBA(255,255,255,222), 16);

	// przyciski
	int ile = (game->sv_server ? 6 : 3);
	for(int i=0; i<ile; ++i)
		bts[i].Draw();

	// input
	itb.Draw();

	// grid
	grid.Draw();

	// tekst
	RECT r = {340+global_pos.x, 355+global_pos.y, 340+185+global_pos.x, 355+160+global_pos.y};
	GUI.DrawText(GUI.default_font, Format(txServerText, game->server_name2.c_str(), game->players, game->max_players2, game->enter_pswd.empty() ? GUI.txNo : GUI.txYes), 0, BLACK, r, &r);
}

//=================================================================================================
void ServerPanel::Update(float dt)
{
	// przyciski
	int ile = (game->sv_server ? 6 : 3);
	for(int i=0; i<ile; ++i)
	{
		bts[i].mouse_focus = focus;
		bts[i].Update(dt);
	}

	// textbox
	itb.mouse_focus = focus;
	itb.Update(dt);

	// grid
	grid.focus = focus;
	grid.Update(dt);

	if(focus && Key.Focus() && Key.PressedRelease(VK_ESCAPE))
		Event(GuiEvent(game->sv_server ? IdCancel : IdKick));

	game->UpdateLobbyNet(dt);
}

//=================================================================================================
void ServerPanel::Event(GuiEvent e)
{
	if(e == GuiEvent_Show || e == GuiEvent_WindowResize)
	{
		if(e == GuiEvent_Show)
		{
			visible = true;
			itb.focus = true;
			itb.Event(GuiEvent_GainFocus);
		}
		global_pos = (GUI.wnd_size - size)/2;
		for(int i=0; i<6; ++i)
			bts[i].global_pos = global_pos + bts[i].pos;
		itb.Event(GuiEvent_Moved);
		grid.Move(global_pos);
	}
	else if(e == GuiEvent_GainFocus)
	{
		itb.focus = true;
		itb.Event(GuiEvent_GainFocus);
	}
	else if(e == GuiEvent_LostFocus)
	{
		grid.LostFocus();
		itb.focus = false;
		itb.Event(GuiEvent_LostFocus);
	}
	else if(e == GuiEvent_Close)
	{
		visible = false;
		grid.LostFocus();
		itb.focus = false;
		itb.Event(GuiEvent_LostFocus);
	}
	else if(e >= GuiEvent_Custom)
	{
		switch(e)
		{
		case IdPickCharacter: // pick character / change character
			{
				PlayerInfo& info = game->game_players[0];
				if(info.clas != Class::INVALID)
				{
					// already have character, redo
					if(info.ready)
					{
						// uncheck ready
						info.ready = false;
						game->ChangeReady();
					}
					game->ShowCreateCharacterPanel(false, true);
				}
				else
					game->ShowCreateCharacterPanel(false);
			}
			break;
		case IdReady: // ready / unready
			{
				PlayerInfo& info = game->game_players[0];
				info.ready = !info.ready;
				game->ChangeReady();
			}
			break;
		case IdKick: // kick / cancel
			if(game->sv_server)
			{
				if(grid.selected == -1)
					AddMsg(txNeedSelectedPlayer);
				else if(grid.selected == 0)
					AddMsg(txCantKickMyself);
				else
				{
					PlayerInfo& info = game->game_players[grid.selected];
					if(info.state != PlayerInfo::IN_LOBBY)
						AddMsg(txCantKickUnconnected);
					else
					{
						// na pewno?
						game->kick_id = info.id;
						DialogInfo di;
						di.event = DialogEvent(this, &ServerPanel::OnKick);
						di.name = "kick";
						di.order = ORDER_TOP;
						di.parent = this;
						di.pause = false;
						di.text = Format(txReallyKick, info.name.c_str());
						di.type = DIALOG_YESNO;
						GUI.ShowDialog(di);
					}
					return;
				}
			}
			else
				ExitLobby();
			break;
		case IdLeader: // change leader
			if(grid.selected == -1)
				AddMsg(txNeedSelectedPlayer);
			else
			{
				PlayerInfo& info = game->game_players[grid.selected];
				if(info.id == game->leader_id)
					AddMsg(txAlreadyLeader);
				else if(info.state == PlayerInfo::IN_LOBBY)
				{
					game->leader_id = info.id;
					game->AddLobbyUpdate(INT2(Lobby_ChangeLeader,0));
					AddMsg(Format(txLeaderChanged, info.name.c_str()));
				}
				else
					AddMsg(txNotJoinedYet);
			}
			break;
		case IdStart: // start game / stop
			if(!game->sv_startup)
			{
				cstring error_text = nullptr;

				for(vector<PlayerInfo>::iterator it = game->game_players.begin(), end = game->game_players.end(); it != end; ++it)
				{
					if(!it->ready)
					{
						error_text = txNotAllReady;
						break;
					}
				}

				if(!error_text)
				{
					// rozpocznij odliczanie do startu
					game->sv_startup = true;
					game->startup_timer = float(STARTUP_TIMER);
					game->last_startup_id = STARTUP_TIMER;
					byte b[] = {ID_STARTUP, STARTUP_TIMER};
					game->peer->Send((cstring)b, 2, IMMEDIATE_PRIORITY, RELIABLE, 0, UNASSIGNED_SYSTEM_ADDRESS, true);
					bts[4].text = txStop;
					cstring s = Format(txStartingIn, STARTUP_TIMER);
					AddMsg(s);
					LOG(s);
				}

				if(error_text)
					AddMsg(error_text);
			}
			else
				StopStartup();
			break;
		case IdCancel: // cancel
			ExitLobby();
			break;
		}
	}
}

//=================================================================================================
void ServerPanel::Show()
{
	bts[0].text = txPickChar;
	bts[0].state = Button::NONE;
	bts[1].text = txReady;
	bts[1].state = Button::DISABLED;

	if(game->sv_server)
	{		
		bts[2].text = txKick;
		bts[4].state = Button::DISABLED;
		bts[4].text = txStart;
	}
	else
		bts[2].text = GUI.txCancel;

	itb.Reset();
	grid.Reset();

	GUI.ShowDialog(this);
}

//=================================================================================================
void ServerPanel::GetCell(int item, int column, Cell& cell)
{
	PlayerInfo& info = game->game_players[item];

	if(column == 0)
		cell.img = (info.ready ? game->tGotowy : game->tNieGotowy);
	else if(column == 1)
	{
		cell.text_color->text = (info.state == PlayerInfo::IN_LOBBY ? info.name.c_str() : info.adr.ToString());
		cell.text_color->color = (info.id == game->leader_id ? 0xFFFFD700 : BLACK);
	}
	else
		cell.text = (info.clas == Class::INVALID ? txNone : g_classes[(int)info.clas].name.c_str());
}

//=================================================================================================
void ServerPanel::ExitLobby(VoidF f)
{
	LOG("ServerPanel: Exiting lobby.");

	if(game->sv_server)
	{
		if(game->mp_load)
			game->ClearGame();

		LOG("ServerPanel: Closing server.");

		// zablokuj do³¹czanie
		game->peer->SetMaximumIncomingConnections(0);
		// wy³¹cz info o serwerze
		game->peer->SetOfflinePingResponse(nullptr, 0);

		if(game->players > 1)
		{
			// roz³¹cz graczy
			LOG("ServerPanel: Disconnecting clients.");
			const byte b[] = {ID_SERVER_CLOSE, 0};
			game->peer->Send((cstring)b, 2, IMMEDIATE_PRIORITY, RELIABLE, 0, UNASSIGNED_SYSTEM_ADDRESS, true);
			game->net_mode = Game::NM_QUITTING_SERVER;
			--game->players;
			game->net_timer = T_WAIT_FOR_DISCONNECT;
			game->info_box->Show(txDisconnecting);
			game->net_callback = f;
		}
		else
		{
			// nie ma graczy, mo¿na zamkn¹æ
			game->ClosePeer();
			CloseDialog();
			if(f)
				f();
		}
	}
	else
	{
		const byte b = ID_LEAVE;
		game->peer->Send((cstring)&b, 1, IMMEDIATE_PRIORITY, RELIABLE, 0, game->server, false);
		game->info_box->Show(txDisconnecting);
		game->net_mode = Game::NM_QUITTING;
		game->net_timer = T_WAIT_FOR_DISCONNECT;
		game->net_callback = f;
		CloseDialog();
	}
}

//=================================================================================================
void ServerPanel::AddMsg(cstring _text)
{
	itb.Add(_text);
}

//=================================================================================================
void ServerPanel::OnKick(int id)
{
	if(id == BUTTON_YES)
	{
		int index = game->GetPlayerIndex(game->kick_id);
		if(index != -1)
			game->KickPlayer(index);
	}
}

//=================================================================================================
void ServerPanel::OnInput(const string& str)
{
	if(str[0] == '/')
		game->ParseCommand(str.substr(1), PrintMsgFunc(this, &ServerPanel::AddMsg), PS_LOBBY);
	else
	{
		// wyœlij tekst
		if(game->players != 1)
		{
			game->net_stream.Reset();
			game->net_stream.Write(ID_SAY);
			game->net_stream.WriteCasted<byte>(game->my_id);
			WriteString1(game->net_stream, str);
			game->peer->Send(&game->net_stream, MEDIUM_PRIORITY, RELIABLE, 0, game->sv_server ? UNASSIGNED_SYSTEM_ADDRESS : game->server, game->sv_server);
		}
		cstring s = Format("%s: %s", game->player_name.c_str(), str.c_str());
		AddMsg(s);
		LOG(s);
	}
}

//=================================================================================================
void ServerPanel::StopStartup()
{
	LOG("Startup canceled.");
	AddMsg(txStartingStop);
	game->sv_startup = false;
	bts[4].text = txStart;

	if(game->players > 1)
	{
		byte c = ID_END_STARTUP;
		game->peer->Send((cstring)&c, 1, IMMEDIATE_PRIORITY, RELIABLE, 0, UNASSIGNED_SYSTEM_ADDRESS, true);
	}
}

//=================================================================================================
void ServerPanel::UseLoadedCharacter(bool have)
{
	if(have)
	{
		LOG("ServerPanel: Joined loaded game with existing character.");
		game->autopick_class = Class::INVALID;
		bts[0].state = Button::DISABLED;
		bts[1].state = Button::NONE;
		AddMsg(txLoadedCharInfo);
	}
	else
	{
		LOG("ServerPanel: Joined loaded game without loaded character.");
		AddMsg(txNotLoadedCharInfo);
	}
}

//=================================================================================================
void ServerPanel::CheckAutopick()
{
	if(game->autopick_class != Class::INVALID)
	{
		LOG("ServerPanel: Autopicking character.");
		PickClass(game->autopick_class, true);
		game->autopick_class = Class::INVALID;
	}
}

//=================================================================================================
void ServerPanel::PickClass(Class clas, bool ready)
{
	PlayerInfo& info = game->game_players[0];
	info.clas = clas;
	game->RandomCharacter(info.clas, game->hair_redo_index, info.hd, info.cc);
	bts[0].text = txChangeChar;
	bts[1].state = Button::NONE;
	bts[1].text = (ready ? txNotReady : txReady);
	info.ready = ready;
	if(!game->sv_server)
	{
		LOG("ServerPanel: Sent pick class packet.");
		BitStream& stream = game->net_stream;
		stream.Reset();
		stream.WriteCasted<byte>(ID_PICK_CHARACTER);
		WriteCharacterData(stream, info.clas, info.hd, info.cc);
		WriteBool(stream, ready);
		game->peer->Send(&stream, IMMEDIATE_PRIORITY, RELIABLE, 0, game->server, false);
	}
	else
	{
		if(game->players > 1)
			game->AddLobbyUpdate(INT2(Lobby_UpdatePlayer, 0));
		game->CheckReady();
	}
}

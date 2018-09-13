#include "Pch.h"
#include "GameCore.h"
#include "PickServerPanel.h"
#include "Language.h"
#include "KeyStates.h"
#include "Const.h"
#include "Game.h"
#include "Version.h"
#include "BitStreamFunc.h"
#include "ResourceManager.h"

//=================================================================================================
PickServerPanel::PickServerPanel(const DialogInfo& info) : GameDialogBox(info)
{
	size = Int2(524, 340);
	bts.resize(2);

	txUnknownResponse = Str("unknownResponse");
	txUnknownResponse2 = Str("unknownResponse2");
	txBrokenResponse = Str("brokenResponse");

	bts[0].size = Int2(180, 44);
	bts[0].pos = Int2(336, 30);
	bts[0].id = GuiEvent_Custom + BUTTON_OK;
	bts[0].text = Str("join");
	bts[0].parent = this;

	bts[1].size = Int2(180, 44);
	bts[1].pos = Int2(336, 80);
	bts[1].id = GuiEvent_Custom + BUTTON_CANCEL;
	bts[1].text = GUI.txCancel;
	bts[1].parent = this;

	grid.pos = Int2(8, 8);
	grid.size = Int2(320, 300);
	grid.event = GridEvent(this, &PickServerPanel::GetCell);
	grid.AddColumn(Grid::IMGSET, 50);
	grid.AddColumn(Grid::TEXT_COLOR, 100, Str("players"));
	grid.AddColumn(Grid::TEXT_COLOR, 150, Str("name2"));
	grid.Init();
}

//=================================================================================================
void PickServerPanel::Draw(ControlDrawData*)
{
	// t³o
	GUI.DrawSpriteFull(tBackground, Color::Alpha(128));

	// panel
	GUI.DrawItem(tDialog, global_pos, size, Color::Alpha(222), 16);

	// przyciski
	for(int i = 0; i < 2; ++i)
		bts[i].Draw();

	// grid
	grid.Draw();
}

//=================================================================================================
void PickServerPanel::Update(float dt)
{
	// update gui
	for(int i = 0; i < 2; ++i)
	{
		bts[i].mouse_focus = focus;
		bts[i].Update(dt);
	}

	grid.focus = focus;
	grid.Update(dt);

	if(!focus)
		return;

	if(Key.Focus() && Key.PressedRelease(VK_ESCAPE))
	{
		Event((GuiEvent)(GuiEvent_Custom + BUTTON_CANCEL));
		return;
	}

	// ping servers
	ping_timer -= dt;
	if(ping_timer < 0.f)
	{
		ping_timer = 1.f;
		N.peer->Ping("255.255.255.255", (word)N.port, true);
	}

	// listen for packets
	Packet* packet;
	for(packet = N.peer->Receive(); packet; N.peer->DeallocatePacket(packet), packet = N.peer->Receive())
	{
		BitStream& stream = N.StreamStart(packet, Stream_PickServer);
		BitStreamReader reader(stream);
		byte msg_id;
		reader >> msg_id;

		switch(msg_id)
		{
		case ID_UNCONNECTED_PONG:
			{
				// header
				TimeMS time_ms;
				char sign[2];
				reader >> time_ms;
				reader >> sign;
				if(!reader)
				{
					N.StreamError("PickServer: Broken packet from %s.", packet->systemAddress.ToString());
					break;
				}
				if(sign[0] != 'C' || sign[1] != 'A')
				{
					Warn("PickServer: Unknown response from %s, this is not CaRpg server (0x%x%x).",
						packet->systemAddress.ToString(), byte(sign[0]), byte(sign[1]));
					N.StreamError();
					break;
				}

				// info about server
				uint version;
				byte active_players, players_max, flags;
				reader >> version;
				reader >> active_players;
				reader >> players_max;
				reader >> flags;
				const string& server_name = reader.ReadString1();
				if(!reader)
				{
					Warn("PickServer: Broken response from %.", packet->systemAddress.ToString());
					N.StreamError();
					break;
				}

				bool valid_version = (version == VERSION);

				// serach for server in list
				bool found = false;
				int index = 0;
				for(vector<ServerData>::iterator it = servers.begin(), end = servers.end(); it != end; ++it, ++index)
				{
					if(it->adr == packet->systemAddress)
					{
						// update
						found = true;
						Info("PickServer: Updated server info %s.", it->adr.ToString());
						it->name = server_name;
						it->active_players = active_players;
						it->max_players = players_max;
						it->flags = flags;
						it->timer = 0.f;
						it->valid_version = valid_version;

						if(game->pick_autojoin && it->active_players != it->max_players && it->valid_version)
						{
							// autojoin server
							bts[0].state = Button::NONE;
							game->pick_autojoin = false;
							grid.selected = index;
							Event(GuiEvent(GuiEvent_Custom + BUTTON_OK));
						}

						break;
					}
				}

				if(!found)
				{
					// add to servers list
					Info("PickServer: Added server info %s.", packet->systemAddress.ToString());
					ServerData& sd = Add1(servers);
					sd.name = server_name;
					sd.active_players = active_players;
					sd.max_players = players_max;
					sd.adr = packet->systemAddress;
					sd.flags = flags;
					sd.timer = 0.f;
					sd.valid_version = valid_version;
					grid.AddItem();

					if(game->pick_autojoin && sd.active_players != sd.max_players && sd.valid_version)
					{
						// autojoin server
						bts[0].state = Button::NONE;
						game->pick_autojoin = false;
						grid.selected = servers.size() - 1;
						Event(GuiEvent(GuiEvent_Custom + BUTTON_OK));
					}
				}
			}
			break;
		default:
			Warn("PickServer: Unknown packet %d from %s.", msg_id, packet->systemAddress.ToString());
			N.StreamError();
			break;
		}

		N.StreamEnd();
	}

	// update servers
	int index = 0;
	for(vector<ServerData>::iterator it = servers.begin(), end = servers.end(); it != end;)
	{
		it->timer += dt;
		if(it->timer >= 2.f)
		{
			grid.RemoveItem(index);
			it = servers.erase(it);
			end = servers.end();
		}
		else
		{
			++it;
			++index;
		}
	}

	// enable/disable join button
	if(grid.selected == -1 || !servers[grid.selected].valid_version)
		bts[0].state = Button::DISABLED;
	else if(bts[0].state == Button::DISABLED)
		bts[0].state = Button::NONE;
}

//=================================================================================================
void PickServerPanel::Event(GuiEvent e)
{
	if(e == GuiEvent_Show || e == GuiEvent_WindowResize)
	{
		if(e == GuiEvent_Show)
			visible = true;
		pos = global_pos = (GUI.wnd_size - size) / 2;
		for(int i = 0; i < 2; ++i)
			bts[i].global_pos = global_pos + bts[i].pos;
		grid.Move(global_pos);
	}
	else if(e == GuiEvent_Close)
	{
		visible = false;
		grid.LostFocus();
	}
	else if(e == GuiEvent_LostFocus)
		grid.LostFocus();
	else if(e == GuiEvent_Custom + BUTTON_OK)
		event(BUTTON_OK);
	else if(e == GuiEvent_Custom + BUTTON_CANCEL)
		event(BUTTON_CANCEL);
}

//=================================================================================================
void PickServerPanel::Show()
{
	try
	{
		N.InitClient();
	}
	catch(cstring err)
	{
		GUI.SimpleDialog(err, (Control*)game->main_menu);
		return;
	}

	Info("Pinging servers.");
	N.peer->Ping("255.255.255.255", (word)N.port, true);

	ping_timer = 1.f;
	servers.clear();
	grid.Reset();

	GUI.ShowDialog(this);
}

//=================================================================================================
void PickServerPanel::GetCell(int item, int column, Cell& cell)
{
	ServerData& server = servers[item];

	if(column == 0)
	{
		vector<TEX>& imgs = *cell.imgset;
		if(IS_SET(server.flags, SERVER_PASSWORD))
			imgs.push_back(tIcoHaslo);
		if(IS_SET(server.flags, SERVER_SAVED))
			imgs.push_back(tIcoZapis);
	}
	else
	{
		cell.text_color->color = (server.valid_version ? Color::Black : Color::Red);
		if(column == 1)
			cell.text_color->text = Format("%d/%d", server.active_players, server.max_players);
		else
			cell.text_color->text = server.name.c_str();
	}
}

//=================================================================================================
void PickServerPanel::LoadData()
{
	auto& tex_mgr = ResourceManager::Get<Texture>();
	tex_mgr.AddLoadTask("save-16.png", tIcoZapis);
	tex_mgr.AddLoadTask("padlock-16.png", tIcoHaslo);
}

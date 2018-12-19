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
#include "GlobalGui.h"
#include "LobbyApi.h"
#include <json.hpp>

//=================================================================================================
PickServerPanel::PickServerPanel(const DialogInfo& info) : GameDialogBox(info), pick_autojoin(false)
{
	size = Int2(524, 340);
	bts.resize(2);

	bts[0].size = Int2(180, 44);
	bts[0].pos = Int2(336, 30);
	bts[0].id = IdOk;
	bts[0].parent = this;

	bts[1].size = Int2(180, 44);
	bts[1].pos = Int2(336, 80);
	bts[1].id = IdCancel;
	bts[1].parent = this;

	cb_internet.id = IdInternet;
	cb_internet.radiobox = true;
	cb_internet.bt_size = Int2(32, 32);
	cb_internet.parent = this;
	cb_internet.pos = Int2(336, 130);
	cb_internet.size = Int2(200, 32);

	cb_lan.id = IdLan;
	cb_lan.radiobox = true;
	cb_lan.bt_size = Int2(32, 32);
	cb_lan.parent = this;
	cb_lan.pos = Int2(336, 170);
	cb_lan.size = Int2(200, 32);

	grid.pos = Int2(8, 8);
	grid.size = Int2(320, 300);
	grid.event = GridEvent(this, &PickServerPanel::GetCell);
}

//=================================================================================================
void PickServerPanel::LoadLanguage()
{
	auto s = Language::GetSection("PickServerPanel");
	txFailedToGetServers = s.Get("failedToGetServers");

	bts[0].text = Str("join");
	bts[1].text = GUI.txCancel;

	cb_internet.text = s.Get("internet");
	cb_lan.text = s.Get("lan");

	grid.AddColumn(Grid::IMGSET, 50);
	grid.AddColumn(Grid::TEXT_COLOR, 100, s.Get("players"));
	grid.AddColumn(Grid::TEXT_COLOR, 150, s.Get("name"));
	grid.Init();
}

//=================================================================================================
void PickServerPanel::LoadData()
{
	auto& tex_mgr = ResourceManager::Get<Texture>();
	tex_mgr.AddLoadTask("save-16.png", tIcoSave);
	tex_mgr.AddLoadTask("padlock-16.png", tIcoPassword);
}

//=================================================================================================
void PickServerPanel::Draw(ControlDrawData*)
{
	// background
	GUI.DrawSpriteFull(tBackground, Color::Alpha(128));

	// panel
	GUI.DrawItem(tDialog, global_pos, size, Color::Alpha(222), 16);

	// controls
	for(int i = 0; i < 2; ++i)
		bts[i].Draw();
	cb_internet.Draw();
	cb_lan.Draw();
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
	cb_internet.mouse_focus = focus;
	cb_internet.Update(dt);
	cb_lan.mouse_focus = focus;
	cb_lan.Update(dt);
	grid.focus = focus;
	grid.Update(dt);

	// update lobby api
	N.api->Update(dt);

	if(!focus)
		return;

	if(Key.Focus() && Key.PressedRelease(VK_ESCAPE))
	{
		Event((GuiEvent)(IdCancel));
		return;
	}

	// ping servers
	timer += dt;
	if(timer >= 1.f)
	{
		if(lan_mode)
		{
			N.peer->Ping("255.255.255.255", (word)N.port, false);
			timer = 0;
		}
		else if(!N.api->IsBusy() && !bad_request)
		{
			if(timer > 30.f)
				N.api->GetServers();
			else
				N.api->GetChanges();
			timer = 0;
		}
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
			if(lan_mode)
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

				// search for server in list
				bool found = false;
				int index = 0;
				for(vector<ServerData>::iterator it = servers.begin(), end = servers.end(); it != end; ++it, ++index)
				{
					if(it->adr == packet->systemAddress)
					{
						// update
						found = true;
						Info("PickServer: Updated server %s (%s).", it->name.c_str(), it->adr.ToString());
						it->name = server_name;
						it->active_players = active_players;
						it->max_players = players_max;
						it->flags = flags;
						it->timer = 0.f;
						it->valid_version = valid_version;

						CheckAutojoin();
						break;
					}
				}

				if(!found)
				{
					// add to servers list
					Info("PickServer: Added server %s (%s).", server_name.c_str(), packet->systemAddress.ToString());
					ServerData& sd = Add1(servers);
					sd.id = -1;
					sd.name = server_name;
					sd.active_players = active_players;
					sd.max_players = players_max;
					sd.adr = packet->systemAddress;
					sd.flags = flags;
					sd.timer = 0.f;
					sd.valid_version = valid_version;
					grid.AddItem();

					CheckAutojoin();
				}
			}
			break;
		case ID_DISCONNECTION_NOTIFICATION:
			// when client was connecting to server using master server and have invalid password
			// disconnecting from server will be received here, ignore message
			break;
		default:
			Warn("PickServer: Unknown packet %d from %s.", msg_id, packet->systemAddress.ToString());
			N.StreamError();
			break;
		}

		N.StreamEnd();
	}

	// update servers
	if(lan_mode)
	{
		int index = 0;
		for(vector<ServerData>::iterator it = servers.begin(), end = servers.end(); it != end;)
		{
			it->timer += dt;
			if(it->timer >= 2.f)
			{
				Info("PickServer: Removed server %s (%s).", it->name.c_str(), it->adr.ToString());
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
	switch(e)
	{
	case GuiEvent_Show:
	case GuiEvent_WindowResize:
		if(e == GuiEvent_Show)
			visible = true;
		pos = global_pos = (GUI.wnd_size - size) / 2;
		for(int i = 0; i < 2; ++i)
			bts[i].global_pos = global_pos + bts[i].pos;
		cb_internet.global_pos = global_pos + cb_internet.pos;
		cb_lan.global_pos = global_pos + cb_lan.pos;
		grid.Move(global_pos);
		break;
	case GuiEvent_Close:
		visible = false;
		grid.LostFocus();
		break;
	case GuiEvent_LostFocus:
		grid.LostFocus();
		break;
	case IdOk:
		event(e);
		break;
	case IdCancel:
		N.ClosePeer();
		N.peer->Shutdown(0);
		CloseDialog();
		break;
	case IdInternet:
		cb_lan.checked = false;
		OnChangeMode(false);
		break;
	case IdLan:
		cb_internet.checked = false;
		OnChangeMode(true);
		break;
	}
}

//=================================================================================================
void PickServerPanel::Show(bool pick_autojoin)
{
	this->pick_autojoin = pick_autojoin;

	try
	{
		N.InitClient();
	}
	catch(cstring err)
	{
		GUI.SimpleDialog(err, (Control*)game->gui->main_menu);
		return;
	}

	Info("Getting servers from master server.");
	lan_mode = false;
	bad_request = false;
	cb_internet.checked = true;
	cb_lan.checked = false;
	N.api->Reset();
	N.api->GetServers();
	timer = 0;
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
			imgs.push_back(tIcoPassword);
		if(IS_SET(server.flags, SERVER_SAVED))
			imgs.push_back(tIcoSave);
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
void PickServerPanel::OnChangeMode(bool lan_mode)
{
	this->lan_mode = lan_mode;
	if(lan_mode)
	{
		N.api->Reset();
		N.peer->Ping("255.255.255.255", (word)N.port, false);
	}
	else
	{
		N.api->GetServers();
		bad_request = false;
	}
	servers.clear();
	grid.Reset();
	timer = 0;
}

//=================================================================================================
bool PickServerPanel::HandleGetServers(nlohmann::json& j)
{
	if(!visible || lan_mode || GUI.HaveDialog("GetTextDialog"))
		return false;

	auto& servers = j["servers"];
	for(auto it = servers.begin(), end = servers.end(); it != end; ++it)
		AddServer(*it);

	CheckAutojoin();
	return true;
}

//=================================================================================================
void PickServerPanel::AddServer(nlohmann::json& server)
{
	ServerData& sd = Add1(servers);
	sd.id = server["id"].get<int>();
	sd.guid = server["guid"].get_ref<string&>();
	sd.name = server["name"].get_ref<string&>();
	sd.active_players = server["players"].get<int>();
	sd.max_players = server["maxPlayers"].get<int>();
	sd.flags = server["flags"].get<int>();
	int version = server["version"].get<int>();
	sd.timer = 0.f;
	sd.valid_version = (version == VERSION);
	grid.AddItem();
	Info("PickServer: Added server %s (%d).", sd.name.c_str(), sd.id);
}

//=================================================================================================
bool PickServerPanel::HandleGetChanges(nlohmann::json& j)
{
	if(!visible || lan_mode || GUI.HaveDialog("GetTextDialog"))
		return false;

	auto& changes = j["changes"];
	for(auto it = changes.begin(), end = changes.end(); it != end; ++it)
	{
		auto& change = *it;
		int type = change["type"].get<int>();
		switch(type)
		{
		case 0: // add server
			AddServer(change["server"]);
			break;
		case 1: // update server
			{
				auto& server = change["server"];
				int id = server["id"].get<int>();
				int players = server["players"].get<int>();
				bool found = false;
				for(ServerData& sd : servers)
				{
					if(sd.id == id)
					{
						sd.active_players = players;
						found = true;
						Info("PickServer: Updated server %s (%d).", sd.name.c_str(), id);
						break;
					}
				}
				if(!found)
					Error("PickServer: Missing server %d to update.", id);
			}
			break;
		case 2: // remove server
			{
				int id = change["serverID"].get<int>();
				int index = 0;
				bool found = false;
				for(auto it2 = servers.begin(), end2 = servers.end(); it2 != end2; ++it2)
				{
					if(it2->id == id)
					{
						Info("PickServer: Removed server %s (%d).", it2->name.c_str(), it2->id);
						found = true;
						grid.RemoveItem(index);
						servers.erase(it2);
						break;
					}
					++index;
				}
				if(!found)
					Error("PickServer: Missing server %d to remove.", id);
			}
			break;
		}
	}

	CheckAutojoin();
	return true;
}

//=================================================================================================
void PickServerPanel::CheckAutojoin()
{
	if(!pick_autojoin)
		return;
	int index = 0;
	for(ServerData& sd : servers)
	{
		if(sd.active_players != sd.max_players && sd.valid_version)
		{
			// autojoin server
			bts[0].state = Button::NONE;
			pick_autojoin = false;
			grid.selected = index;
			Event(GuiEvent(IdOk));
		}
		++index;
	}
}

//=================================================================================================
void PickServerPanel::HandleBadRequest()
{
	bad_request = true;
	GUI.SimpleDialog(txFailedToGetServers, this);
}

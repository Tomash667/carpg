#include "Pch.h"
#include "PickServerPanel.h"

#include "BitStreamFunc.h"
#include "Const.h"
#include "Game.h"
#include "GameGui.h"
#include "Language.h"
#include "LobbyApi.h"
#include "Version.h"

#include <Input.h>
#include <json.hpp>
#include <ResourceManager.h>

//=================================================================================================
PickServerPanel::PickServerPanel(const DialogInfo& info) : DialogBox(info), pickAutojoin(false)
{
	size = Int2(544, 390);
	bts.resize(2);

	grid.pos = Int2(20, 70);
	grid.size = Int2(320, 300);
	grid.event = GridEvent(this, &PickServerPanel::GetCell);

	cbInternet.id = IdInternet;
	cbInternet.radiobox = true;
	cbInternet.btSize = Int2(32, 32);
	cbInternet.parent = this;
	cbInternet.pos = Int2(355, 80);
	cbInternet.size = Int2(200, 32);

	cbLan.id = IdLan;
	cbLan.radiobox = true;
	cbLan.btSize = Int2(32, 32);
	cbLan.parent = this;
	cbLan.pos = Int2(355, 120);
	cbLan.size = Int2(200, 32);

	bts[0].size = Int2(180, 44);
	bts[0].pos = Int2(345, 390 - 22 - 44 - 50);
	bts[0].id = IdOk;
	bts[0].parent = this;

	bts[1].size = Int2(180, 44);
	bts[1].pos = Int2(345, 390 - 22 - 44);
	bts[1].id = IdCancel;
	bts[1].parent = this;
}

//=================================================================================================
void PickServerPanel::LoadLanguage()
{
	auto s = Language::GetSection("PickServerPanel");
	txTitle = s.Get("title");
	txFailedToGetServers = s.Get("failedToGetServers");
	txInvalidServerVersion = s.Get("invalidServerVersion");

	bts[0].text = s.Get("join");
	bts[1].text = gui->txCancel;

	cbInternet.text = s.Get("internet");
	cbLan.text = s.Get("lan");

	grid.AddColumn(Grid::IMGSET, 50);
	grid.AddColumn(Grid::TEXT_COLOR, 100, s.Get("players"));
	grid.AddColumn(Grid::TEXT_COLOR, 150, s.Get("name"));
	grid.Init();
}

//=================================================================================================
void PickServerPanel::LoadData()
{
	tIcoSave = resMgr->Load<Texture>("save-16.png");
	tIcoPassword = resMgr->Load<Texture>("padlock-16.png");
}

//=================================================================================================
void PickServerPanel::Draw()
{
	DrawPanel();

	// header
	Rect r = { globalPos.x + 12, globalPos.y + 8, globalPos.x + size.x - 12, globalPos.y + size.y };
	gui->DrawText(GameGui::fontBig, txTitle, DTF_TOP | DTF_CENTER, Color::Black, r);

	// controls
	for(int i = 0; i < 2; ++i)
		bts[i].Draw();
	cbInternet.Draw();
	cbLan.Draw();
	grid.Draw();
}

//=================================================================================================
void PickServerPanel::Update(float dt)
{
	// update gui
	for(int i = 0; i < 2; ++i)
	{
		bts[i].mouseFocus = focus;
		bts[i].Update(dt);
	}
	cbInternet.mouseFocus = focus;
	cbInternet.Update(dt);
	cbLan.mouseFocus = focus;
	cbLan.Update(dt);
	grid.focus = focus;
	grid.Update(dt);

	if(!focus || !visible)
		return;

	if(input->Focus() && input->PressedRelease(Key::Escape))
	{
		Event((GuiEvent)(IdCancel));
		return;
	}

	// ping servers
	timer += dt;
	if(timer >= 1.f)
	{
		if(lanMode)
		{
			net->peer->Ping("255.255.255.255", (word)net->port, false);
			timer = 0;
		}
		else if(!badRequest)
		{
			if(timer > 30.f)
				api->GetServers();
			else
				api->GetChanges();
			timer = 0;
		}
	}

	// listen for packets
	Packet* packet;
	for(packet = net->peer->Receive(); packet; net->peer->DeallocatePacket(packet), packet = net->peer->Receive())
	{
		BitStreamReader reader(packet);
		byte msg_id;
		reader >> msg_id;

		switch(msg_id)
		{
		case ID_UNCONNECTED_PONG:
			if(lanMode)
			{
				// header
				TimeMS time_ms;
				char sign[2];
				reader >> time_ms;
				reader >> sign;
				if(!reader || sign[0] != 'C' || sign[1] != 'A')
				{
					// someone responded but this is not carpg server or game already started and we are ignored
					break;
				}

				// info about server
				uint version;
				byte activePlayers, players_max, flags;
				reader >> version;
				reader >> activePlayers;
				reader >> players_max;
				reader >> flags;
				const string& serverName = reader.ReadString1();
				if(!reader)
				{
					Warn("PickServer: Broken response from %.", packet->systemAddress.ToString());
					break;
				}

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
						it->name = serverName;
						it->activePlayers = activePlayers;
						it->maxPlayers = players_max;
						it->flags = flags;
						it->timer = 0.f;
						it->version = version;

						CheckAutojoin();
						break;
					}
				}

				if(!found)
				{
					// add to servers list
					Info("PickServer: Added server %s (%s).", serverName.c_str(), packet->systemAddress.ToString());
					ServerData& sd = Add1(servers);
					sd.id = -1;
					sd.name = serverName;
					sd.activePlayers = activePlayers;
					sd.maxPlayers = players_max;
					sd.adr = packet->systemAddress;
					sd.flags = flags;
					sd.timer = 0.f;
					sd.version = version;
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
			break;
		}
	}

	// update servers
	if(lanMode)
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
	if(grid.selected == -1)
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
		pos = globalPos = (gui->wndSize - size) / 2;
		for(int i = 0; i < 2; ++i)
			bts[i].globalPos = globalPos + bts[i].pos;
		cbInternet.globalPos = globalPos + cbInternet.pos;
		cbLan.globalPos = globalPos + cbLan.pos;
		grid.Move(globalPos);
		break;
	case GuiEvent_Close:
		visible = false;
		grid.LostFocus();
		break;
	case GuiEvent_LostFocus:
		grid.LostFocus();
		break;
	case IdOk:
		if(servers[grid.selected].IsValidVersion())
			event(e);
		else
			gui->SimpleDialog(Format(txInvalidServerVersion, VersionToString(servers[grid.selected].version), VERSION_STR), this);
		break;
	case IdCancel:
		net->ClosePeer();
		net->peer->Shutdown(0);
		CloseDialog();
		break;
	case IdInternet:
		cbLan.checked = false;
		OnChangeMode(false);
		break;
	case IdLan:
		cbInternet.checked = false;
		OnChangeMode(true);
		break;
	}
}

//=================================================================================================
void PickServerPanel::Show(bool pickAutojoin)
{
	this->pickAutojoin = pickAutojoin;

	try
	{
		net->InitClient();
	}
	catch(cstring err)
	{
		gui->SimpleDialog(err, (Control*)game_gui->mainMenu);
		return;
	}

	if(net->joinLan)
	{
		Info("Pinging LAN servers...");
		lanMode = true;
		cbInternet.checked = false;
		cbLan.checked = true;
		net->peer->Ping("255.255.255.255", (word)net->port, false);
	}
	else
	{
		Info("Getting servers from master server.");
		lanMode = false;
		cbInternet.checked = true;
		cbLan.checked = false;
		api->Reset();
		api->GetServers();
	}

	badRequest = false;
	timer = 0;
	servers.clear();
	grid.Reset();

	gui->ShowDialog(this);
}

//=================================================================================================
void PickServerPanel::GetCell(int item, int column, Cell& cell)
{
	ServerData& server = servers[item];

	if(column == 0)
	{
		vector<Texture*>& imgs = *cell.imgset;
		if(IsSet(server.flags, SERVER_PASSWORD))
			imgs.push_back(tIcoPassword);
		if(IsSet(server.flags, SERVER_SAVED))
			imgs.push_back(tIcoSave);
	}
	else
	{
		cell.textColor->color = (server.IsValidVersion() ? Color::Black : Color::Red);
		if(column == 1)
			cell.textColor->text = Format("%d/%d", server.activePlayers, server.maxPlayers);
		else
			cell.textColor->text = server.name.c_str();
	}
}

//=================================================================================================
void PickServerPanel::OnChangeMode(bool lanMode)
{
	this->lanMode = lanMode;
	if(lanMode)
	{
		api->Reset();
		net->peer->Ping("255.255.255.255", (word)net->port, false);
	}
	else
	{
		api->GetServers();
		badRequest = false;
	}
	net->joinLan = lanMode;
	game->cfg.Add("joinLan", lanMode);
	game->SaveCfg();
	servers.clear();
	grid.Reset();
	timer = 0;
}

//=================================================================================================
bool PickServerPanel::HandleGetServers(nlohmann::json& j)
{
	if(!visible || lanMode || gui->HaveDialog("GetTextDialog"))
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
	sd.activePlayers = server["players"].get<int>();
	sd.maxPlayers = server["maxPlayers"].get<int>();
	sd.flags = server["flags"].get<int>();
	sd.version = server["version"].get<int>();
	sd.timer = 0.f;
	grid.AddItem();
	Info("PickServer: Added server %s (%d).", sd.name.c_str(), sd.id);
}

//=================================================================================================
bool PickServerPanel::HandleGetChanges(nlohmann::json& j)
{
	if(!visible || lanMode || gui->HaveDialog("GetTextDialog"))
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
						sd.activePlayers = players;
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
	if(!pickAutojoin)
		return;
	int index = 0;
	for(ServerData& sd : servers)
	{
		if(sd.activePlayers != sd.maxPlayers && sd.IsValidVersion())
		{
			// autojoin server
			bts[0].state = Button::NONE;
			pickAutojoin = false;
			grid.selected = index;
			Event(GuiEvent(IdOk));
		}
		++index;
	}
}

//=================================================================================================
void PickServerPanel::HandleBadRequest()
{
	badRequest = true;
	gui->SimpleDialog(txFailedToGetServers, this);
}

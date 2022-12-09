#pragma once

//-----------------------------------------------------------------------------
#include <DialogBox.h>
#include <Grid.h>
#include <CheckBox.h>
#include "Version.h"

//-----------------------------------------------------------------------------
// Panel used to select and connect to server, supports online & lan mode
class PickServerPanel : public DialogBox
{
public:
	enum Id
	{
		IdOk = GuiEvent_Custom,
		IdCancel,
		IdInternet,
		IdLan
	};

	struct ServerData
	{
		int id;
		string name, guid;
		SystemAddress adr;
		uint activePlayers, maxPlayers;
		int flags, version;
		float timer;

		bool IsValidVersion() const { return version == VERSION; }
	};

	explicit PickServerPanel(const DialogInfo& info);
	void LoadLanguage();
	void LoadData();
	void Draw() override;
	void Update(float dt) override;
	void Event(GuiEvent e) override;
	void Show(bool pickAutojoin = false);
	void GetCell(int item, int column, Cell& cell);
	bool HandleGetServers(nlohmann::json&);
	bool HandleGetChanges(nlohmann::json&);
	void HandleBadRequest();
	bool IsLAN() const { return lanMode; }

	Grid grid;
	vector<ServerData> servers;

private:
	void OnChangeMode(bool lanMode);
	void AddServer(nlohmann::json&);
	void CheckAutojoin();

	TexturePtr tIcoPassword, tIcoSave;
	CheckBox cbInternet, cbLan;
	float timer;
	cstring txTitle, txFailedToGetServers, txInvalidServerVersion;
	bool pickAutojoin, lanMode, badRequest;
};

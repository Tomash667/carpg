#pragma once

//-----------------------------------------------------------------------------
#include <DialogBox.h>
#include <Grid.h>
#include <CheckBox.h>
#include "Version.h"

//-----------------------------------------------------------------------------
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
		uint active_players, max_players;
		int flags, version;
		float timer;

		bool IsValidVersion() const { return version == VERSION; }
	};

	explicit PickServerPanel(const DialogInfo& info);
	void LoadLanguage();
	void LoadData();
	void Draw(ControlDrawData* cdd = nullptr) override;
	void Update(float dt) override;
	void Event(GuiEvent e) override;
	void Show(bool pick_autojoin = false);
	void GetCell(int item, int column, Cell& cell);
	bool HandleGetServers(nlohmann::json&);
	bool HandleGetChanges(nlohmann::json&);
	void HandleBadRequest();
	bool IsLAN() const { return lan_mode; }

	Grid grid;
	vector<ServerData> servers;

private:
	void OnChangeMode(bool lan_mode);
	void AddServer(nlohmann::json&);
	void CheckAutojoin();

	TexturePtr tIcoPassword, tIcoSave;
	CheckBox cb_internet, cb_lan;
	float timer;
	cstring txFailedToGetServers, txInvalidServerVersion;
	bool pick_autojoin, lan_mode, bad_request;
};

#pragma once

//-----------------------------------------------------------------------------
#include "GameDialogBox.h"
#include "Grid.h"
#include "CheckBox.h"

//-----------------------------------------------------------------------------
class PickServerPanel : public GameDialogBox
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
		int flags;
		float timer;
		bool valid_version;
	};

	explicit PickServerPanel(const DialogInfo& info);
	void LoadLanguage();
	void LoadData();
	void Draw(ControlDrawData* cdd = nullptr) override;
	void Update(float dt) override;
	void Event(GuiEvent e) override;
	void Show(bool pick_autojoin = false);
	void GetCell(int item, int column, Cell& cell);
	void HandleGetServers(nlohmann::json&);
	void HandleGetChanges(nlohmann::json&);
	void HandleBadRequest();

	Grid grid;
	vector<ServerData> servers;
	float ping_timer;
	cstring txUnknownResponse, txUnknownResponse2, txBrokenResponse;

private:
	void OnChangeMode(bool lan_mode);
	void AddServer(nlohmann::json&);
	void CheckAutojoin();

	TEX tIcoPassword, tIcoSave;
	CheckBox cb_internet, cb_lan;
	bool pick_autojoin, lan_mode, bad_request;
};

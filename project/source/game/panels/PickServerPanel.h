#pragma once

//-----------------------------------------------------------------------------
#include "GameDialogBox.h"
#include "Grid.h"

//-----------------------------------------------------------------------------
class PickServerPanel : public GameDialogBox
{
public:
	struct ServerData
	{
		string name;
		SystemAddress adr;
		uint active_players, max_players;
		int flags;
		float timer;
		bool valid_version;
	};

	explicit PickServerPanel(const DialogInfo& info);

	void Draw(ControlDrawData* cdd = nullptr) override;
	void Update(float dt) override;
	void Event(GuiEvent e) override;

	void Show();
	void GetCell(int item, int column, Cell& cell);
	void LoadData();

	Grid grid;
	vector<ServerData> servers;
	float ping_timer;
	cstring txUnknownResponse, txUnknownResponse2, txBrokenResponse;

private:
	TEX tIcoHaslo, tIcoZapis;
};

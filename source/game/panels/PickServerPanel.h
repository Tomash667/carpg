#pragma once

//-----------------------------------------------------------------------------
#include "Dialog2.h"
#include "Grid.h"

//-----------------------------------------------------------------------------
class PickServerPanel : public Dialog
{
public:
	struct ServerData
	{
		string name;
		SystemAddress adr;
		int players, max_players;
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

	Grid grid;
	vector<ServerData> servers;
	float ping_timer;
	cstring txUnknownResponse, txUnknownResponse2, txBrokenResponse;
};

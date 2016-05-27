#pragma once

#include "Container.h"

class MenuBar;

class Panel : public Container
{
public:
	Panel();

	virtual void Draw(ControlDrawData* cdd = nullptr) override;
	virtual void Update(float dt) override;

	void SetMenu(MenuBar* menu);
	void BeginUpdate();
	void EndUpdate();
	void SetSize(const INT2& size);
	void SetPosition(const INT2& pos);

private:
	void Refresh();
	inline void NeedRefresh()
	{
		if(in_update)
			need_refresh = true;
		else
			Refresh();
	}
	void Reposition();
	inline void NeedReposition()
	{
		if(in_update)
			need_reposition = true;
		else
			Reposition();
	}

	MenuBar* menu;
	bool in_update, need_refresh, need_reposition;
};

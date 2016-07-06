#pragma once

#include "Container2.h"

namespace gui
{
	class MenuBar;

	class Panel2 : public Container2
	{
	public:
		//Panel2();

		//virtual void Update(float dt) override;

		/*virtual void Draw(ControlDrawData* cdd = nullptr) override;
		virtual void Update(float dt) override;*/

		//void SetMenu(MenuBar* menu);
		/*void BeginUpdate();
		void EndUpdate();*/
		//virtual void SetSize(const INT2& size) override;
		//virtual void SetPosition(const INT2& pos) override;

	private:
		/*void Refresh();
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
		}*/

		//virtual void Draw2() override;

		MenuBar* menu;
		DWORD bkg_color;
		//bool in_update, need_refresh, need_reposition;
	};
}

// zal¹¿ek inteligentnego GUI, samo ustala która kontrolka jest aktywna, aktualizuje po³o¿enie, rysuje, aktualizuje
#include "Pch.h"
#include "Base.h"
#include "GuiContainer.h"
#include "KeyStates.h"

//=================================================================================================
GuiContainer::GuiContainer() : focus(false), focus_ctrl(nullptr), give_focus(nullptr)
{

}

//=================================================================================================
void GuiContainer::Draw()
{
	for(Iter it = items.begin(), end = items.end(); it != end; ++it)
		it->first->Draw();
}

//=================================================================================================
void GuiContainer::Update(float dt)
{
	// przeka¿ focus kontrolce
	if(focus && give_focus)
		CheckGiveFocus();

	// aktualizuj
	// jeœli nic nie jest aktywne to aktywuj pierwsz¹ kontrolkê
	for(Iter it = items.begin(), end = items.end(); it != end; ++it)
	{
		if(IS_SET(it->second, F_MOUSE_FOCUS))
			it->first->mouse_focus = focus;
		if(focus)
		{
			if(!focus_ctrl && IS_SET(it->second, F_FOCUS))
			{
				focus_ctrl = it->first;
				focus_ctrl->GainFocus();
			}
			if(IS_SET(it->second, F_CLICK_TO_FOCUS))
			{
				if(it->first->IsInside(GUI.cursor_pos) && (Key.Pressed(VK_LBUTTON) || Key.Pressed(VK_RBUTTON)))
				{
					if(!it->first->focus)
					{
						if(focus_ctrl)
							focus_ctrl->LostFocus();
						focus_ctrl = it->first;
						focus_ctrl->GainFocus();
					}
				}
			}
		}
		it->first->Update(dt);
	}

	// prze³¹czanie
	if(focus && Key.Focus() && Key.PressedRelease(VK_TAB))
	{
		// znajdŸ aktualny element
		Iter begin = items.begin(), end = items.end(), start;
		for(start = begin; start != end; ++start)
		{
			if(start->first == focus_ctrl)
				break;
		}
		if(start == end)
			return;
		Iter new_item = end;
		if(Key.Down(VK_SHIFT))
		{
			// znajdŸ poprzedni
			// od start do begin
			if(start != begin)
			{
				for(Iter it = start-1; it != begin; --it)
				{
					if(IS_SET(it->second, F_FOCUS))
					{
						new_item = it;
						break;
					}
				}
			}
			// od end do start
			if(new_item == end)
			{
				for(Iter it = end-1; it != start; --it)
				{
					if(IS_SET(it->second, F_FOCUS))
					{
						new_item = it;
						break;
					}
				}
			}
		}
		else
		{
			// znajdŸ nastêpny
			// od start do end
			for(Iter it = start+1; it != end; ++it)
			{
				if(IS_SET(it->second, F_FOCUS))
				{
					new_item = it;
					break;
				}
			}
			// od begin do start
			if(new_item == end)
			{
				for(Iter it = begin; it != start; ++it)
				{
					if(IS_SET(it->second, F_FOCUS))
					{
						new_item = it;
						break;
					}
				}
			}
		}

		// zmiana focus
		if(new_item != end)
		{
			if(focus_ctrl)
				focus_ctrl->LostFocus();
			focus_ctrl = new_item->first;
			focus_ctrl->GainFocus();
		}
	}
}

//=================================================================================================
void GuiContainer::GainFocus()
{
	if(focus)
		return;

	focus = true;
	// aktywuj coœ
	if(give_focus)
		CheckGiveFocus();
	else if(!focus_ctrl)
	{
		for(Iter it = items.begin(), end = items.end(); it != end; ++it)
		{
			if(IS_SET(it->second, F_FOCUS))
			{
				focus_ctrl = it->first;
				focus_ctrl->GainFocus();
				break;
			}
		}
	}
}

//=================================================================================================
void GuiContainer::LostFocus()
{
	if(!focus)
		return;

	focus = false;
	if(focus_ctrl)
	{
		focus_ctrl->LostFocus();
		focus_ctrl = nullptr;
	}
}

//=================================================================================================
void GuiContainer::Move(const INT2& global_pos)
{
	for(Iter it = items.begin(), end = items.end(); it != end; ++it)
		it->first->global_pos = global_pos + it->first->pos;
}

//=================================================================================================
void GuiContainer::CheckGiveFocus()
{
	bool ok = false;
	for(Iter it = items.begin(), end = items.end(); it != end; ++it)
	{
		if(it->first == give_focus)
		{
			if(focus_ctrl != it->first)
			{
				if(focus_ctrl)
					focus_ctrl->LostFocus();
				focus_ctrl = give_focus;
				focus_ctrl->GainFocus();
			}
			ok = true;
			break;
		}
	}
	give_focus = nullptr;
	assert(ok);
}

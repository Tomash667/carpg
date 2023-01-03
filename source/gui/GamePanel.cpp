#include "Pch.h"
#include "GamePanel.h"

#include "GameGui.h"
#include "Language.h"

#include <DialogBox.h>
#include <Input.h>

//-----------------------------------------------------------------------------
TexturePtr GamePanel::tBackground, GamePanel::tDialog;

//-----------------------------------------------------------------------------
enum MenuId
{
	Menu_Edit,
	Menu_Save,
	Menu_Load,
	Menu_LoadDefault
};

//=================================================================================================
GamePanel::GamePanel() : boxState(BOX_NOT_VISIBLE), order(0), lastIndex(INDEX_INVALID), lastIndex2(INDEX_INVALID)
{
	focusable = true;
}

//=================================================================================================
void GamePanel::Draw()
{
	gui->DrawItem(tBackground, pos, size, Color::Alpha(222), 16);
}

//=================================================================================================
void GamePanel::Event(GuiEvent e)
{
	if(e == GuiEvent_Show)
	{
		boxState = BOX_NOT_VISIBLE;
		lastIndex = INDEX_INVALID;
	}
}

//=================================================================================================
void GamePanel::DrawBox()
{
	if(boxState == BOX_VISIBLE)
		static_cast<GamePanelContainer*>(parent)->drawBox = this;
}

//=================================================================================================
void GamePanel::DrawBoxInternal()
{
	int alpha = int(boxAlpha * 222),
		alpha2 = int(boxAlpha * 255);

	// box
	gui->DrawItem(tDialog, boxPos, boxSize, Color::Alpha(alpha), 12);

	// obrazek
	if(boxImg)
		gui->DrawSprite(boxImg, boxImgPos, Color::Alpha(alpha2));

	// du¿y tekst
	gui->DrawText(GameGui::font, boxText, DTF_PARSE_SPECIAL, Color(0, 0, 0, alpha2), boxBig);

	// ma³y tekst
	if(!boxTextSmall.empty())
		gui->DrawText(GameGui::fontSmall, boxTextSmall, DTF_PARSE_SPECIAL, Color(0, 0, 0, alpha2), boxSmall);
}

//=================================================================================================
void GamePanel::UpdateBoxIndex(float dt, int index, int index2, bool refresh)
{
	if(index != INDEX_INVALID)
	{
		if(index != lastIndex || index2 != lastIndex2)
		{
			boxState = BOX_COUNTING;
			lastIndex = index;
			lastIndex2 = index2;
			showTimer = 0.3f;
		}
		else
			showTimer -= dt;

		if(boxState == BOX_COUNTING)
		{
			if(showTimer <= 0.f)
			{
				boxState = BOX_VISIBLE;
				boxAlpha = 0.f;
				refresh = true;
			}
		}
		else
		{
			boxAlpha += dt * 5;
			if(boxAlpha >= 1.f)
				boxAlpha = 1.f;
		}

		if(refresh)
		{
			FormatBox(refresh);
			if(boxImg)
				boxImgSize = boxImg->GetSize();
		}
	}
	else
	{
		boxState = BOX_NOT_VISIBLE;
		lastIndex = INDEX_INVALID;
		lastIndex2 = INDEX_INVALID;
	}

	if(boxState == BOX_VISIBLE)
	{
		Int2 textSize = GameGui::font->CalculateSize(boxText);
		boxBig = Rect::Create(Int2(0, 0), textSize);
		Int2 size = textSize + Int2(24, 24);
		Int2 pos2 = Int2(gui->cursorPos) + Int2(24, 24);
		Int2 textPos(12, 12);

		// uwzglêdnij rozmiar obrazka
		if(boxImg)
		{
			textPos.x += boxImgSize.x + 8;
			size.x += boxImgSize.x + 8;
			if(size.y < boxImgSize.y + 24)
				size.y = boxImgSize.y + 24;
		}

		// minimalna szerokoœæ
		if(size.x < 256)
			size.x = 256;

		Int2 textPos2(12, textPos.y);
		textPos2.y += size.y - 12;

		if(!boxTextSmall.empty())
		{
			Int2 sizeSmall = GameGui::fontSmall->CalculateSize(boxTextSmall, size.x - 24);
			boxSmall = Rect::Create(Int2(0, 0), sizeSmall);
			int sizeY = sizeSmall.y;
			size.y += sizeY + 12;
		}

		if(pos2.x + size.x >= gui->wndSize.x)
			pos2.x = gui->wndSize.x - size.x - 1;
		if(pos2.y + size.y >= gui->wndSize.y)
			pos2.y = gui->wndSize.y - size.y - 1;

		boxImgPos = Int2(pos2.x + 12, pos2.y + 12);
		boxBig = Rect::Create(textPos + pos2, textSize);
		boxSmall += pos2 + textPos2;
		boxSmall.Right() = boxSmall.Left() + boxSize.x - 24;

		boxSize = size;
		boxPos = pos2;
	}
}

//=================================================================================================
// GAME PANEL CONTAINER
//=================================================================================================
GamePanelContainer::GamePanelContainer() : order(0), lostFocus(false)
{
}

//=================================================================================================
void GamePanelContainer::Draw()
{
	drawBox = nullptr;

	for(vector<Control*>::iterator it = ctrls.begin(), end = ctrls.end(); it != end; ++it)
	{
		if((*it)->visible)
			(*it)->Draw();
	}

	if(drawBox)
		drawBox->DrawBoxInternal();
}

//=================================================================================================
void GamePanelContainer::Update(float dt)
{
	if(focus)
	{
		if(lostFocus)
		{
			lostFocus = false;
			ctrls.back()->Event(GuiEvent_GainFocus);
			ctrls.back()->focus = true;
		}

		Int2 cp = gui->cursorPos;
		GamePanel* top = nullptr;

		for(vector<Control*>::iterator it = ctrls.begin(), end = ctrls.end(); it != end; ++it)
		{
			GamePanel* gp = (GamePanel*)*it;
			gp->mouseFocus = false;
			if(gp->IsInside(cp))
				top = gp;
		}

		if(top)
		{
			top->mouseFocus = true;
			if((input->Pressed(Key::LeftButton) || input->Pressed(Key::RightButton)) && top->order != order - 1)
			{
				ctrls.back()->Event(GuiEvent_LostFocus);
				ctrls.back()->focus = false;
				top->order = order;
				++order;
				std::sort(ctrls.begin(), ctrls.end(), [](const Control* c1, const Control* c2)
				{
					return static_cast<const GamePanel*>(c1)->order < static_cast<const GamePanel*>(c2)->order;
				});
				top->Event(GuiEvent_GainFocus);
				top->focus = true;
			}
		}

		for(vector<Control*>::iterator it = ctrls.begin(), end = ctrls.end(); it != end; ++it)
			(*it)->Update(dt);
	}
	else
	{
		if(!lostFocus)
		{
			lostFocus = true;
			ctrls.back()->Event(GuiEvent_LostFocus);
			ctrls.back()->focus = false;
		}

		for(vector<Control*>::iterator it = ctrls.begin(), end = ctrls.end(); it != end; ++it)
		{
			(*it)->mouseFocus = false;
			(*it)->Update(dt);
		}
	}
}

//=================================================================================================
void GamePanelContainer::Add(GamePanel* panel)
{
	Container::Add(panel);
	panel->order = order;
	++order;
}

//=================================================================================================
void GamePanelContainer::Show()
{
	visible = true;
	for(vector<Control*>::iterator it = ctrls.begin(), end = ctrls.end(); it != end; ++it)
		(*it)->Event(GuiEvent_Show);
	ctrls.back()->Event(GuiEvent_GainFocus);
	ctrls.back()->focus = true;
}

//=================================================================================================
void GamePanelContainer::Hide()
{
	visible = false;
	for(vector<Control*>::iterator it = ctrls.begin(), end = ctrls.end(); it != end; ++it)
	{
		if((*it)->focus)
		{
			(*it)->Event(GuiEvent_LostFocus);
			(*it)->focus = false;
		}
	}
}

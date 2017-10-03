#include "Pch.h"
#include "Core.h"
#include "Options.h"
#include "Language.h"
#include "KeyStates.h"
#include "Game.h"
#include "MenuList.h"

//-----------------------------------------------------------------------------
cstring txQuality, txMsNone;

//-----------------------------------------------------------------------------
class Res : public GuiElement
{
public:
	Int2 size;
	int hz;

	Res(const Int2& size, int hz) : size(size), hz(hz)
	{
	}

	cstring ToString()
	{
		return Format("%dx%d (%d Hz)", size.x, size.y, hz);
	}
};

//-----------------------------------------------------------------------------
inline bool ResPred(const Res* r1, const Res* r2)
{
	if(r1->size.x > r2->size.x)
		return false;
	else if(r1->size.x < r2->size.x)
		return true;
	else if(r1->size.y > r2->size.y)
		return false;
	else if(r1->size.y < r2->size.y)
		return true;
	else if(r1->hz > r2->hz)
		return false;
	else if(r1->hz < r2->hz)
		return true;
	else
		return false;
}

//-----------------------------------------------------------------------------
class MultisamplingItem : public GuiElement
{
public:
	int level, quality;

	MultisamplingItem(int level, int quality) : level(level), quality(quality)
	{
	}

	cstring ToString()
	{
		if(level == 0)
			return txMsNone;
		else
			return Format("x%d (%s %d)", level, txQuality, quality);
	}
};

//-----------------------------------------------------------------------------
class LanguageItem : public GuiElement
{
public:
	string text, id;

	LanguageItem(const string& id, const string& text) : id(id), text(text)
	{
	}

	cstring ToString()
	{
		return text.c_str();
	}
};

//=================================================================================================
Options::Options(const DialogInfo& info) : GameDialogBox(info)
{
	txOPTIONS = Str("OPTIONS");
	txResolution = Str("resolution");
	txMultisampling = Str("multisampling");
	txLanguage = Str("language");
	txMultisamplingError = Str("multisamplingError");
	txNeedRestart = Str("needRestart");
	txSoundVolume = Str("soundVolume");
	txMusicVolume = Str("musicVolume");
	txMouseSensitivity = Str("mouseSensitivity");
	txGrassRange = Str("grassRange");

	txQuality = Str("quality");
	txMsNone = Str("msNone");

	cstring ids[] = {
		"fullscreenMode",
		"glow",
		"normalMap",
		"specularMap",
		"vsync"
	};

	size = Int2(570, 460);
	bts.resize(2);

	Int2 offset(290, 60);

	for(int i = 0; i < 5; ++i)
	{
		check[i].id = IdFullscreen + i;
		check[i].parent = this;
		check[i].size = Int2(size.x - 44, 32);
		check[i].text = Str(ids[i]);
		check[i].pos = offset;
		offset.y += 32 + 10;
	}

	bts[0].id = IdOk;
	bts[0].parent = this;
	bts[0].text = Str("ok");
	bts[0].size = GUI.default_font->CalculateSize(bts[0].text) + Int2(24, 24);

	bts[1].id = IdControls;
	bts[1].parent = this;
	bts[1].text = Str("controls");
	bts[1].size = GUI.default_font->CalculateSize(bts[1].text) + Int2(24, 24);

	bts[0].size.x = bts[1].size.x = max(bts[0].size.x, bts[1].size.x);
	bts[0].pos = Int2(20, 410);
	bts[1].pos = Int2(bts[0].size.x + 40, 410);

	scroll[0].pos = Int2(290, 290);
	scroll[0].size = Int2(250, 16);
	scroll[0].total = 100;
	scroll[0].part = 10;
	scroll[0].offset = 0;
	scroll[0].hscrollbar = true;

	scroll[1].pos = Int2(290, 330);
	scroll[1].size = Int2(250, 16);
	scroll[1].total = 100;
	scroll[1].part = 10;
	scroll[1].offset = 0;
	scroll[1].hscrollbar = true;

	scroll[2].pos = Int2(290, 370);
	scroll[2].size = Int2(250, 16);
	scroll[2].total = 100;
	scroll[2].part = 10;
	scroll[2].offset = 0;
	scroll[2].hscrollbar = true;

	scroll[3].pos = Int2(290, 410);
	scroll[3].size = Int2(250, 16);
	scroll[3].total = 100;
	scroll[3].part = 10;
	scroll[3].offset = 0;
	scroll[3].hscrollbar = true;

	// lista rozdzielczoœci
	res.parent = this;
	res.pos = Int2(20, 80);
	res.size = Int2(250, 200);
	res.event_handler = DialogEvent(this, &Options::OnChangeRes);
	LocalVector<Res*> vres;
	uint display_modes = game->d3d->GetAdapterModeCount(game->used_adapter, DISPLAY_FORMAT);
	for(uint i = 0; i < display_modes; ++i)
	{
		D3DDISPLAYMODE d_mode;
		V(game->d3d->EnumAdapterModes(game->used_adapter, DISPLAY_FORMAT, i, &d_mode));
		if(d_mode.Width >= (uint)Engine::MIN_WINDOW_SIZE.x && d_mode.Height >= (uint)Engine::MIN_WINDOW_SIZE.y)
			vres->push_back(new Res(Int2(d_mode.Width, d_mode.Height), d_mode.RefreshRate));
	}
	// sortuj
	std::sort(vres->begin(), vres->end(), ResPred);
	// dodaj do listboxa i wybierz
	int index = 0;
	for(auto r : vres)
	{
		res.Add(r);
		if(r->size == game->GetWindowSize() && r->hz == game->wnd_hz)
			res.SetIndex(index);
	}
	res.Initialize();
	res.ScrollTo(res.GetIndex(), true);

	// multisampling
	multisampling.SetCollapsed(true);
	multisampling.parent = this;
	multisampling.pos = Int2(20, 327);
	multisampling.size = Int2(250, 25);
	multisampling.event_handler = DialogEvent(this, &Options::OnChangeMultisampling);
	multisampling.Add(new MultisamplingItem(0, 0));
	int ms, msq;
	game->GetMultisampling(ms, msq);
	if(ms == 0)
		multisampling.SetIndex(0);
	index = 1;
	for(int j = 2; j <= 16; ++j)
	{
		DWORD levels, levels2;
		if(SUCCEEDED(game->d3d->CheckDeviceMultiSampleType(game->used_adapter, D3DDEVTYPE_HAL, BACKBUFFER_FORMAT, FALSE, (D3DMULTISAMPLE_TYPE)j, &levels)) &&
			SUCCEEDED(game->d3d->CheckDeviceMultiSampleType(game->used_adapter, D3DDEVTYPE_HAL, ZBUFFER_FORMAT, FALSE, (D3DMULTISAMPLE_TYPE)j, &levels2)))
		{
			int level = min(levels, levels2);
			for(int i = 0; i < level; ++i)
			{
				multisampling.Add(new MultisamplingItem(j, i));
				if(ms == j && msq == i)
					multisampling.SetIndex(index);
			}
			++index;
		}
	}
	multisampling.Initialize();

	// jêzyk
	language.SetCollapsed(true);
	language.parent = this;
	language.pos = Int2(20, 383);
	language.size = Int2(250, 25);
	language.event_handler = DialogEvent(this, &Options::OnChangeLanguage);
	index = 0;
	for(vector<LanguageMap*>::iterator it = g_languages.begin(), end = g_languages.end(); it != end; ++it, ++index)
	{
		LanguageMap& lmap = **it;
		string& dir = lmap["dir"];
		language.Add(new LanguageItem(dir, Format("%s, %s, %s", dir.c_str(), lmap["englishName"].c_str(), lmap["localName"].c_str())));
		if(dir == g_lang_prefix)
			language.SetIndex(index);
	}
	language.Initialize();

	visible = false;
}

//=================================================================================================
void Options::Draw(ControlDrawData* /*cdd*/)
{
	// t³o
	GUI.DrawSpriteFull(tBackground, COLOR_RGBA(255, 255, 255, 128));

	// panel
	GUI.DrawItem(tDialog, global_pos, size, COLOR_RGBA(255, 255, 255, 222), 16);

	// checkboxy
	for(int i = 0; i < 5; ++i)
		check[i].Draw();

	// scrollbary
	for(int i = 0; i < 4; ++i)
		scroll[i].Draw();

	// przyciski
	for(int i = 0; i < 2; ++i)
		bts[i].Draw();

	// tekst OPCJE
	Rect r = { global_pos.x, global_pos.y + 8, global_pos.x + size.x, global_pos.y + size.y };
	GUI.DrawText(GUI.fBig, txOPTIONS, DT_TOP | DT_CENTER, BLACK, r);

	// tekst Rozdzielczoœæ:
	Rect r2 = { global_pos.x + 10, global_pos.y + 50, global_pos.x + size.x, global_pos.y + 75 };
	GUI.DrawText(GUI.default_font, txResolution, DT_SINGLELINE, BLACK, r2);
	// Multisampling:
	r2.Top() = global_pos.y + 300;
	r2.Bottom() = r2.Top() + 20;
	GUI.DrawText(GUI.default_font, txMultisampling, DT_SINGLELINE, BLACK, r2);
	// Jêzyk:
	r2.Top() = global_pos.y + 360;
	r2.Bottom() = r2.Top() + 20;
	GUI.DrawText(GUI.default_font, txLanguage, DT_SINGLELINE, BLACK, r2);
	// G³oœnoœæ dŸwiêku (0)
	r2.Left() = global_pos.x + 290;
	r2.Top() = global_pos.y + 270;
	r2.Bottom() = r2.Top() + 20;
	GUI.DrawText(GUI.default_font, Format("%s (%d)", txSoundVolume, sound_volume), DT_SINGLELINE, BLACK, r2);
	// G³oœnoœæ muzyki (0)
	r2.Top() = global_pos.y + 310;
	r2.Bottom() = r2.Top() + 20;
	GUI.DrawText(GUI.default_font, Format("%s (%d)", txMusicVolume, music_volume), DT_SINGLELINE, BLACK, r2);
	// Czu³oœæ myszki (0)
	r2.Top() = global_pos.y + 350;
	r2.Bottom() = r2.Top() + 20;
	GUI.DrawText(GUI.default_font, Format("%s (%d)", txMouseSensitivity, mouse_sensitivity), DT_SINGLELINE, BLACK, r2);
	// Zasiêg trawy (0)
	r2.Top() = global_pos.y + 390;
	r2.Bottom() = r2.Top() + 20;
	GUI.DrawText(GUI.default_font, Format("%s (%d)", txGrassRange, grass_range), DT_SINGLELINE, BLACK, r2);

	// listbox z rozdzielczoœciami
	res.Draw();
	multisampling.Draw();
	language.Draw();
	if(multisampling.menu->visible)
		multisampling.menu->Draw();
	if(language.menu->visible)
		language.menu->Draw();
}

//=================================================================================================
void Options::Update(float dt)
{
	SetOptions();

	// aktualizuj kontrolki
	if(multisampling.menu->visible)
		multisampling.menu->Update(dt);
	if(language.menu->visible)
		language.menu->Update(dt);
	for(int i = 0; i < 5; ++i)
	{
		check[i].mouse_focus = focus;
		check[i].Update(dt);
	}
	for(int i = 0; i < 4; ++i)
	{
		scroll[i].mouse_focus = focus;
		float prev_offset = scroll[i].offset;
		scroll[i].Update(dt);
		if(prev_offset != scroll[i].offset)
		{
			int value = int(scroll[i].GetValue() * 100);
			if(i == 0)
				sound_volume = value;
			else if(i == 1)
				music_volume = value;
			else if(i == 2)
				mouse_sensitivity = value;
			else
				grass_range = value;
			event(IdSoundVolume + i);
		}
	}
	for(int i = 0; i < 2; ++i)
	{
		bts[i].mouse_focus = focus;
		bts[i].Update(dt);
	}
	res.mouse_focus = focus;
	res.Update(dt);
	multisampling.mouse_focus = focus;
	multisampling.Update(dt);
	language.mouse_focus = focus;
	language.Update(dt);

	if(focus && Key.Focus() && Key.PressedRelease(VK_ESCAPE))
		Event((GuiEvent)IdOk);
}

//=================================================================================================
void Options::Event(GuiEvent e)
{
	if(e == GuiEvent_Show || e == GuiEvent_WindowResize)
	{
		if(e == GuiEvent_Show)
		{
			visible = true;
			SetOptions();
		}
		pos = global_pos = (GUI.wnd_size - size) / 2;
		for(int i = 0; i < 5; ++i)
			check[i].global_pos = global_pos + check[i].pos;
		for(int i = 0; i < 4; ++i)
			scroll[i].global_pos = global_pos + scroll[i].pos;
		for(int i = 0; i < 2; ++i)
			bts[i].global_pos = global_pos + bts[i].pos;
		res.Event(GuiEvent_Moved);
		multisampling.Event(GuiEvent_Moved);
		language.Event(GuiEvent_Moved);
	}
	else if(e == GuiEvent_Close)
	{
		res.Event(GuiEvent_LostFocus);
		visible = false;
	}
	else if(e == GuiEvent_LostFocus)
		res.Event(GuiEvent_LostFocus);
	else if(e >= GuiEvent_Custom)
		event((Id)e);
}

//=================================================================================================
void Options::SetOptions()
{
	check[0].checked = game->IsFullscreen();
	check[1].checked = game->cl_glow;
	check[2].checked = game->cl_normalmap;
	check[3].checked = game->cl_specularmap;
	check[4].checked = game->GetVsync();

	Res& re = *res.GetItemCast<Res>();
	if(re.size != game->GetWindowSize() || re.hz != game->wnd_hz)
	{
		auto& ress = res.GetItemsCast<Res>();
		int index = 0;
		for(auto r : ress)
		{
			if(r->size == game->GetWindowSize() && r->hz == game->wnd_hz)
			{
				res.SetIndex(index);
				break;
			}
			++index;
		}
	}

	MultisamplingItem& mi = *multisampling.GetItemCast<MultisamplingItem>();
	int ms, msq;
	game->GetMultisampling(ms, msq);
	if(mi.level != ms || mi.quality != msq)
	{
		auto& multis = multisampling.GetItemsCast<MultisamplingItem>();
		int index = 0;
		for(vector<MultisamplingItem*>::iterator it = multis.begin(), end = multis.end(); it != end; ++it, ++index)
		{
			if((*it)->level == ms && (*it)->quality == msq)
			{
				multisampling.SetIndex(index);
				break;
			}
		}
	}

	if(sound_volume != game->sound_volume)
	{
		sound_volume = game->sound_volume;
		scroll[0].SetValue(float(sound_volume) / 100.f);
	}
	if(music_volume != game->music_volume)
	{
		music_volume = game->music_volume;
		scroll[1].SetValue(float(music_volume) / 100.f);
	}
	if(mouse_sensitivity != game->mouse_sensitivity)
	{
		mouse_sensitivity = game->mouse_sensitivity;
		scroll[2].SetValue(float(mouse_sensitivity) / 100.f);
	}
	if(grass_range != game->grass_range)
	{
		grass_range = (int)game->grass_range;
		scroll[3].SetValue(float(grass_range) / 100.f);
	}
}

//=================================================================================================
void Options::OnChangeRes(int)
{
	Res& r = *res.GetItemCast<Res>();
	game->ChangeMode(r.size, game->IsFullscreen(), r.hz);
	event(IdChangeRes);
}

//=================================================================================================
void Options::OnChangeMultisampling(int id)
{
	MultisamplingItem& multi = *multisampling.GetItemCast<MultisamplingItem>();
	if(game->ChangeMultisampling(multi.level, multi.quality) == 0)
		GUI.SimpleDialog(txMultisamplingError, this);
}

//=================================================================================================
void Options::OnChangeLanguage(int id)
{
	language_id = language.GetItemCast<LanguageItem>()->id;

	DialogInfo info;
	info.event = DialogEvent(this, &Options::ChangeLanguage);
	info.order = ORDER_TOP;
	info.parent = this;
	info.pause = false;
	info.text = txNeedRestart;
	info.type = DIALOG_YESNO;
	GUI.ShowDialog(info);
}

//=================================================================================================
void Options::ChangeLanguage(int id)
{
	if(id == BUTTON_YES)
	{
		g_lang_prefix = language_id;
		game->SaveOptions();
		game->RestartGame();
	}
	else
	{
		// przywróc zaznaczenie
		vector<LanguageItem*>& langs = language.GetItemsCast<LanguageItem>();
		int index = 0;
		for(vector<LanguageItem*>::iterator it = langs.begin(), end = langs.end(); it != end; ++it, ++index)
		{
			if((*it)->id == g_lang_prefix)
			{
				language.SetIndex(index);
				break;
			}
		}
	}
}

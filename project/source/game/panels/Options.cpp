#include "Pch.h"
#include "GameCore.h"
#include "Options.h"
#include "Language.h"
#include "KeyStates.h"
#include "GlobalGui.h"
#include "Game.h"
#include "MenuList.h"
#include "SoundManager.h"

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
	size = Int2(570, 460);
	bts.resize(2);

	Int2 offset(290, 60);

	for(int i = 0; i < 5; ++i)
	{
		check[i].id = IdFullscreen + i;
		check[i].parent = this;
		check[i].size = Int2(size.x - 44, 32);
		check[i].pos = offset;
		offset.y += 32 + 10;
	}
	
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

	// j�zyk
	language.SetCollapsed(true);
	language.parent = this;
	language.pos = Int2(20, 383);
	language.size = Int2(250, 25);
	language.event_handler = DialogEvent(this, &Options::OnChangeLanguage);
	int index = 0;
	for(Language::LanguageMap* p_lmap : Language::GetLanguages())
	{
		Language::LanguageMap& lmap = *p_lmap;
		string& dir = lmap["dir"];
		language.Add(new LanguageItem(dir, Format("%s, %s, %s", dir.c_str(), lmap["englishName"].c_str(), lmap["localName"].c_str())));
		if(dir == Language::prefix)
			language.SetIndex(index);
		++index;
	}
	language.Initialize();

	visible = false;
}

//=================================================================================================
void Options::LoadLanguage()
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

	check[0].text = Str("fullscreenMode");
	check[1].text = Str("glow");
	check[2].text = Str("normalMap");
	check[3].text = Str("specularMap");
	check[4].text = Str("vsync");

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

	// lista rozdzielczo�ci
	res.parent = this;
	res.pos = Int2(20, 80);
	res.size = Int2(250, 200);
	res.event_handler = DialogEvent(this, &Options::OnChangeRes);
	vector<Resolution> resolutions;
	game->GetResolutions(resolutions);
	LocalVector<Res*> vres;
	for(Resolution& r : resolutions)
		vres->push_back(new Res(r.size, r.hz));
	std::sort(vres->begin(), vres->end(), ResPred);
	int index = 0;
	for(auto r : vres)
	{
		res.Add(r);
		if(r->size == game->GetWindowSize() && r->hz == game->wnd_hz)
			res.SetIndex(index);
		++index;
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
	vector<Int2> ms_modes;
	game->GetMultisamplingModes(ms_modes);
	index = 1;
	for(Int2& mode : ms_modes)
	{
		multisampling.Add(new MultisamplingItem(mode.x, mode.y));
		if(ms == mode.x && msq == mode.y)
			multisampling.SetIndex(index);
		++index;
	}
	multisampling.Initialize();
}

//=================================================================================================
void Options::Draw(ControlDrawData* /*cdd*/)
{
	// t�o
	GUI.DrawSpriteFull(tBackground, Color::Alpha(128));

	// panel
	GUI.DrawItem(tDialog, global_pos, size, Color::Alpha(222), 16);

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
	GUI.DrawText(GUI.fBig, txOPTIONS, DTF_TOP | DTF_CENTER, Color::Black, r);

	// tekst Rozdzielczo��:
	Rect r2 = { global_pos.x + 10, global_pos.y + 50, global_pos.x + size.x, global_pos.y + 75 };
	GUI.DrawText(GUI.default_font, txResolution, DTF_SINGLELINE, Color::Black, r2);
	// Multisampling:
	r2.Top() = global_pos.y + 300;
	r2.Bottom() = r2.Top() + 20;
	GUI.DrawText(GUI.default_font, txMultisampling, DTF_SINGLELINE, Color::Black, r2);
	// J�zyk:
	r2.Top() = global_pos.y + 360;
	r2.Bottom() = r2.Top() + 20;
	GUI.DrawText(GUI.default_font, txLanguage, DTF_SINGLELINE, Color::Black, r2);
	// G�o�no�� d�wi�ku (0)
	r2.Left() = global_pos.x + 290;
	r2.Top() = global_pos.y + 270;
	r2.Bottom() = r2.Top() + 20;
	GUI.DrawText(GUI.default_font, Format("%s (%d)", txSoundVolume, sound_volume), DTF_SINGLELINE, Color::Black, r2);
	// G�o�no�� muzyki (0)
	r2.Top() = global_pos.y + 310;
	r2.Bottom() = r2.Top() + 20;
	GUI.DrawText(GUI.default_font, Format("%s (%d)", txMusicVolume, music_volume), DTF_SINGLELINE, Color::Black, r2);
	// Czu�o�� myszki (0)
	r2.Top() = global_pos.y + 350;
	r2.Bottom() = r2.Top() + 20;
	GUI.DrawText(GUI.default_font, Format("%s (%d)", txMouseSensitivity, mouse_sensitivity), DTF_SINGLELINE, Color::Black, r2);
	// Zasi�g trawy (0)
	r2.Top() = global_pos.y + 390;
	r2.Bottom() = r2.Top() + 20;
	GUI.DrawText(GUI.default_font, Format("%s (%d)", txGrassRange, grass_range), DTF_SINGLELINE, Color::Black, r2);

	// listbox z rozdzielczo�ciami
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
			Event((GuiEvent)(IdSoundVolume + i));
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
	{
		switch((Id)e)
		{
		case IdOk:
			CloseDialog();
			game->SaveOptions();
			break;
		case IdFullscreen:
			game->ChangeMode(check[0].checked);
			break;
		case IdChangeRes:
			break;
		case IdSoundVolume:
			game->sound_mgr->SetSoundVolume(sound_volume);
			break;
		case IdMusicVolume:
			game->sound_mgr->SetMusicVolume(music_volume);
			break;
		case IdMouseSensitivity:
			game->settings.mouse_sensitivity = mouse_sensitivity;
			game->settings.mouse_sensitivity_f = Lerp(0.5f, 1.5f, float(game->settings.mouse_sensitivity) / 100);
			break;
		case IdGrassRange:
			game->grass_range = (float)game->grass_range;
			break;
		case IdControls:
			GUI.ShowDialog((DialogBox*)game->gui->controls);
			break;
		case IdGlow:
			game->cl_glow = check[1].checked;
			break;
		case IdNormal:
			game->cl_normalmap = check[2].checked;
			break;
		case IdSpecular:
			game->cl_specularmap = check[3].checked;
			break;
		case IdVsync:
			game->SetVsync(!game->GetVsync());
			break;
		}
	}
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

	SoundManager* sound_mgr = game->sound_mgr.get();
	if(sound_volume != sound_mgr->GetSoundVolume())
	{
		sound_volume = sound_mgr->GetSoundVolume();
		scroll[0].SetValue(float(sound_volume) / 100.f);
	}
	if(music_volume != sound_mgr->GetMusicVolume())
	{
		music_volume = sound_mgr->GetMusicVolume();
		scroll[1].SetValue(float(music_volume) / 100.f);
	}
	if(mouse_sensitivity != game->settings.mouse_sensitivity)
	{
		mouse_sensitivity = game->settings.mouse_sensitivity;
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
	Event((GuiEvent)IdChangeRes);
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
		Language::prefix = language_id;
		game->SaveOptions();
		game->RestartGame();
	}
	else
	{
		// przywr�c zaznaczenie
		vector<LanguageItem*>& langs = language.GetItemsCast<LanguageItem>();
		int index = 0;
		for(vector<LanguageItem*>::iterator it = langs.begin(), end = langs.end(); it != end; ++it, ++index)
		{
			if((*it)->id == Language::prefix)
			{
				language.SetIndex(index);
				break;
			}
		}
	}
}

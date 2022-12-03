#include "Pch.h"
#include "Options.h"

#include "Game.h"
#include "GameGui.h"
#include "Language.h"

#include <Engine.h>
#include <Input.h>
#include <MenuList.h>
#include <Render.h>
#include <SceneManager.h>
#include <SoundManager.h>

//-----------------------------------------------------------------------------
cstring txQuality, txMsNone;

//-----------------------------------------------------------------------------
class ResolutionItem : public GuiElement
{
public:
	Resolution resolution;

	explicit ResolutionItem(const Resolution& resolution) : resolution(resolution) {}
	cstring ToString() override
	{
		return Format("%dx%d", resolution.size.x, resolution.size.y);
	}
};

//-----------------------------------------------------------------------------
class MultisamplingItem : public GuiElement
{
public:
	int level, quality;

	MultisamplingItem(int level, int quality) : level(level), quality(quality) {}
	cstring ToString() override
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

	LanguageItem(const string& id, const string& text) : id(id), text(text) {}
	cstring ToString() override { return text.c_str(); }
};

//=================================================================================================
class SoundDeviceItem : public GuiElement
{
public:
	string name;
	Guid device;

	SoundDeviceItem(const string& name, const Guid& device) : name(name), device(device) {}
	cstring ToString() override { return name.c_str(); }
};

//=================================================================================================
Options::Options(const DialogInfo& info) : DialogBox(info)
{
	size = Int2(560, 495);
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

	language.SetCollapsed(true);
	language.parent = this;
	language.pos = Int2(20, 369);
	language.size = Int2(250, 25);
	language.eventHandler = DialogEvent(this, &Options::OnChangeLanguage);
	int index = 0;
	for(Language::Map* p_lmap : Language::GetLanguages())
	{
		Language::Map& lmap = *p_lmap;
		string& dir = lmap["dir"];
		language.Add(new LanguageItem(dir, Format("%s, %s, %s", dir.c_str(), lmap["englishName"].c_str(), lmap["localName"].c_str())));
		if(dir == Language::prefix)
			language.SetIndex(index);
		++index;
	}
	language.Initialize();

	// sound drivers
	soundDevice.SetCollapsed(true);
	soundDevice.parent = this;
	soundDevice.pos = Int2(20, 426);
	soundDevice.size = Int2(250, 25);
	soundDevice.eventHandler = DialogEvent(this, &Options::OnChangeSoundDevice);
	soundDevice.Initialize();

	visible = false;
}

//=================================================================================================
void Options::LoadLanguage()
{
	Language::Section s = Language::GetSection("Options");

	txTitle = s.Get("title");
	txResolution = s.Get("resolution");
	txMultisampling = s.Get("multisampling");
	txLanguage = s.Get("language");
	txMultisamplingError = s.Get("multisamplingError");
	txNeedRestart = s.Get("needRestart");
	txSoundVolume = s.Get("soundVolume");
	txMusicVolume = s.Get("musicVolume");
	txMouseSensitivity = s.Get("mouseSensitivity");
	txGrassRange = s.Get("grassRange");
	txSoundDevice = s.Get("soundDevice");
	txDefaultDevice = s.Get("defaultDevice");

	txQuality = s.Get("quality");
	txMsNone = s.Get("msNone");

	check[0].text = s.Get("fullscreenMode");
	check[1].text = s.Get("glow");
	check[2].text = s.Get("normalMap");
	check[3].text = s.Get("specularMap");
	check[4].text = s.Get("vsync");

	bts[0].id = IdOk;
	bts[0].parent = this;
	bts[0].text = Str("ok");
	bts[0].size = GameGui::font->CalculateSize(bts[0].text) + Int2(24, 24);
	bts[1].id = IdControls;
	bts[1].parent = this;
	bts[1].text = s.Get("controls");
	bts[1].size = GameGui::font->CalculateSize(bts[1].text) + Int2(24, 24);
	bts[0].size.x = bts[1].size.x = max(bts[0].size.x, bts[1].size.x);
	bts[0].pos = Int2(290, 432);
	bts[1].pos = Int2(bts[0].size.x + 320, 432);

	// resolutions list
	res.parent = this;
	res.pos = Int2(20, 80);
	res.size = Int2(250, 200);
	res.eventHandler = DialogEvent(this, &Options::OnChangeRes);
	const vector<Resolution>& resolutions = render->GetResolutions();
	LocalVector<ResolutionItem*> items;
	for(const Resolution& r : resolutions)
		items->push_back(new ResolutionItem(r));
	std::sort(items->begin(), items->end(), [](const ResolutionItem* r1, const ResolutionItem* r2)
	{
		return r1->resolution < r2->resolution;
	});
	int index = 0;
	for(ResolutionItem* item : items)
	{
		res.Add(item);
		if(item->resolution.size == engine->GetWindowSize())
			res.SetIndex(index);
		++index;
	}
	res.Initialize();
	res.ScrollTo(res.GetIndex(), true);

	// multisampling
	multisampling.SetCollapsed(true);
	multisampling.parent = this;
	multisampling.pos = Int2(20, 312);
	multisampling.size = Int2(250, 25);
	multisampling.eventHandler = DialogEvent(this, &Options::OnChangeMultisampling);
	multisampling.Add(new MultisamplingItem(0, 0));
	int ms, msq;
	render->GetMultisampling(ms, msq);
	if(ms == 0)
		multisampling.SetIndex(0);
	const vector<Int2>& ms_modes = render->GetMultisamplingModes();
	index = 1;
	for(const Int2& mode : ms_modes)
	{
		for(int level = 0; level < mode.y; ++level)
		{
			multisampling.Add(new MultisamplingItem(mode.x, level));
			if(ms == mode.x && msq == level)
				multisampling.SetIndex(index);
			++index;
		}
	}
	multisampling.Initialize();
}

//=================================================================================================
void Options::Draw()
{
	DrawPanel();

	// title
	Rect r = { globalPos.x, globalPos.y + 8, globalPos.x + size.x, globalPos.y + size.y };
	gui->DrawText(GameGui::fontBig, txTitle, DTF_TOP | DTF_CENTER, Color::Black, r);

	// controls
	for(int i = 0; i < 5; ++i)
		check[i].Draw();
	for(int i = 0; i < 4; ++i)
		scroll[i].Draw();
	for(int i = 0; i < 2; ++i)
		bts[i].Draw();

	//------ Left part
	// Resolution:
	Rect r2 = { globalPos.x + 20, globalPos.y + 60, globalPos.x + size.x, globalPos.y + 80 };
	gui->DrawText(GameGui::font, txResolution, DTF_SINGLELINE, Color::Black, r2);
	// Multisampling:
	r2.Top() = globalPos.y + 292;
	r2.Bottom() = r2.Top() + 20;
	gui->DrawText(GameGui::font, txMultisampling, DTF_SINGLELINE, Color::Black, r2);
	// Language:
	r2.Top() = globalPos.y + 349;
	r2.Bottom() = r2.Top() + 20;
	gui->DrawText(GameGui::font, txLanguage, DTF_SINGLELINE, Color::Black, r2);
	// Sound device:
	r2.Top() = globalPos.y + 406;
	r2.Bottom() = r2.Top() + 20;
	gui->DrawText(GameGui::font, txSoundDevice, DTF_SINGLELINE, Color::Black, r2);

	//------ Right part
	// Sound volume (0)
	r2.Left() = globalPos.x + 290;
	r2.Top() = globalPos.y + 270;
	r2.Bottom() = r2.Top() + 20;
	gui->DrawText(GameGui::font, Format("%s (%d)", txSoundVolume, soundVolume), DTF_SINGLELINE, Color::Black, r2);
	// Music volume (0)
	r2.Top() = globalPos.y + 310;
	r2.Bottom() = r2.Top() + 20;
	gui->DrawText(GameGui::font, Format("%s (%d)", txMusicVolume, musicVolume), DTF_SINGLELINE, Color::Black, r2);
	// Mouse sensitivity (0)
	r2.Top() = globalPos.y + 350;
	r2.Bottom() = r2.Top() + 20;
	gui->DrawText(GameGui::font, Format("%s (%d)", txMouseSensitivity, mouseSensitivity), DTF_SINGLELINE, Color::Black, r2);
	// Grass range (0)
	r2.Top() = globalPos.y + 390;
	r2.Bottom() = r2.Top() + 20;
	gui->DrawText(GameGui::font, Format("%s (%d)", txGrassRange, grassRange), DTF_SINGLELINE, Color::Black, r2);

	// listboxes
	res.Draw();
	multisampling.Draw();
	language.Draw();
	soundDevice.Draw();
	if(multisampling.menu->visible)
		multisampling.menu->Draw();
	if(language.menu->visible)
		language.menu->Draw();
	if(soundDevice.menu->visible)
		soundDevice.menu->Draw();
}

//=================================================================================================
void Options::Update(float dt)
{
	SetOptions();

	if(multisampling.menu->visible)
		multisampling.menu->Update(dt);
	if(language.menu->visible)
		language.menu->Update(dt);
	if(soundDevice.menu->visible)
		soundDevice.menu->Update(dt);
	for(int i = 0; i < 5; ++i)
	{
		check[i].mouseFocus = focus;
		check[i].Update(dt);
	}
	for(int i = 0; i < 4; ++i)
	{
		scroll[i].mouseFocus = focus;
		float prev_offset = scroll[i].offset;
		scroll[i].Update(dt);
		if(prev_offset != scroll[i].offset)
		{
			int value = int(scroll[i].GetValue() * 100);
			if(i == 0)
				soundVolume = value;
			else if(i == 1)
				musicVolume = value;
			else if(i == 2)
				mouseSensitivity = value;
			else
				grassRange = value;
			Event((GuiEvent)(IdSoundVolume + i));
		}
	}
	for(int i = 0; i < 2; ++i)
	{
		bts[i].mouseFocus = focus;
		bts[i].Update(dt);
	}
	res.mouseFocus = focus;
	res.Update(dt);
	multisampling.mouseFocus = focus;
	multisampling.Update(dt);
	language.mouseFocus = focus;
	language.Update(dt);
	soundDevice.mouseFocus = focus;
	soundDevice.Update(dt);

	if(focus && input->Focus() && input->PressedRelease(Key::Escape))
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
			SetSoundDevices();
		}
		pos = globalPos = (gui->wndSize - size) / 2;
		for(int i = 0; i < 5; ++i)
			check[i].globalPos = globalPos + check[i].pos;
		for(int i = 0; i < 4; ++i)
			scroll[i].globalPos = globalPos + scroll[i].pos;
		for(int i = 0; i < 2; ++i)
			bts[i].globalPos = globalPos + bts[i].pos;
		res.Event(GuiEvent_Moved);
		multisampling.Event(GuiEvent_Moved);
		language.Event(GuiEvent_Moved);
		soundDevice.Event(GuiEvent_Moved);
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
			engine->SetFullscreen(check[0].checked);
			break;
		case IdChangeRes:
			break;
		case IdSoundVolume:
			soundMgr->SetSoundVolume(soundVolume);
			break;
		case IdMusicVolume:
			soundMgr->SetMusicVolume(musicVolume);
			break;
		case IdMouseSensitivity:
			game->settings.mouseSensitivity = mouseSensitivity;
			break;
		case IdGrassRange:
			game->settings.grassRange = (float)grassRange;
			break;
		case IdControls:
			gui->ShowDialog((DialogBox*)game_gui->controls);
			break;
		case IdGlow:
			game->useGlow = check[1].checked;
			break;
		case IdNormal:
			sceneMgr->useNormalmap = check[2].checked;
			break;
		case IdSpecular:
			sceneMgr->useSpecularmap = check[3].checked;
			break;
		case IdVsync:
			render->SetVsync(!render->IsVsyncEnabled());
			break;
		}
	}
}

//=================================================================================================
void Options::SetOptions()
{
	check[0].checked = engine->IsFullscreen();
	check[1].checked = game->useGlow;
	check[2].checked = sceneMgr->useNormalmap;
	check[3].checked = sceneMgr->useSpecularmap;
	check[4].checked = render->IsVsyncEnabled();

	ResolutionItem& currentItem = *res.GetItemCast<ResolutionItem>();
	const Int2& wndSize = engine->GetWindowSize();
	if(currentItem.resolution.size != wndSize)
	{
		int index = 0;
		for(ResolutionItem* item : res.GetItemsCast<ResolutionItem>())
		{
			if(item->resolution.size == wndSize)
			{
				res.SetIndex(index);
				break;
			}
			++index;
		}
	}

	MultisamplingItem& mi = *multisampling.GetItemCast<MultisamplingItem>();
	int ms, msq;
	render->GetMultisampling(ms, msq);
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

	if(soundVolume != soundMgr->GetSoundVolume())
	{
		soundVolume = soundMgr->GetSoundVolume();
		scroll[0].SetValue(float(soundVolume) / 100.f);
	}
	if(musicVolume != soundMgr->GetMusicVolume())
	{
		musicVolume = soundMgr->GetMusicVolume();
		scroll[1].SetValue(float(musicVolume) / 100.f);
	}
	if(mouseSensitivity != game->settings.mouseSensitivity)
	{
		mouseSensitivity = game->settings.mouseSensitivity;
		scroll[2].SetValue(float(mouseSensitivity) / 100.f);
	}
	if(grassRange != game->settings.grassRange)
	{
		grassRange = (int)game->settings.grassRange;
		scroll[3].SetValue(float(grassRange) / 100.f);
	}
}

//=================================================================================================
void Options::SetSoundDevices()
{
	Guid currentDevice;
	vector<pair<Guid, string>> devices;
	currentDevice = soundMgr->GetDevice();
	soundMgr->GetDevices(devices);
	soundDevice.Reset();
	soundDevice.Add(new SoundDeviceItem(txDefaultDevice, Guid::Empty));
	if(currentDevice == Guid::Empty)
		soundDevice.SetIndex(0);
	int index = 1;
	for(pair<Guid, string>& p : devices)
	{
		soundDevice.Add(new SoundDeviceItem(p.second, p.first));
		if(p.first == currentDevice)
			soundDevice.SetIndex(index);
		++index;
	}
}

//=================================================================================================
void Options::OnChangeRes(int)
{
	ResolutionItem& item = *res.GetItemCast<ResolutionItem>();
	engine->SetWindowSize(item.resolution.size);
	Event((GuiEvent)IdChangeRes);
}

//=================================================================================================
void Options::OnChangeMultisampling(int id)
{
	MultisamplingItem& multi = *multisampling.GetItemCast<MultisamplingItem>();
	if(render->SetMultisampling(multi.level, multi.quality) == 0)
		gui->SimpleDialog(txMultisamplingError, this);
}

//=================================================================================================
void Options::OnChangeLanguage(int id)
{
	languageId = language.GetItemCast<LanguageItem>()->id;

	DialogInfo info;
	info.event = DialogEvent(this, &Options::ChangeLanguage);
	info.order = DialogOrder::Top;
	info.parent = this;
	info.pause = false;
	info.text = txNeedRestart;
	info.type = DIALOG_YESNO;
	gui->ShowDialog(info);
}

//=================================================================================================
void Options::ChangeLanguage(int id)
{
	if(id == BUTTON_YES)
	{
		Language::prefix = languageId;
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
			if((*it)->id == Language::prefix)
			{
				language.SetIndex(index);
				break;
			}
		}
	}
}

//=================================================================================================
void Options::OnChangeSoundDevice(int id)
{
	SoundDeviceItem* item = soundDevice.GetItemCast<SoundDeviceItem>();
	soundMgr->SetDevice(item->device);
}

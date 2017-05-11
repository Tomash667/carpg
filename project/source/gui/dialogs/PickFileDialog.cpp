#include "Pch.h"
#include "Base.h"
#include "Button.h"
#include "KeyStates.h"
#include "ListBox.h"
#include "Overlay.h"
#include "PickFileDialog.h"
#include "ResourceManager.h"
#include "TextBox.h"

using namespace gui;

namespace gui
{
	class PickFileDialogItem : public GuiElement
	{
	public:
		cstring ToString()
		{
			return filename.c_str();
		}

		string filename, path;
		bool is_dir;
	};
}

bool PickFileDialogItemSort(const PickFileDialogItem* i1, const PickFileDialogItem* i2)
{
	if(i1->is_dir == i2->is_dir)
	{
		int r = _stricmp(i1->filename.c_str(), i2->filename.c_str());
		if(r == 0)
			return i1->filename < i2->filename;
		else
			return r < 0;
	}
	else
		return i1->is_dir;
}

PickFileDialog* PickFileDialog::self;

PickFileDialog::PickFileDialog()
{
	SetAreaSize(INT2(640, 480));

	list_box = new ListBox(true);
	list_box->event_handler2 = ListBox::Handler(this, &PickFileDialog::HandleListBoxEvent);
	list_box->SetSize(INT2(640 - 4, 480 - 100));
	list_box->SetPosition(INT2(2, 34));
	Add(list_box);

	list_extensions = new ListBox(true);
	list_extensions->event_handler2 = ListBox::Handler(this, &PickFileDialog::HandleChangeExtension);
	list_extensions->SetSize(INT2(640-406, 30));
	list_extensions->SetPosition(INT2(404, 480 - 64));
	Add(list_extensions);

	tb_path = new TextBox(false, true);
	tb_path->SetReadonly(true);
	tb_path->SetSize(INT2(640 - 4, 30));
	tb_path->SetPosition(INT2(2, 2));
	Add(tb_path);

	tb_filename = new TextBox(false, true);
	tb_filename->SetSize(INT2(400, 30));
	tb_filename->SetPosition(INT2(2, 480 - 64));
	Add(tb_filename);

	bt_select = new Button;
	bt_select->id = SelectItem;
	bt_select->text = "Open";
	bt_select->SetSize(INT2(100, 30));
	bt_select->SetPosition(INT2(640 - 212, 480 - 32));
	Add(bt_select);

	bt_cancel = new Button;
	bt_cancel->id = Cancel;
	bt_cancel->text = "Cancel";
	bt_cancel->SetSize(INT2(100, 30));
	bt_cancel->SetPosition(INT2(640 - 102, 480 - 32));
	Add(bt_cancel);

	ResourceManager::Get().GetLoadedTexture("dir.png", tex_dir);
}

PickFileDialog::~PickFileDialog()
{
}

void PickFileDialog::Destroy()
{
	delete self;
}

void PickFileDialog::Show(const PickFileDialogOptions& options)
{
	if(!self)
		self = new PickFileDialog;
	self->Setup(options);
	GUI.GetOverlay()->ShowDialog(self);
}

void PickFileDialog::Setup(const PickFileDialogOptions& options)
{
	SetText(options.title);
	root_dir = options.root_dir;
	handler = options.handler;
	active_dir = root_dir;
	ParseFilters(options.filters);
	LoadDir(false);
}

void PickFileDialog::Draw(ControlDrawData*)
{
	Window::Draw();
}

void PickFileDialog::Event(GuiEvent e)
{
	switch(e)
	{
	case GuiEvent_Initialize:
		list_box->Init();
		list_extensions->Init(true);
		break;
	case SelectItem:
		PickItem();
		break;
	case Cancel:
		CancelPick();
		break;
	default:
		GuiDialog::Event(e);
		break;
	}
}

void PickFileDialog::Update(float dt)
{
	if(Key.PressedRelease(VK_ESCAPE))
		CancelPick();
	if(list_box->focus)
	{
		if(Key.PressedRelease(VK_RETURN))
			PickItem();
		else if(Key.PressedRelease(VK_BACK) && !list_box->IsEmpty())
		{
			auto item = list_box->GetItemsCast<PickFileDialogItem>()[0];
			if(item->filename == "..")
				PickDir(item);
		}
	}
	else if(tb_filename->focus && Key.PressedRelease(VK_RETURN))
	{
		string filename = trimmed(tb_filename->GetText());
		if(!filename.empty())
		{
			bool ok = false;
			auto& items = list_box->GetItemsCast<PickFileDialogItem>();
			uint index = 0;
			for(auto item : items)
			{
				if(item->filename == filename)
				{
					if(item->is_dir)
						tb_filename->Reset();
					list_box->Select(index);
					ok = true;
					PickItem();
					break;
				}
				++index;
			}

			if(!ok)
				SimpleDialog(Format("Can't file file or directory '%s'.", filename.c_str()));
		}
	}

	Window::Update(dt);
}

string GetParentDir(const string& path)
{
	std::size_t pos = path.find_last_of('/');
	string part = path.substr(0, pos);
	return part;
}

string GetExt(const string& filename)
{
	std::size_t pos = filename.find_last_of('.');
	if(pos == string::npos)
		return string();
	string ext = filename.substr(pos + 1);
	return ext;
}

void PickFileDialog::LoadDir(bool keep_selected)
{
	WIN32_FIND_DATA find_data;
	HANDLE handle = FindFirstFile(Format("%s/*.*", active_dir.c_str()), &find_data);
	if(handle == INVALID_HANDLE_VALUE)
		return;

	string old_filename;
	auto selected = list_box->GetItemCast<PickFileDialogItem>();
	if(selected)
		old_filename = selected->filename;
	tb_path->SetText(Format("%s/", active_dir.c_str()));

	list_box->Reset();

	// add parent dir
	if(active_dir != root_dir)
	{
		auto item = new PickFileDialogItem;
		item->filename = "..";
		item->path = GetParentDir(active_dir);
		item->is_dir = true;
		item->tex = tex_dir;
		list_box->Add(item);
	}

	// add all files/dirs matching filter
	do
	{
		if(strcmp(find_data.cFileName, ".") == 0 || strcmp(find_data.cFileName, "..") == 0)
			continue;

		if(IS_SET(find_data.dwFileAttributes, FILE_ATTRIBUTE_DIRECTORY))
		{
			auto item = new PickFileDialogItem;
			item->filename = find_data.cFileName;
			item->path = Format("%s/%s", active_dir.c_str(), item->filename.c_str());
			item->is_dir = true;
			item->tex = tex_dir;
			list_box->Add(item);
		}
		else
		{
			string filename = find_data.cFileName;
			string ext = GetExt(filename);

			bool ok = false;
			if(active_filter->exts.empty())
				ok = true;
			else
			{
				for(auto& filter : active_filter->exts)
				{
					if(filter == ext)
					{
						ok = true;
						break;
					}
				}
			}

			if(ok)
			{
				auto item = new PickFileDialogItem;
				item->filename = filename;
				item->path = Format("%s/%s", active_dir.c_str(), item->filename.c_str());
				item->is_dir = false;
				list_box->Add(item);
			}
		}
	} while(FindNextFile(handle, &find_data) != 0);

	// sort items
	auto& items = list_box->GetItemsCast<PickFileDialogItem>();
	std::sort(items.begin(), items.end(), PickFileDialogItemSort);

	// keep old selected item if it exists
	if(keep_selected && !old_filename.empty())
	{
		uint index = 0;
		for(auto item : items)
		{
			if(item->filename == old_filename)
			{
				list_box->Select(index);
				break;
			}
			++index;
		}
	}
}

void SplitText(const string& str, vector<string>& splitted, char separator, bool ignore_empty)
{
	uint pos = 0;
	string text;

	while(true)
	{
		uint pos2 = str.find_first_of(separator, pos);
		if(pos2 == string::npos)
		{
			text = str.substr(pos);
			if(!text.empty() || !ignore_empty)
				splitted.push_back(text);
			break;
		}

		text = str.substr(pos, pos2 - pos);
		if(!text.empty() || !ignore_empty)
			splitted.push_back(text);

		pos = pos2 + 1;
	}
}

void PickFileDialog::ParseFilters(const string& str)
{
	filters.clear();

	uint pos = 0;
	string text, ext;

	while(true)
	{
		uint pos2 = str.find_first_of(';', pos);
		if(pos2 == string::npos)
			break;
		text = str.substr(pos, pos2 - pos);
		++pos2;
		pos = str.find_first_of(';', pos2);
		if(pos == string::npos)
			ext = str.substr(pos2);
		else
			ext = str.substr(pos2, pos - pos2);
		Filter f;
		f.text = std::move(text);
		if(ext != "*")
			SplitText(ext, f.exts, ',', true);
		filters.push_back(std::move(f));
		if(pos == string::npos)
			break;
		++pos;
	}

	if(filters.empty())
	{
		Filter f;
		f.text = "All files";
		filters.push_back(f);
	}

	active_filter = &filters[0];

	list_extensions->Reset();
	for(uint i = 0; i < filters.size(); ++i)
		list_extensions->Add(filters[i].text.c_str(), i);
	list_extensions->Select(0);
}

bool PickFileDialog::HandleChangeExtension(int action, int index)
{
	if(action == ListBox::A_INDEX_CHANGED)
	{
		active_filter = &filters[index];
		LoadDir(true);
	}

	return true;
}

bool PickFileDialog::HandleListBoxEvent(int action, int index)
{
	switch(action)
	{
	case ListBox::A_INDEX_CHANGED:
		{
			auto item = list_box->GetItemCast<PickFileDialogItem>();
			if(!item->is_dir)
				tb_filename->SetText(item->filename.c_str());
		}
		break;
	case ListBox::A_DOUBLE_CLICK:
		PickItem();
		break;
	}

	return true;
}

void PickFileDialog::PickItem()
{
	auto item = list_box->GetItemCast<PickFileDialogItem>();
	if(!item)
		return;
	if(item->is_dir)
		PickDir(item);
	else
	{
		result = true;
		result_filename = item->filename;
		result_path = item->path;
		Close();
		if(handler)
			handler(this);
	}
}

cstring FilenameFromPath(const string& path)
{
	uint pos = path.find_last_of('/');
	if(pos == string::npos)
		return path.c_str();
	else
		return path.c_str() + pos + 1;
}

void PickFileDialog::PickDir(PickFileDialogItem* item)
{
	if(item->filename == "..")
	{
		string current_dir = FilenameFromPath(active_dir);
		active_dir = item->path;
		LoadDir(false);

		// select old parent dir
		auto& items = list_box->GetItemsCast<PickFileDialogItem>();
		uint index = 0;
		for(auto item : items)
		{
			if(item->filename == current_dir)
			{
				list_box->Select(index);
				break;
			}
			++index;
		}
	}
	else
	{
		active_dir = item->path;
		LoadDir(false);
	}
}

void PickFileDialog::CancelPick()
{
	result = false;
	Close();
	if(handler)
		handler(this);
}

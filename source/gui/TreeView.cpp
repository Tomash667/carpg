#include "Pch.h"
#include "Base.h"
#include "KeyStates.h"
#include "MenuStrip.h"
#include "TextBox.h"
#include "TreeView.h"

/*
lewo - zwiñ, idŸ do parenta (z shift dzia³a)
prawo - rozwiñ - idŸ do 1 childa (shift)
góra - z shift
dó³
del - usuñ
spacja - zaznacz
litera - przejdŸ do nastêpnego o tej literze

klik - zaznacza, z ctrl dodaje/usuwa zaznaczenie
z shift - zaznacza wszystkie od poprzedniego focusa do teraz
z shift zaznacza obszar od 1 klika do X, 1 miejsce sie nie zmienia
z shift i ctrl nie usuwa nigdy zaznaczenia (mo¿na dodaæ obszary)
*/

using namespace gui;

static bool SortTreeNodesPred(const TreeNode* node1, const TreeNode* node2)
{
	if(node1->IsDir() == node2->IsDir())
		return node1->GetText() < node2->GetText();
	else
		return node1->IsDir();
}

TreeView::Enumerator::Iterator::Iterator(TreeNode* node, TreeNode::Pred pred) : node(node), pred(pred)
{
	to_check.push_back(node);
	Next();
}

void TreeView::Enumerator::Iterator::Next()
{
	while(true)
	{
		if(to_check.empty())
		{
			node = nullptr;
			return;
		}

		TreeNode* n = to_check.back();
		to_check.pop_back();

		for(auto child : n->childs)
			to_check.push_back(child);

		if(!pred || pred(n))
		{
			node = n;
			break;
		}
	}
}

TreeNode::TreeNode(bool is_dir) : tree(nullptr), parent(nullptr), selected(false), is_dir(is_dir), data(nullptr), collapsed(true)
{

}

TreeNode::~TreeNode()
{
	DeleteElements(childs);
}

void TreeNode::AddChild(TreeNode* node, bool expand)
{
	// dir przed item, sortowanie
	assert(node);
	assert(!node->tree);
	assert(tree);
	node->parent = this;
	node->tree = tree;
	auto it = std::lower_bound(childs.begin(), childs.end(), node, SortTreeNodesPred);
	if(it == childs.end())
		childs.push_back(node);
	else
		childs.insert(it, node);
	if(expand || !collapsed)
	{
		collapsed = false;
		tree->CalculatePos();
	}
}

TreeNode* TreeNode::AddDirIfNotExists(const string& name, bool expand)
{
	TreeNode* dir = FindDir(name);
	if(!dir)
	{
		dir = new TreeNode(true);
		dir->SetText(name);
		AddChild(dir, expand);
	}
	return dir;
}

void TreeNode::DeattachChild(TreeNode* node)
{
	assert(node);
	assert(node->parent == this);
	tree->RemoveSelection(node);
	node->tree = nullptr;
	node->parent = nullptr;
	RemoveElementOrder(childs, node);
}

void TreeNode::EditName()
{
	tree->EditName(this);
}

TreeNode* TreeNode::FindDir(const string& name)
{
	for(auto node : childs)
	{
		if(node->is_dir && node->text == name)
			return node;
	}

	return nullptr;
}

void TreeNode::GenerateDirName(TreeNode* node, cstring name)
{
	assert(node && name);
	node->text = name;
	uint index = 1;
	while(true)
	{
		bool ok = true;
		for(auto child : childs)
		{
			if(child->text == node->text)
			{
				ok = false;
				break;
			}
		}
		if(ok)
			return;
		node->text = Format("%s (%u)", name, index);
		++index;
	}
}

void TreeNode::RecalculatePath(const string& new_path)
{
	for(auto node : childs)
	{
		node->path = new_path;
		if(node->is_dir && !node->childs.empty())
		{
			string combined_path = Format("%s/%s", new_path.c_str(), node->text.c_str());
			node->RecalculatePath(combined_path);
		}
	}
}

void TreeNode::Remove()
{
	assert(parent);
	parent->RemoveChild(this);
}

void TreeNode::RemoveChild(TreeNode* node)
{
	assert(node);
	assert(node->parent == this);
	tree->RemoveSelection(node);
	RemoveElementOrder(childs, node);
	delete node;
}

void TreeNode::Select()
{
	tree->SelectNode(this);
}

void TreeNode::SetCollapsed(bool new_collapsed)
{
	if(new_collapsed == collapsed)
		return;
	collapsed = new_collapsed;
	if(!childs.empty())
		tree->CalculatePos();
}

void TreeNode::SetText(const AnyString& s)
{
	if(tree && parent)
	{
		auto it = std::lower_bound(parent->childs.begin(), parent->childs.end(), this, SortTreeNodesPred);
		parent->childs.erase(it);
		text = s.s;
		it = std::lower_bound(parent->childs.begin(), parent->childs.end(), this, SortTreeNodesPred);
		if(it == parent->childs.end())
			parent->childs.push_back(this);
		else
			parent->childs.insert(it, this);
		tree->CalculatePos();
	}
	else
		text = s.s;
}

TreeView::TreeView() : Control(true), TreeNode(true), menu(nullptr), scrollbar(false, true), hover(nullptr), edited(nullptr)
{
	tree = this;
	text = "Root";
	collapsed = false;
	text_box = new TextBox(false, true);
	text_box->visible = false;
	text_box->SetBackground(layout->tree_view.text_box_background);
}

TreeView::~TreeView()
{
	delete text_box;
}

void TreeView::CalculatePos()
{
	INT2 offset(0, 0);
	CalculatePos(this, offset);
}

void TreeView::CalculatePos(TreeNode* node, INT2& offset)
{
	node->pos = offset;
	offset.y += item_height;
	if(node->IsDir() && !node->IsCollapsed())
	{
		offset.x += level_offset;
		for(auto child : node->childs)
			CalculatePos(child, offset);
		offset.x -= level_offset;
	}
}

void TreeView::Draw(ControlDrawData*)
{
	BOX2D box = BOX2D::Create(global_pos, size);
	GUI.DrawArea(box, layout->tree_view.background);

	Draw(this);

	if(text_box->visible)
		text_box->Draw();
}

void TreeView::Draw(TreeNode* node)
{
	int offset = 0;

	// selection
	if(node->selected)
		GUI.DrawArea(BOX2D::Create(global_pos + INT2(1, node->pos.y + 1), INT2(size.x - 2, item_height - 1)), layout->tree_view.selected);

	if(node->IsDir())
	{
		// collapse/expand button
		AreaLayout* area;
		if(node->collapsed)
		{
			if(node == hover)
				area = &layout->tree_view.button_hover;
			else
				area = &layout->tree_view.button;
		}
		else
		{
			if(node == hover)
				area = &layout->tree_view.button_down_hover;
			else
				area = &layout->tree_view.button_down;
		}
		GUI.DrawArea(BOX2D::Create(global_pos + node->pos, area->size), *area);
		offset += area->size.x;
	}

	// text
	if(node != edited)
	{
		RECT r = { global_pos.x + node->pos.x + offset, global_pos.y + node->pos.y, global_pos.x + size.x, global_pos.y + node->pos.y + item_height };
		GUI.DrawText(layout->tree_view.font, node->text, DT_LEFT | DT_VCENTER | DT_SINGLELINE, layout->tree_view.font_color, r, &r);
	}

	// childs
	if(node->IsDir() && !node->IsCollapsed())
	{
		for(auto child : node->childs)
			Draw(child);
	}
}

void TreeView::Event(GuiEvent e)
{
	if(e == GuiEvent_Initialize)
	{
		item_height = layout->tree_view.font->height + 2;
		level_offset = layout->tree_view.level_offset;
		CalculatePos();
	}
}

void TreeView::Update(float dt)
{
	hover = nullptr;

	// update edit box
	if(text_box->visible)
	{
		UpdateControl(text_box, dt);
		if(!text_box->focus)
			EndEdit(true);
		else
		{
			if(Key.PressedRelease(VK_RETURN))
				EndEdit(true);
			else if(Key.PressedRelease(VK_ESCAPE))
				EndEdit(false);
		}
	}

	// recursively update nodes
	if(mouse_focus)
		Update(this);

	// keyboard shortcuts
	if(focus && current)
	{
		if(Key.DownRepeat(VK_UP))
			MoveCurrent(-1);
		else if(Key.DownRepeat(VK_DOWN))
			MoveCurrent(+1);
		else if(Key.PressedRelease(VK_LEFT))
		{
			if(current->IsDir())
				current->SetCollapsed(true);
		}
		else if(Key.PressedRelease(VK_RIGHT))
		{
			if(current->IsDir())
				current->SetCollapsed(false);
		}
		else if(Key.Shortcut(VK_CONTROL, 'R'))
			current->EditName();
	}
}

void TreeView::EndEdit(bool apply)
{
	text_box->visible = false;
	if(!apply)
	{
		edited = nullptr;
		return;
	}
	new_name = text_box->GetText();
	trim(new_name);
	if(new_name != edited->text)
	{
		if(!new_name.empty())
		{
			bool ok = true;
			for(auto child : edited->parent->childs)
			{
				if(child != edited && child->IsDir() == edited->IsDir() && child->text == new_name)
				{
					ok = false;
					break;
				}
			}

			if(!ok)
				SimpleDialog("Name must be unique.");
			else if(handler(A_BEFORE_RENAME, (int)edited))
			{
				edited->SetText(new_name);
				handler(A_RENAMED, (int)edited);
			}
		}
		else
			SimpleDialog("Name cannot be empty.");
	}
	edited = nullptr;
}

bool TreeView::Update(TreeNode* node)
{
	if(GUI.cursor_pos.y >= global_pos.y + node->pos.y && GUI.cursor_pos.y <= global_pos.y + node->pos.y + item_height)
	{
		if(menu && Key.Pressed(VK_RBUTTON))
		{
			if(SelectNode(node) && handler(A_BEFORE_MENU_SHOW, (int)node))
			{
				menu->SetHandler(delegate<void(int)>(this, &TreeView::OnSelect));
				menu->ShowMenu();
			}
		}

		if(node->IsDir() && PointInRect(GUI.cursor_pos, global_pos.x + node->pos.x, global_pos.y + node->pos.y, 
			global_pos.x + node->pos.x + 16, global_pos.y + node->pos.y + item_height))
		{
			hover = node;
			if(Key.Pressed(VK_LBUTTON))
			{
				node->SetCollapsed(!node->IsCollapsed());
				SelectNode(node);
				TakeFocus(true);
			}
			return true;
		}

		if(Key.Pressed(VK_LBUTTON))
		{
			SelectNode(node);
			TakeFocus(true);
		}
		return true;
	}

	if(node->IsDir() && !node->IsCollapsed())
	{
		for(auto child : node->childs)
		{
			if(Update(child))
				return true;
		}
	}

	return false;
}

void TreeView::Add(TreeNode* node, const string& path, bool expand)
{
	assert(node);

	if(path.empty())
	{
		AddChild(node, expand);
		return;
	}

	uint pos = 0;
	TreeNode* container = this;
	string part;

	while(true)
	{
		uint end = path.find_first_of('/', pos);
		if(end == string::npos)
			break;
		part = path.substr(pos, end - pos);
		container = container->AddDirIfNotExists(part, expand);
		pos = end + 1;
	}
	
	part = path.substr(pos);
	container = container->AddDirIfNotExists(part, expand);
	container->AddChild(node, expand);
}

void TreeView::Deattach(TreeNode* node)
{
	assert(node);
	assert(node->tree == this && node->parent != nullptr); // root node have null parent
	RemoveSelection(node);
	RemoveElementOrder(node->parent->childs, node);
	node->parent = nullptr;
	node->tree = nullptr;
}

void TreeView::EditName(TreeNode* node)
{
	assert(node && node->tree == this);
	if(!SelectNode(node))
		return;
	edited = node;
	text_box->visible = true;
	text_box->SetText(node->text.c_str());
	INT2 pos = global_pos + node->pos;
	if(node->IsDir())
		pos.x += layout->tree_view.button.size.x;
	text_box->SetPosition(pos - INT2(0,2));
	text_box->SetSize(INT2(size.x - node->pos.x - layout->tree_view.button.size.x, item_height + 4));
	text_box->SelectAll();
}

TreeView::Enumerator TreeView::ForEachNotDir()
{
	return Enumerator(this, [](TreeNode* node) { return node->IsDir(); });
}

void TreeView::RecalculatePath()
{
	path.clear();
	TreeNode::RecalculatePath(path);
}

bool TreeView::SelectNode(TreeNode* node)
{
	assert(node && node->tree == this);
	if(selected.size() == 1u && selected.back() == node)
		return true;

	if(!handler(A_BEFORE_CURRENT_CHANGE, (int)node))
		return false;

	// deselect old
	for(auto node : selected)
		node->selected = false;
	selected.clear();

	// select new
	node->selected = true;
	selected.push_back(node);
	if(current != node)
	{
		current = node;
		handler(A_CURRENT_CHANGED, (int)node);
	}
	return true;
}

void TreeView::Remove(TreeNode* node)
{
	assert(node);
	assert(node->tree == this && node->parent != nullptr); // root node have null parent
	RemoveSelection(node);
	RemoveElementOrder(node->parent->childs, node);
	delete node;
}

void TreeView::RemoveSelection(TreeNode* node)
{
	for(TreeNode* child : node->childs)
		RemoveSelection(child);
	if(node->selected)
	{
		RemoveElementOrder(selected, node);
		node->selected = false;
	}
}

void TreeView::OnSelect(int id)
{
	handler(A_MENU, id);
}

void TreeView::MoveCurrent(int dir)
{
	if(dir == -1)
	{
		if(current->parent == nullptr)
			return;

		auto node = current;
		int index = GetIndex(node->parent->childs, node);
		if(index != 0)
		{
			node = node->parent->childs[index - 1];
			while(node->IsDir() && !node->IsCollapsed() && !node->childs.empty())
				node = node->childs.back();
		}
		else
			node = node->parent;
		SelectNode(node);
	}
	else
	{
		if(current->parent == nullptr)
		{
			if(!current->childs.empty())
				SelectNode(current->childs[0]);
			return;
		}

		auto node = current;
		if(node->IsDir() && !node->IsCollapsed() && !node->childs.empty())
			node = current->childs.front();
		else
		{
			while(true)
			{
				int index = GetIndex(node->parent->childs, node);
				if(index + 1 != node->parent->childs.size())
				{
					node = node->parent->childs[index + 1];
					break;
				}
				else
				{
					node = node->parent;
					if(node->parent == nullptr)
						return;
				}
			}
		}
		SelectNode(node);
	}
}

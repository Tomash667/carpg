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
	if(node)
	{
		to_check.push_back(node);
		Next();
	}
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

		PredResult result;
		if(pred)
			result = pred(n);
		else
			result = GET_AND_CHECK_CHILDS;

		if(result == SKIP_AND_CHECK_CHILDS || result == GET_AND_CHECK_CHILDS)
		{
			for(auto child : n->childs)
				to_check.push_back(child);
		}

		if(result == GET_AND_SKIP_CHILDS || result == GET_AND_CHECK_CHILDS)
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
	tree->Remove(this);
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

TreeView::TreeView() : Control(true), TreeNode(true), menu(nullptr), scrollbar(false, true), hover(nullptr), edited(nullptr), fixed(nullptr), drag(DRAG_NO)
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

	if(drag == DRAG_MOVED)
		GUI.DrawSprite(layout->tree_view.drag_n_drop, GUI.cursor_pos + INT2(16, 16));
}

void TreeView::Draw(TreeNode* node)
{
	int offset = 0;

	// selection
	if(node->selected || (drag == DRAG_MOVED && node == above && CanDragAndDrop()))
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
	else if(e == GuiEvent_LostFocus)
	{
		drag = DRAG_NO;
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
	above = nullptr;
	if(mouse_focus)
		Update(this);

	// drag & drop
	if(drag != DRAG_NO)
	{
		if(drag == DRAG_DOWN && drag_node != above && above)
			drag = DRAG_MOVED;
		if(Key.Up(VK_LBUTTON))
		{
			if(above == drag_node)
				SelectNode(drag_node, Key.Down(VK_SHIFT), false, Key.Down(VK_CONTROL));
			else if(CanDragAndDrop())
			{
				auto old_selected = selected_nodes;
				SelectTopSelectedNodes();
				above->collapsed = false;
				for(auto node : selected_nodes)
				{
					MoveNode(node, above);
					node->selected = false;
				}
				selected_nodes = old_selected;
				for(auto node : selected_nodes)
					node->selected = true;
				CalculatePos();
			}
			drag = DRAG_NO;
		}
	}

	// keyboard shortcuts
	if(focus && current && drag == DRAG_NO)
	{
		if(Key.DownRepeat(VK_UP))
			MoveCurrent(-1, Key.Down(VK_SHIFT));
		else if(Key.DownRepeat(VK_DOWN))
			MoveCurrent(+1, Key.Down(VK_SHIFT));
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
		else if(Key.PressedRelease(VK_DELETE))
			handler(A_DELETE_KEY, (int)current);
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
			{
				SimpleDialog("Name must be unique.");
				SelectNode(edited);
			}
			else if(handler(A_BEFORE_RENAME, (int)edited))
			{
				edited->SetText(new_name);
				handler(A_RENAMED, (int)edited);
			}
		}
		else
		{
			SimpleDialog("Name cannot be empty.");
			SelectNode(edited);
		}
	}
	edited = nullptr;
}

bool TreeView::Update(TreeNode* node)
{
	if(GUI.cursor_pos.y >= global_pos.y + node->pos.y && GUI.cursor_pos.y <= global_pos.y + node->pos.y + item_height)
	{
		above = node;

		bool add = Key.Down(VK_SHIFT);
		bool ctrl = Key.Down(VK_CONTROL);

		if(menu && Key.Pressed(VK_RBUTTON))
		{
			if(SelectNode(node, add, true, ctrl) && handler(A_BEFORE_MENU_SHOW, (int)node))
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
				SelectNode(node, add, false, ctrl);
				TakeFocus(true);
			}
			return true;
		}

		if(Key.Pressed(VK_LBUTTON))
		{
			if(!node->selected)
				SelectNode(node, add, false, ctrl);
			TakeFocus(true);
			drag = DRAG_DOWN;
			drag_node = node;
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

void TreeView::ClearSelection()
{
	for(auto node : selected_nodes)
		node->selected = false;
	selected_nodes.clear();
	current = nullptr;
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
	return Enumerator(this, [](TreeNode* node)
	{
		return node->IsDir() ? SKIP_AND_CHECK_CHILDS : GET_AND_CHECK_CHILDS;
	});
}

void TreeView::RecalculatePath()
{
	path.clear();
	TreeNode::RecalculatePath(path);
}

bool TreeView::SelectNode(TreeNode* node, bool add, bool right_click, bool ctrl)
{
	assert(node && node->tree == this);

	if(current == node && (add || right_click))
		return true;

	if(!handler(A_BEFORE_CURRENT_CHANGE, (int)node))
		return false;

	if(right_click && node->selected)
	{
		current = node;
		handler(A_CURRENT_CHANGED, (int)node);
		return true;
	}

	// deselect old
	if(!ctrl)
	{
		for(auto node : selected_nodes)
			node->selected = false;
		selected_nodes.clear();
	}

	if(add && fixed)
		SelectRange(fixed, node);
	else
	{
		// select new
		if(!node->selected)
		{
			node->selected = true;
			selected_nodes.push_back(node);
		}
		fixed = node;
	}

	current = node;
	handler(A_CURRENT_CHANGED, (int)node);

	return true;
}

void TreeView::Remove(TreeNode* node)
{
	assert(node);
	assert(node->tree == this && node->parent != nullptr); // root node have null parent
	RemoveSelection(node);
	TreeNode* next;
	auto parent = node->parent;
	int index = GetIndex(parent->childs, node);
	if(index + 1 < (int)parent->childs.size())
		next = parent->childs[index + 1];
	else if(index > 0)
		next = parent->childs[index - 1];
	else
		next = parent;
	RemoveElementOrder(node->parent->childs, node);
	delete node;
	CalculatePos();
	SelectNode(next);
}

void TreeView::RemoveSelection(TreeNode* node)
{
	if(node->selected)
	{
		RemoveElement(selected_nodes, node);
		node->selected = false;
	}
	if(current == node)
		current = nullptr;
	for(auto child : node->childs)
		RemoveSelection(child);
}

void TreeView::OnSelect(int id)
{
	handler(A_MENU, id);
}

void TreeView::MoveCurrent(int dir, bool add)
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
		SelectNode(node, add, false, false);
	}
	else
	{
		if(current->parent == nullptr)
		{
			if(!current->childs.empty())
				SelectNode(current->childs[0], add, false, false);
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
		SelectNode(node, add, false, false);
	}
}

void TreeView::SelectRange(TreeNode* node1, TreeNode* node2)
{
	if(node1->pos.y > node2->pos.y)
		std::swap(node1, node2);
	auto e = ForEach([](TreeNode* node)
	{
		if(node->IsDir() && node->IsCollapsed())
			return GET_AND_SKIP_CHILDS;
		else
			return GET_AND_CHECK_CHILDS;
	});
	for(auto node : e)
	{
		if(!node->selected && node->pos.y >= node1->pos.y && node->pos.y <= node2->pos.y)
		{
			node->selected = true;
			selected_nodes.push_back(node);
		}
	}
}

void TreeView::RemoveSelected()
{
	if(selected_nodes.empty())
		return;

	if(selected_nodes.size() == 1u)
	{
		Remove(current);
		return;
	}

	auto node = current;
	// get not selected parent
	while(node->parent->selected)
		node = node->parent;
	bool ok = !node->selected;
	if(!ok)
	{
		// get item below
		int index = GetIndex(node->parent->childs, node);
		int start_index = index;
		while(true)
		{
			++index;
			if(index == node->parent->childs.size())
				break;
			if(!node->parent->childs[index]->selected)
			{
				ok = true;
				node = node->parent->childs[index];
				break;
			}
		}

		// get item above
		if(!ok)
		{
			index = start_index;
			while(true)
			{
				--index;
				if(index < 0)
					break;
				if(!node->parent->childs[index]->selected)
				{
					ok = true;
					node = node->parent->childs[index];
					break;
				}
			}
		}

		// get parent
		if(!ok)
			node = node->parent;
	}

	// remove nodes
	SelectTopSelectedNodes();
	for(auto node : selected_nodes)
	{
		RemoveElementOrder(node->parent->childs, node);
		delete node;
	}
	selected_nodes.clear();

	// select new
	CalculatePos();
	SelectNode(node);
}

void TreeView::SelectChildNodes()
{
	for(auto node : selected_nodes)
		SelectChildNodes(node);
}

void TreeView::SelectChildNodes(TreeNode* node)
{
	for(auto child : node->childs)
	{
		if(!child->selected)
		{
			child->selected;
			if(!child->childs.empty())
				SelectChildNodes(child);
		}
	}
}

void TreeView::SelectTopSelectedNodes()
{
	SelectChildNodes();
	LoopAndRemove(selected_nodes, [](TreeNode* node) { return node->GetParent()->IsSelected(); });
}

bool TreeView::CanDragAndDrop()
{
	return above && !above->selected && above->IsDir();
}

bool TreeView::MoveNode(TreeNode* node, TreeNode* new_parent)
{
	if(node->parent == new_parent)
		return false;
	RemoveElementOrder(node->parent->childs, node);
	node->parent = new_parent;
	auto it = std::lower_bound(new_parent->childs.begin(), new_parent->childs.end(), node, SortTreeNodesPred);
	if(it == new_parent->childs.end())
		new_parent->childs.push_back(node);
	else
		new_parent->childs.insert(it, node);
	return true;
}

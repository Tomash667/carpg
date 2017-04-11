#pragma once

#include "Scrollbar.h"

/*
TreeView
current - last clicked node, only one
selected - can be more than one if multiselect enabled

Currently can'y change this settings: drag&drop, multiselect, autosort (YAGNI)
*/

class TextBox;

namespace gui
{
	class MenuStrip;
	class TreeView;

	class TreeNode
	{
		friend TreeView;
	public:
		enum PredResult
		{
			SKIP_AND_SKIP_CHILDS,
			SKIP_AND_CHECK_CHILDS,
			GET_AND_SKIP_CHILDS,
			GET_AND_CHECK_CHILDS
		};

		typedef delegate<PredResult(TreeNode*)> Pred;

		TreeNode(bool is_dir = false);
		virtual ~TreeNode();

		void AddChild(TreeNode* node, bool expand = true);
		TreeNode* AddDirIfNotExists(const string& name, bool expand = true);
		void DeattachChild(TreeNode* node);
		void EditName();
		TreeNode* FindDir(const string& name);
		void GenerateDirName(TreeNode* node, cstring name);
		void RecalculatePath(const string& new_path);
		void Remove();
		void Select();

		vector<TreeNode*>& GetChilds() { return childs; }
		void* GetData() { return data; }
		template<typename T>
		T* GetData() { return (T*)data; }
		TreeNode* GetParent() { return parent; }
		const string& GetPath() const { return path; }
		const string& GetText() const { return text; }
		TreeView* GetTree() { return tree; }
		bool IsCollapsed() const { return collapsed; }
		bool IsDir() const { return is_dir; }
		bool IsEmpty() const { return childs.empty(); }
		bool IsRoot() const { return parent == nullptr; }
		bool IsSelected() const { return selected; }

		void SetCollapsed(bool new_collapsed);
		void SetData(void* new_data) { data = new_data; }
		void SetText(const AnyString& s);

	private:
		void CalculateWidth();

		string text, path;
		TreeView* tree;
		TreeNode* parent;
		vector<TreeNode*> childs;
		void* data;
		INT2 pos;
		int width, end_offset;
		bool selected, is_dir, collapsed;
	};

	class TreeView : public Control, public TreeNode
	{
		friend TreeNode;

		struct Enumerator
		{
			struct Iterator
			{
				Iterator(TreeNode* node, TreeNode::Pred pred);
				bool operator != (const Iterator& it) const { return node != it.node; }
				void operator ++ () { Next(); }
				TreeNode* operator * () { return node; }

			private:
				void Next();

				TreeNode* node;
				TreeNode::Pred pred;
				vector<TreeNode*> to_check;
			};

			Enumerator(TreeNode* node, TreeNode::Pred pred) : node(node), pred(pred) {}
			Iterator begin() { return Iterator(node, pred); }
			Iterator end() { return Iterator(nullptr, nullptr); }

		private:
			TreeNode* node;
			TreeNode::Pred pred;
		};

	public:
		enum Action
		{
			A_BEFORE_CURRENT_CHANGE,
			A_CURRENT_CHANGED,
			A_BEFORE_MENU_SHOW,
			A_MENU,
			A_BEFORE_RENAME,
			A_RENAMED,
			A_SHORTCUT
		};

		enum Shortcut
		{
			S_ADD,
			S_ADD_DIR,
			S_DUPLICATE,
			S_RENAME,
			S_REMOVE
		};

		typedef delegate<bool(int, int)> Handler;

		TreeView();
		~TreeView();

		void Draw(ControlDrawData* cdd = nullptr) override;
		void Event(GuiEvent e) override;
		void Update(float dt) override;

		void Add(TreeNode* node, const string& path, bool expand = true);
		void ClearSelection();
		void CollapseAll() { SetAllCollapsed(true); }
		void Deattach(TreeNode* node);
		void EditName(TreeNode* node);
		void ExpandAll() { SetAllCollapsed(false); }
		void Remove(TreeNode* node);
		Enumerator ForEach() { return Enumerator(this, nullptr); }
		Enumerator ForEach(delegate<bool(TreeNode*)> pred) { return Enumerator(this, pred); }
		Enumerator ForEachNotDir();
		void RecalculatePath();
		void RemoveSelected();
		bool SelectNode(TreeNode* node) { return SelectNode(node, false, false, false); }
		void SetAllCollapsed(bool collapsed);

		TreeNode* GetCurrentNode() { return current; }
		Handler GetHandler() const { return handler; }
		MenuStrip* GetMenu() const { return menu; }
		string& GetNewName() { return new_name; }
		TreeNode* GetSelectedNode() { return selected_nodes.empty() ? nullptr : selected_nodes[0]; }
		vector<TreeNode*>& GetSelectedNodes() { return selected_nodes; }

		bool HaveSelected() const { return !selected_nodes.empty(); }
		bool IsMultipleNodesSelected() const { return selected_nodes.size() > 1u; }

		void SetHandler(Handler new_handler) { handler = new_handler; }
		void SetMenu(MenuStrip* new_menu) { menu = new_menu; }

	private:
		enum DRAG_MODE
		{
			DRAG_NO,
			DRAG_DOWN,
			DRAG_MOVED
		};

		void CalculatePos();
		void CalculatePos(TreeNode* node, INT2& offset, int& max_width);
		bool CanDragAndDrop();
		void Draw(TreeNode* node);
		void EndEdit(bool apply, bool set_focus);
		TreeNode* GetNextNode(int dir);
		void MoveCurrent(int dir, bool add);
		bool MoveNode(TreeNode* node, TreeNode* new_parent);
		void RemoveSelection(TreeNode* node);
		bool Update(TreeNode* node);
		void UpdateSize(TreeNode* node);
		void OnSelect(int id);
		bool SelectNode(TreeNode* node, bool add, bool right_click, bool ctrl);
		void SelectRange(TreeNode* node1, TreeNode* node2);
		void SelectChildNodes();
		void SelectChildNodes(TreeNode* node);
		void SelectTopSelectedNodes();
		void SetTextboxLocation();

		vector<TreeNode*> selected_nodes;
		TreeNode* current, *hover, *edited, *fixed, *drag_node, *above;
		Handler handler;
		MenuStrip* menu;
		Scrollbar hscrollbar, vscrollbar;
		TextBox* text_box;
		string new_name;
		int item_height, level_offset;
		DRAG_MODE drag;
		INT2 total_size, area_size;
		BOX2D clip_rect;
	};
}

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

		typedef delegate<bool(TreeNode*)> Pred;

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
		void RemoveChild(TreeNode* node);
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

		void SetCollapsed(bool new_collapsed);
		void SetData(void* new_data) { data = new_data; }
		void SetText(const AnyString& s);

	private:
		string text, path;
		TreeView* tree;
		TreeNode* parent;
		vector<TreeNode*> childs;
		void* data;
		INT2 pos;
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
			A_RENAMED
		};

		typedef delegate<bool(int, int)> Handler;

		TreeView();
		~TreeView();

		void Draw(ControlDrawData* cdd = nullptr) override;
		void Event(GuiEvent e) override;
		void Update(float dt) override;

		void Add(TreeNode* node, const string& path, bool expand = true);
		void ClearSelection();
		void Deattach(TreeNode* node);
		void EditName(TreeNode* node);
		void Remove(TreeNode* node);
		Enumerator ForEach() { return Enumerator(this, nullptr); }
		Enumerator ForEach(delegate<bool(TreeNode*)> pred) { return Enumerator(this, pred); }
		Enumerator ForEachNotDir();
		void RecalculatePath();
		bool SelectNode(TreeNode* node);

		bool HaveSelected() const { return !selected.empty(); }

		TreeNode* GetCurrentNode() { return current; }
		//bool GetDragAndDrop() const { return drag_drop; }
		Handler GetHandler() const { return handler; }
		MenuStrip* GetMenu() const { return menu; }
		//bool GetMultiselect() const { return multiselect; }
		string& GetNewName() { return new_name; }
		TreeNode* GetSelectedNode() { return selected.empty() ? nullptr : selected[0]; }
		vector<TreeNode*>& GetSelectedNodes() { return selected; }

		//void SetDragAndDrop(bool allow) { drag_drop = allow; }
		void SetHandler(Handler new_handler) { handler = new_handler; }
		void SetMenu(MenuStrip* new_menu) { menu = new_menu; }
		//void SetMultiselect(bool allow) { multiselect = allow; }

	private:
		void CalculatePos();
		void CalculatePos(TreeNode* node, INT2& offset);
		void Draw(TreeNode* node);
		void EndEdit(bool apply);
		void MoveCurrent(int dir);
		void RemoveSelection(TreeNode* node);
		bool Update(TreeNode* node);
		void OnSelect(int id);

		vector<TreeNode*> selected;
		TreeNode* current, *hover, *edited;
		Handler handler;
		MenuStrip* menu;
		Scrollbar scrollbar;
		TextBox* text_box;
		string new_name;
		int item_height, level_offset;
		//bool multiselect, drag_drop;
	};
}

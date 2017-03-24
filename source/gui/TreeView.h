#pragma once

#include "Control.h"
#include "Event.h"

namespace gui
{
	class TreeView;

	class TreeNode
	{
		friend TreeView;
	public:
		TreeNode();
		virtual ~TreeNode();

		void AddChild(TreeNode* node);
		void AddToSelection();
		void DeattachChild(TreeNode* node);
		void RemoveChild(TreeNode* node);
		void Select();

		vector<TreeNode*>& GetChilds() { return childs; }
		TreeNode* GetParent() { return parent; }
		const string& GetText() const { return text; }
		TreeView* GetTree() { return tree; }

		void SetText(const AnyString& s) { text = s.s; }

	private:
		string text;
		TreeView* tree;
		TreeNode* parent;
		vector<TreeNode*> childs;
		bool selected;
	};

	class TreeView : public Control, public TreeNode
	{
		friend TreeNode;
	public:
		TreeView();
		~TreeView();

		void Draw(ControlDrawData* cdd = nullptr) override;
		void Event(GuiEvent e) override;
		void Update(float dt) override;

		void ClearSelection();
		void Deattach(TreeNode* node);
		void Remove(TreeNode* node);

		bool HaveSelected() const { return !selected.empty(); }

		TreeNode* GetClickedNode() { return clicked; }
		bool GetDragAndDrop() const { return drag_drop; }
		KeyEvent GetKeyEvent() { return key_event; }
		MouseEvent GetMouseEvent() { return mouse_event; }
		bool GetMultiselect() const { return multiselect; }
		TreeNode* GetSelectedNode() { return selected.empty() ? nullptr : selected[0]; }
		vector<TreeNode*>& GetSelectedNodes() { return selected; }

		void SetDragAndDrop(bool allow) { drag_drop = allow; }
		void SetKeyEvent(KeyEvent event) { key_event = event; }
		void SetMouseEvent(MouseEvent event) { mouse_event = event; }
		void SetMultiselect(bool allow) { multiselect = allow; }

	private:
		void RemoveSelection(TreeNode* node);

		vector<TreeNode*> selected;
		TreeNode* clicked;
		KeyEvent key_event;
		MouseEvent mouse_event;
		bool multiselect, drag_drop;
	};
}

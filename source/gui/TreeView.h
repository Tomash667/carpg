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

		inline vector<TreeNode*>& GetChilds() { return childs; }
		inline TreeNode* GetParent() { return parent; }
		inline const string& GetText() const { return text; }
		inline TreeView* GetTree() { return tree; }

		inline void SetText(const AnyString& s) { text = s.s; }

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

		inline bool HaveSelected() const { return !selected.empty(); }

		inline TreeNode* GetClickedNode() { return clicked; }
		inline bool GetDragAndDrop() const { return drag_drop; }
		inline KeyEvent GetKeyEvent() { return key_event; }
		inline MouseEvent GetMouseEvent() { return mouse_event; }
		inline bool GetMultiselect() const { return multiselect; }
		inline TreeNode* GetSelectedNode()
		{
			if(selected.empty())
				return nullptr;
			return selected[0];
		}
		inline vector<TreeNode*>& GetSelectedNodes() { return selected; }

		inline void SetDragAndDrop(bool allow) { drag_drop = allow; }
		inline void SetKeyEvent(KeyEvent event) { key_event = event; }
		inline void SetMouseEvent(MouseEvent event) { mouse_event = event; }
		inline void SetMultiselect(bool allow) { multiselect = allow; }

	private:
		void RemoveSelection(TreeNode* node);

		vector<TreeNode*> selected;
		TreeNode* clicked;
		KeyEvent key_event;
		MouseEvent mouse_event;
		bool multiselect, drag_drop;
	};
}

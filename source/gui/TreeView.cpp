#include "Pch.h"
#include "Base.h"
#include "TreeView.h"

using namespace gui;

TreeNode::TreeNode() : tree(nullptr), parent(nullptr), selected(false)
{

}

TreeNode::~TreeNode()
{
	DeleteElements(childs);
}

void TreeNode::AddChild(TreeNode* node)
{
	assert(node);
	assert(!node->tree);
	node->parent = this;
	node->tree = tree;
	childs.push_back(node);
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

void TreeNode::RemoveChild(TreeNode* node)
{
	assert(node);
	assert(node->parent == this);
	tree->RemoveSelection(node);
	RemoveElementOrder(childs, node);
	delete node;
}

TreeView::TreeView() : multiselect(false)
{
	tree = this;
}

TreeView::~TreeView()
{
}

void TreeView::Draw(ControlDrawData*)
{
	GUI.DrawArea(COLOR_RGB(0, 255, 0), global_pos, INT2(100, 100));
}

void TreeView::Event(GuiEvent e)
{
}

void TreeView::Update(float dt)
{
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

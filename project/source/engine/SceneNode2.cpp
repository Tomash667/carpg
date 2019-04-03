#include "Pch.h"
#include "EngineCore.h"
#include "SceneNode2.h"
#include "MeshInstance.h"

void SceneNode2::OnGet()
{
	id = 0;
	parent = nullptr;
	mesh = nullptr;
	mesh_inst = nullptr;
	tex_override = nullptr;
	tint = Color::White;
	visible = true;
}

void SceneNode2::OnFree()
{
	Free(childs);
}


void SceneNode2::AddChild(SceneNode2* node, int parent_point)
{
	assert(node && !node->parent);
	node->parent = this;
	childs.push_back(node);
	assert(node->VerifyParentPoint(parent_point));
	node->parent_point = parent_point;
}

void SceneNode2::AddChild(SceneNode2* node, cstring name)
{
	assert(node && !node->parent);
	node->parent = this;
	childs.push_back(node);
	assert(mesh_inst);
	int index = mesh_inst->mesh->GetPointIndex(name);
	assert(index != -1);
	node->parent_point = index;
}

SceneNode2* SceneNode2::GetNode(int id)
{
	for(SceneNode2* node : childs)
	{
		if(node->id == id)
			return node;
	}
	return nullptr;
}

void SceneNode2::SetMesh(Mesh* mesh)
{
	this->mesh = mesh;
	mesh_inst = nullptr;
	assert(VerifyParentPoint(parent_point));
}

void SceneNode2::SetMeshInstance(MeshInstance* mesh_inst)
{
	this->mesh_inst = mesh_inst;
	mesh = mesh_inst->mesh;
	assert(VerifyParentPoint(parent_point));
}

void SceneNode2::SetMeshInstance(Mesh* mesh)
{
	mesh_inst = new MeshInstance(mesh);
	this->mesh = mesh;
	assert(VerifyParentPoint(parent_point));
}

void SceneNode2::SetParentPoint(int parent_point)
{
	assert(VerifyParentPoint(parent_point));
	this->parent_point = parent_point;
}

void SceneNode2::SetParentPoint(cstring name)
{
	assert(name && mesh_inst);
	parent_point = mesh_inst->mesh->GetPointIndex(name);
	assert(parent_point != -1);
}

bool SceneNode2::VerifyParentPoint(int parent_point) const
{
	if(parent_point == NONE)
		return true;
	if(!parent || !parent->mesh_inst)
		return false;
	if(parent_point == USE_PARENT_SKELETON)
		return true;
	return (uint)parent_point < parent->mesh_inst->mesh->attach_points.size();
}

#include "Pch.h"
#include "SceneNodeHelper.h"

#include <MeshInstance.h>
#include <SceneNode.h>

//=================================================================================================
bool SceneNodeHelper::Create(SceneNode*& node, Mesh* mesh)
{
	assert(mesh);
	if(node)
	{
		assert(node->mesh_inst && node->mesh_inst->preload);
		node->mesh_inst->ApplyPreload(mesh);
		node->SetMesh(node->mesh_inst);
		return true;
	}
	else
	{
		node = SceneNode::Get();
		node->SetMesh(new MeshInstance(mesh));
		return false;
	}
}

//=================================================================================================
void SceneNodeHelper::Save(SceneNode* node, StreamWriter& f)
{
	assert(!node || node->mesh_inst);
	if(!node || !node->mesh_inst->groups[0].IsActive())
	{
		f.Write0();
		return;
	}
	MeshInstance* meshInst = node->mesh_inst;
	const byte groups = (byte)meshInst->groups.size();
	f << groups;
	if(groups == 1u)
	{
		const MeshInstance::Group& group = meshInst->groups[0];
		f << group.time;
		f << group.speed;
		f << (group.state & ~MeshInstance::FLAG_BLENDING); // don't save blending
		if(group.anim)
			f << group.anim->name;
		else
			f.Write0();
		f << group.frame_end;
	}
	else
	{
		for(const MeshInstance::Group& group : meshInst->groups)
		{
			f << group.time;
			f << group.speed;
			f << (group.state & ~MeshInstance::FLAG_BLENDING); // don't save blending
			f << group.prio;
			f << group.used_group;
			if(group.anim)
				f << group.anim->name;
			else
				f.Write0();
			f << group.frame_end;
		}
	}
}

//=================================================================================================
SceneNode* SceneNodeHelper::Load(StreamReader& f)
{
	byte groups;
	f >> groups;
	if(groups == 0u)
		return nullptr;

	MeshInstance* meshInst = new MeshInstance(nullptr);
	meshInst->groups.resize(groups);
	if(groups == 1u)
	{
		MeshInstance::Group& group = meshInst->groups[0];
		f >> group.time;
		f >> group.speed;
		f >> group.state;
		const string& animName = f.ReadString1();
		if(!animName.empty())
			group.animName = StringPool.Get(animName);
		f >> group.frame_end;
		group.used_group = 0;
		group.prio = 0;
	}
	else
	{
		for(MeshInstance::Group& group : meshInst->groups)
		{
			f >> group.time;
			f >> group.speed;
			f >> group.state;
			f >> group.prio;
			f >> group.used_group;
			const string& animName = f.ReadString1();
			if(!animName.empty())
				group.animName = StringPool.Get(animName);
			f >> group.frame_end;
		}
	}

	SceneNode* node = SceneNode::Get();
	node->mesh_inst = meshInst;
	return node;
}

//=================================================================================================
SceneNode* SceneNodeHelper::old::LoadSimple(GameReader& f)
{
	int state = f.Read<int>();
	if(state == 0)
		return nullptr;

	MeshInstance* meshInst = new MeshInstance(nullptr);
	meshInst->groups.resize(1u);
	MeshInstance::Group& group = meshInst->groups[0];
	group.state = state;
	group.used_group = 0;
	f >> group.time;
	f >> group.blend_time;
	group.animName = StringPool.Get("first");

	SceneNode* node = SceneNode::Get();
	node->mesh_inst = meshInst;
	return node;
}

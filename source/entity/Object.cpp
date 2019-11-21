#include "Pch.h"
#include "GameCore.h"
#include "GameFile.h"
#include "Object.h"
#include "SaveState.h"
#include "ResourceManager.h"
#include "BitStreamFunc.h"
#include "LevelArea.h"
#include <Scene.h>

//=================================================================================================
void Object::CreateNode(Scene* scene)
{
	node = SceneNode::Get();
	node->pos = pos;
	if(IsBillboard())
		node->billboard = true;
	else
		node->mat = Matrix::Transform(pos, rot, scale);
	node->mesh = mesh;
	int alpha = RequireAlphaTest();
	if(alpha == 0)
		node->flags = SceneNode::F_ALPHA_TEST;
	else if(alpha == 1)
		node->flags = SceneNode::F_ALPHA_TEST | SceneNode::F_NO_CULLING;
	node->tmp = false;
	scene->Add(node);
}

//=================================================================================================
void Object::Save(GameWriter& f)
{
	f << pos;
	f << rot;
	f << scale;

	if(base)
		f << base->id;
	else
	{
		f.Write0();
		f << mesh->filename;
	}
}

//=================================================================================================
void Object::Load(GameReader& f)
{
	f >> pos;
	f >> rot;
	f >> scale;

	const string& base_id = f.ReadString1();
	if(!base_id.empty())
	{
		base = BaseObject::Get(base_id);
		mesh = base->mesh;
	}
	else
	{
		base = nullptr;
		mesh = res_mgr->Load<Mesh>(f.ReadString1());
	}
	if(f.is_local)
		CreateNode(f.area->scene);
}

//=================================================================================================
void Object::Write(BitStreamWriter& f) const
{
	f << pos;
	f << rot;
	f << scale;
	if(base)
		f << base->id;
	else
	{
		f.Write0();
		f << mesh->filename;
	}
}

//=================================================================================================
bool Object::Read(BitStreamReader& f)
{
	f >> pos;
	f >> rot;
	f >> scale;
	const string& base_id = f.ReadString1();
	if(!f)
		return false;
	if(!base_id.empty())
	{
		// use base obj
		base = BaseObject::Get(base_id);
		if(!base)
		{
			Error("Missing base object '%s'!", base_id.c_str());
			return false;
		}
		mesh = base->mesh;
	}
	else
	{
		// use mesh
		const string& mesh_id = f.ReadString1();
		if(!f)
			return false;
		mesh = res_mgr->Load<Mesh>(mesh_id);
		base = nullptr;
	}
	CreateNode(f.area->scene);
	return true;
}

#include "Pch.h"
#include "DestroyedObject.h"

#include "BaseUsable.h"
#include "BitStreamFunc.h"
#include "GameFile.h"

#include <SceneNode.h>

//=================================================================================================
SceneNode* DestroyedObject::CreateSceneNode()
{
	node = SceneNode::Get();
	node->SetMesh(base->mesh);
	node->center = pos;
	node->flags |= SceneNode::F_ALPHA_BLEND;
	node->tint = Vec4(1, 1, 1, timer);
	node->mat = Matrix::RotationY(rot) * Matrix::Translation(pos);
	node->addBlend = true;
	node->persistent = true;
	return node;
}

//=================================================================================================
bool DestroyedObject::Update(float dt)
{
	timer -= dt * 4;
	if(timer > 0)
	{
		node->tint.w = timer;
		return false;
	}
	return true;
}

//=================================================================================================
void DestroyedObject::Save(GameWriter& f)
{
	f << base->hash;
	f << pos;
	f << rot;
	f << timer;
}

//=================================================================================================
void DestroyedObject::Load(GameReader& f)
{
	base = BaseUsable::Get(f.Read<int>());
	f >> pos;
	f >> rot;
	f >> timer;
}

//=================================================================================================
void DestroyedObject::Write(BitStreamWriter& f) const
{
	f << base->hash;
	f << pos;
	f << rot;
	f << timer;
}

//=================================================================================================
bool DestroyedObject::Read(BitStreamReader& f)
{
	uint hash;
	f >> hash;
	f >> pos;
	f >> rot;
	f >> timer;
	if(!f)
		return false;
	base = BaseUsable::Get(hash);
	return true;
}

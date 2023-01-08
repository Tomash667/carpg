#include "Pch.h"
#include "Game.h"

#include "Ability.h"
#include "City.h"
#include "DestroyedObject.h"
#include "DungeonMeshBuilder.h"
#include "Explo.h"
#include "GameGui.h"
#include "GameMessages.h"
#include "GameResources.h"
#include "InsideLocation.h"
#include "Level.h"
#include "LevelPart.h"
#include "Pathfinding.h"
#include "PhysicCallbacks.h"
#include "Portal.h"

#include <Algorithm.h>
#include <BasicShader.h>
#include <DebugNode.h>
#include <DirectX.h>
#include <GlowShader.h>
#include <ParticleShader.h>
#include <ParticleSystem.h>
#include <PostfxShader.h>
#include <Render.h>
#include <ResourceManager.h>
#include <Scene.h>
#include <SceneManager.h>
#include <SkyboxShader.h>
#include <SoundManager.h>
#include <SuperShader.h>
#include <Terrain.h>
#include <TerrainShader.h>

//=================================================================================================
void Game::ListDrawObjects(LocationPart& locPart, FrustumPlanes& frustum)
{
	LevelPart& lvlPart = *locPart.lvlPart;

	drawBatch.Clear();
	drawBatch.locPart = &locPart;
	drawBatch.scene = lvlPart.scene;
	drawBatch.camera = &gameLevel->camera;
	drawBatch.gatherLights = !drawBatch.scene->useLightDir && sceneMgr->useLighting;
	ClearGrass();
	if(locPart.partType == LocationPart::Type::Outside)
	{
		ListQuadtreeNodes();
		ListGrass();
	}

	// existing nodes
	for(SceneNode* node : drawBatch.scene->nodes)
	{
		if(frustum.SphereToFrustum(node->center, node->radius))
		{
			if(drawBatch.gatherLights)
				GatherDrawBatchLights(node);
			drawBatch.Add(node);
		}
	}

	// terrain
	if(locPart.partType == LocationPart::Type::Outside && IsSet(drawFlags, DF_TERRAIN))
		gameLevel->terrain->ListVisibleParts(drawBatch.terrainParts, frustum);

	// dungeon
	if(locPart.partType == LocationPart::Type::Inside && IsSet(drawFlags, DF_TERRAIN))
		dungeonMeshBuilder->ListVisibleParts(drawBatch, frustum);

	// units
	if(IsSet(drawFlags, DF_UNITS))
	{
		for(Unit* unit : locPart.units)
			ListDrawObjectsUnit(frustum, *unit);
	}

	// objects
	if(IsSet(drawFlags, DF_OBJECTS))
	{
		if(locPart.partType == LocationPart::Type::Outside)
		{
			for(LevelQuad* quad : levelQuads)
			{
				for(QuadObj& obj : quad->objects)
				{
					const Object& o = *obj.obj;
					o.mesh->EnsureIsLoaded();
					if(frustum.SphereToFrustum(o.pos, o.GetRadius()))
						AddObjectToDrawBatch(frustum, o);
				}
			}
		}
		else
		{
			for(vector<Object*>::iterator it = locPart.objects.begin(), end = locPart.objects.end(); it != end; ++it)
			{
				const Object& o = **it;
				o.mesh->EnsureIsLoaded();
				if(frustum.SphereToFrustum(o.pos, o.GetRadius()))
					AddObjectToDrawBatch(frustum, o);
			}
		}
	}

	// usable objects
	if(IsSet(drawFlags, DF_USABLES))
	{
		for(vector<Usable*>::iterator it = locPart.usables.begin(), end = locPart.usables.end(); it != end; ++it)
		{
			Usable& use = **it;
			Mesh* mesh = use.GetMesh();
			mesh->EnsureIsLoaded();
			if(frustum.SphereToFrustum(use.pos, mesh->head.radius))
			{
				SceneNode* node = SceneNode::Get();
				node->SetMesh(mesh);
				node->center = use.pos;
				node->mat = Matrix::RotationY(use.rot) * Matrix::Translation(use.pos);
				if(drawBatch.gatherLights)
					GatherDrawBatchLights(node);
				if(pc->data.beforePlayer == BP_USABLE && pc->data.beforePlayerPtr.usable == &use)
				{
					if(useGlow)
					{
						GlowNode& glow = Add1(drawBatch.glowNodes);
						glow.node = node;
						glow.color = Color::White;
					}
					else
						node->tint = Vec4(2, 2, 2, 1);
				}
				drawBatch.Add(node);
			}
		}
	}

	// chests
	if(IsSet(drawFlags, DF_USABLES))
	{
		for(vector<Chest*>::iterator it = locPart.chests.begin(), end = locPart.chests.end(); it != end; ++it)
		{
			Chest& chest = **it;
			chest.meshInst->mesh->EnsureIsLoaded();
			if(frustum.SphereToFrustum(chest.pos, chest.meshInst->mesh->head.radius))
			{
				SceneNode* node = SceneNode::Get();
				if(chest.meshInst->IsActive())
				{
					chest.meshInst->SetupBones();
					node->SetMesh(chest.meshInst);
				}
				else
					node->SetMesh(chest.meshInst->mesh);
				node->center = chest.pos;
				node->mat = Matrix::RotationY(chest.rot) * Matrix::Translation(chest.pos);
				if(drawBatch.gatherLights)
					GatherDrawBatchLights(node);
				if(pc->data.beforePlayer == BP_CHEST && pc->data.beforePlayerPtr.chest == &chest)
				{
					if(useGlow)
					{
						GlowNode& glow = Add1(drawBatch.glowNodes);
						glow.node = node;
						glow.color = Color::White;
					}
					else
						node->tint = Vec4(2, 2, 2, 1);
				}
				drawBatch.Add(node);
			}
		}
	}

	// doors
	if(IsSet(drawFlags, DF_USABLES))
	{
		for(vector<Door*>::iterator it = locPart.doors.begin(), end = locPart.doors.end(); it != end; ++it)
		{
			Door& door = **it;
			door.meshInst->mesh->EnsureIsLoaded();
			if(frustum.SphereToFrustum(door.pos, door.meshInst->mesh->head.radius))
			{
				SceneNode* node = SceneNode::Get();
				if(!door.meshInst->groups[0].anim || door.meshInst->groups[0].time == 0.f)
					node->SetMesh(door.meshInst->mesh);
				else
				{
					door.meshInst->SetupBones();
					node->SetMesh(door.meshInst);
				}
				node->center = door.pos;
				node->mat = Matrix::RotationY(door.rot) * Matrix::Translation(door.pos);
				if(drawBatch.gatherLights)
					GatherDrawBatchLights(node);
				if(pc->data.beforePlayer == BP_DOOR && pc->data.beforePlayerPtr.door == &door)
				{
					if(useGlow)
					{
						GlowNode& glow = Add1(drawBatch.glowNodes);
						glow.node = node;
						glow.color = Color::White;
					}
					else
						node->tint = Vec4(2, 2, 2, 1);
				}
				drawBatch.Add(node);
			}
		}
	}

	// bloods
	if(IsSet(drawFlags, DF_BLOOD))
	{
		for(Blood& blood : locPart.bloods)
		{
			if(blood.size > 0.f && frustum.SphereToFrustum(blood.pos, blood.size * blood.scale))
			{
				if(drawBatch.gatherLights)
					GatherDrawBatchLights(nullptr, blood.pos.x, blood.pos.y, blood.size * blood.scale, 0, blood.lights);
				drawBatch.bloods.push_back(&blood);
			}
		}
	}

	// bullets
	if(IsSet(drawFlags, DF_BULLETS))
	{
		for(vector<Bullet*>::iterator it = lvlPart.bullets.begin(), end = lvlPart.bullets.end(); it != end; ++it)
		{
			Bullet& bullet = **it;
			if(bullet.mesh)
			{
				bullet.mesh->EnsureIsLoaded();
				if(frustum.SphereToFrustum(bullet.pos, bullet.mesh->head.radius))
				{
					SceneNode* node = SceneNode::Get();
					node->SetMesh(bullet.mesh);
					node->center = bullet.pos;
					node->mat = Matrix::Rotation(bullet.rot) * Matrix::Translation(bullet.pos);
					if(drawBatch.gatherLights)
						GatherDrawBatchLights(node);
					drawBatch.Add(node);
				}
			}
			else
			{
				if(frustum.SphereToFrustum(bullet.pos, bullet.texSize))
				{
					Billboard& bb = Add1(drawBatch.billboards);
					bb.pos = bullet.pos;
					bb.size = bullet.texSize;
					bb.tex = bullet.tex;
				}
			}
		}
	}

	// traps
	if(IsSet(drawFlags, DF_TRAPS))
	{
		for(vector<Trap*>::iterator it = locPart.traps.begin(), end = locPart.traps.end(); it != end; ++it)
		{
			Trap& trap = **it;
			trap.base->mesh->EnsureIsLoaded();
			if((trap.state == 0 || (trap.base->type != TRAP_ARROW && trap.base->type != TRAP_POISON))
				&& frustum.SphereToFrustum(trap.pos, trap.base->mesh->head.radius))
			{
				SceneNode* node = SceneNode::Get();
				if(trap.meshInst && trap.meshInst->IsActive())
				{
					trap.meshInst->SetupBones();
					node->SetMesh(trap.meshInst);
				}
				else
					node->SetMesh(trap.base->mesh);
				node->center = trap.pos;
				node->mat = Matrix::RotationY(trap.rot) * Matrix::Translation(trap.pos);
				if(drawBatch.gatherLights)
					GatherDrawBatchLights(node);
				drawBatch.Add(node);
			}
		}
	}

	// explosions
	if(IsSet(drawFlags, DF_EXPLOS))
	{
		gameRes->aSpellball->EnsureIsLoaded();
		for(vector<Explo*>::iterator it = lvlPart.explos.begin(), end = lvlPart.explos.end(); it != end; ++it)
		{
			Explo& explo = **it;
			if(frustum.SphereToFrustum(explo.pos, explo.size))
			{
				SceneNode* node = SceneNode::Get();
				node->SetMesh(gameRes->aSpellball);
				node->flags |= SceneNode::F_NO_LIGHTING | SceneNode::F_ALPHA_BLEND | SceneNode::F_NO_ZWRITE;
				node->center = explo.pos;
				node->radius *= explo.size;
				node->mat = Matrix::Scale(explo.size) * Matrix::Translation(explo.pos);
				node->texOverride = &explo.ability->texExplode;
				node->tint = Vec4(1, 1, 1, 1.f - explo.size / explo.sizemax);
				node->addBlend = false;
				drawBatch.Add(node);
			}
		}
	}

	// destroyed objects
	for(DestroyedObject* obj : lvlPart.destroyedObjects)
	{
		Mesh* mesh = obj->base->mesh;
		mesh->EnsureIsLoaded();
		if(frustum.SphereToFrustum(obj->pos, mesh->head.radius))
		{
			SceneNode* node = SceneNode::Get();
			node->SetMesh(mesh);
			node->center = obj->pos;
			node->flags |= SceneNode::F_ALPHA_BLEND;
			node->tint = Vec4(1, 1, 1, obj->timer);
			node->mat = Matrix::RotationY(obj->rot) * Matrix::Translation(obj->pos);
			node->addBlend = true;
			if(drawBatch.gatherLights)
				GatherDrawBatchLights(node);
			drawBatch.Add(node);
		}
	}

	// particles
	if(IsSet(drawFlags, DF_PARTICLES))
	{
		for(vector<ParticleEmitter*>::iterator it = lvlPart.pes.begin(), end = lvlPart.pes.end(); it != end; ++it)
		{
			ParticleEmitter& pe = **it;
			if(pe.alive && frustum.SphereToFrustum(pe.pos, pe.radius))
			{
				drawBatch.pes.push_back(&pe);
				if(drawParticleSphere)
				{
					DebugNode* debugNode = DebugNode::Get();
					debugNode->mat = Matrix::Scale(pe.radius * 2) * Matrix::Translation(pe.pos) * drawBatch.camera->matViewProj;
					debugNode->shape = MeshShape::Sphere;
					debugNode->color = Color::Green;
					drawBatch.debugNodes.push_back(debugNode);
				}
			}
		}

		if(lvlPart.tpes.empty())
			drawBatch.tpes = nullptr;
		else
			drawBatch.tpes = &lvlPart.tpes;
	}
	else
		drawBatch.tpes = nullptr;

	// portals
	if(IsSet(drawFlags, DF_PORTALS) && locPart.partType != LocationPart::Type::Building)
	{
		gameRes->aPortal->EnsureIsLoaded();
		Portal* portal = gameLevel->location->portal;
		while(portal)
		{
			if(gameLevel->location->outside || gameLevel->dungeonLevel == portal->atLevel)
			{
				SceneNode* node = SceneNode::Get();
				node->SetMesh(gameRes->aPortal);
				node->flags |= SceneNode::F_NO_LIGHTING | SceneNode::F_ALPHA_BLEND | SceneNode::F_NO_CULLING;
				node->center = portal->pos + Vec3(0, 0.67f + 0.305f, 0);
				node->mat = Matrix::Rotation(0, portal->rot, -portalAnim * PI * 2) * Matrix::Translation(node->center);
				node->addBlend = false;
				drawBatch.Add(node);
			}
			portal = portal->nextPortal;
		}
	}

	// areas
	if(IsSet(drawFlags, DF_AREA))
		ListAreas(locPart);

	// colliders
	if(drawCol)
	{
		for(vector<CollisionObject>::iterator it = lvlPart.colliders.begin(), end = lvlPart.colliders.end(); it != end; ++it)
		{
			MeshShape shape = MeshShape::None;
			Vec3 scale;
			float rot = 0.f;

			switch(it->type)
			{
			case CollisionObject::RECTANGLE:
				scale = Vec3(it->w, 1, it->h);
				shape = MeshShape::Box;
				break;
			case CollisionObject::RECTANGLE_ROT:
				scale = Vec3(it->w, 1, it->h);
				shape = MeshShape::Box;
				rot = it->rot;
				break;
			case CollisionObject::SPHERE:
				scale = Vec3(it->radius, 1, it->radius);
				shape = MeshShape::Cylinder;
				break;
			default:
				break;
			}

			if(shape != MeshShape::None)
			{
				DebugNode* node = DebugNode::Get();
				node->shape = shape;
				node->color = Color(153, 217, 164);
				node->mat = Matrix::Scale(scale) * Matrix::RotationY(rot) * Matrix::Translation(it->pos) * drawBatch.camera->matViewProj;
				drawBatch.debugNodes.push_back(node);
			}
		}
	}

	// physics
	if(drawPhy)
	{
		const btCollisionObjectArray& cobjs = phyWorld->getCollisionObjectArray();
		const int count = cobjs.size();

		for(int i = 0; i < count; ++i)
		{
			const btCollisionObject* cobj = cobjs[i];
			const btCollisionShape* shape = cobj->getCollisionShape();
			Matrix mWorld;
			cobj->getWorldTransform().getOpenGLMatrix(&mWorld._11);

			switch(shape->getShapeType())
			{
			case BOX_SHAPE_PROXYTYPE:
				{
					const btBoxShape* box = (const btBoxShape*)shape;
					DebugNode* node = DebugNode::Get();
					node->shape = MeshShape::Box;
					node->color = Color(163, 73, 164);
					node->mat = Matrix::Scale(ToVec3(box->getHalfExtentsWithMargin())) * mWorld * drawBatch.camera->matViewProj;
					drawBatch.debugNodes.push_back(node);
				}
				break;
			case CAPSULE_SHAPE_PROXYTYPE:
				{
					const btCapsuleShape* capsule = (const btCapsuleShape*)shape;
					float r = capsule->getRadius();
					float h = capsule->getHalfHeight();
					DebugNode* node = DebugNode::Get();
					node->shape = MeshShape::Capsule;
					node->color = Color(163, 73, 164);
					node->mat = Matrix::Scale(r, h + r, r) * mWorld * drawBatch.camera->matViewProj;
					drawBatch.debugNodes.push_back(node);
				}
				break;
			case CYLINDER_SHAPE_PROXYTYPE:
				{
					const btCylinderShape* cylinder = (const btCylinderShape*)shape;
					DebugNode* node = DebugNode::Get();
					node->shape = MeshShape::Cylinder;
					node->color = Color(163, 73, 164);
					Vec3 v = ToVec3(cylinder->getHalfExtentsWithoutMargin());
					node->mat = Matrix::Scale(v.x, v.y / 2, v.z) * mWorld * drawBatch.camera->matViewProj;
					drawBatch.debugNodes.push_back(node);
				}
				break;
			case TERRAIN_SHAPE_PROXYTYPE:
				break;
			case COMPOUND_SHAPE_PROXYTYPE:
				{
					const btCompoundShape* compound = (const btCompoundShape*)shape;
					int count = compound->getNumChildShapes();

					for(int i = 0; i < count; ++i)
					{
						const btCollisionShape* child = compound->getChildShape(i);
						if(child->getShapeType() == BOX_SHAPE_PROXYTYPE)
						{
							btBoxShape* box = (btBoxShape*)child;
							DebugNode* node = DebugNode::Get();
							node->shape = MeshShape::Box;
							node->color = Color(163, 73, 164);
							Matrix mChild;
							compound->getChildTransform(i).getOpenGLMatrix(&mChild._11);
							node->mat = Matrix::Scale(ToVec3(box->getHalfExtentsWithMargin())) * mChild * mWorld * drawBatch.camera->matViewProj;
							drawBatch.debugNodes.push_back(node);
						}
						else
						{
							// TODO
							assert(0);
						}
					}
				}
				break;
			case TRIANGLE_MESH_SHAPE_PROXYTYPE:
				{
					DebugNode* node = DebugNode::Get();
					node->shape = MeshShape::TriMesh;
					node->color = Color(163, 73, 164);
					node->mat = mWorld * drawBatch.camera->matViewProj;
					node->trimesh = reinterpret_cast<SimpleMesh*>(shape->getUserPointer());
					drawBatch.debugNodes.push_back(node);
				}
			default:
				break;
			}
		}
	}

	// glow
	drawBatch.tmpGlow = nullptr;
	if(PlayerController::data.beforePlayer == BP_ITEM)
	{
		GroundItem* groundItem = PlayerController::data.beforePlayerPtr.item;
		if(useGlow)
		{
			GlowNode& glow = Add1(drawBatch.glowNodes);
			glow.node = groundItem->node;
			glow.color = Color::White;
		}
		else
		{
			groundItem->node->tint = Vec4(2, 2, 2, 1);
			drawBatch.tmpGlow = groundItem->node;
		}
	}

	drawBatch.Process();
}

//=================================================================================================
void Game::ListDrawObjectsUnit(FrustumPlanes& frustum, Unit& u)
{
	u.meshInst->mesh->EnsureIsLoaded();
	if(!frustum.SphereToFrustum(u.visualPos, u.GetSphereRadius()))
		return;

	// add stun effect
	if(u.IsAlive())
	{
		Effect* effect = u.FindEffect(EffectId::Stun);
		if(effect)
		{
			SceneNode* node = SceneNode::Get();
			node->SetMesh(gameRes->aStun);
			node->flags |= SceneNode::F_NO_LIGHTING | SceneNode::F_ALPHA_BLEND | SceneNode::F_NO_CULLING | SceneNode::F_NO_ZWRITE;
			node->center = u.GetHeadPoint();
			node->mat = Matrix::Scale(u.data->scale) * Matrix::RotationY(portalAnim * PI * 2) * Matrix::Translation(node->center);
			node->addBlend = false;
			drawBatch.Add(node);
		}

		if(u.HaveEffect(EffectId::Rooted, EffectValue_Rooted_Vines))
		{
			const float scale = u.GetRadius() / 0.3f;
			SceneNode* node = SceneNode::Get();
			node->SetMesh(gameRes->mVine);
			node->center = u.pos;
			node->radius *= scale;
			node->mat = Matrix::Scale(scale) * Matrix::Translation(node->center);
			if(drawBatch.gatherLights)
				GatherDrawBatchLights(node);
			drawBatch.Add(node);
		}
	}

	// set bones
	u.meshInst->SetupBones();

	bool selected = (pc->data.beforePlayer == BP_UNIT && pc->data.beforePlayerPtr.unit == &u)
		|| (gameState == GS_LEVEL && ((pc->data.abilityReady && pc->data.abilityOk && pc->data.abilityTarget == u)
			|| (pc->unit->action == A_CAST && pc->unit->act.cast.target == u)));

	// add scene node
	SceneNode* node = SceneNode::Get();
	node->SetMesh(u.meshInst);
	if(IsSet(u.data->flags2, F2_ALPHA_BLEND))
		node->flags |= SceneNode::F_ALPHA_BLEND;
	node->center = u.visualPos;
	node->mat = Matrix::Scale(u.data->scale) * Matrix::RotationY(u.rot) * Matrix::Translation(u.visualPos);
	node->texOverride = u.data->GetTextureOverride();
	node->tint = u.data->tint;
	node->addBlend = false;

	// light settings
	if(drawBatch.gatherLights)
		GatherDrawBatchLights(node);
	if(selected)
	{
		if(useGlow)
		{
			GlowNode& glow = Add1(drawBatch.glowNodes);
			glow.node = node;
			glow.color = pc->unit->GetRelationColor(u);
		}
		else
		{
			node->tint.x *= 2;
			node->tint.y *= 2;
			node->tint.z *= 2;
		}
	}
	drawBatch.Add(node);
	if(u.HaveArmor() && u.GetArmor().armorUnitType == ArmorUnitType::HUMAN && u.GetArmor().mesh)
		node->subs = Bit(1) | Bit(2);

	// armor
	if(u.HaveArmor() && u.GetArmor().mesh)
	{
		const Armor& armor = u.GetArmor();
		SceneNode* node2 = SceneNode::Get();
		node2->SetMesh(armor.mesh, u.meshInst);
		node2->center = node->center;
		node2->mat = node->mat;
		node2->texOverride = armor.GetTextureOverride();
		node2->lights = node->lights;
		if(selected)
		{
			if(useGlow)
			{
				GlowNode& glow = Add1(drawBatch.glowNodes);
				glow.node = node2;
				glow.color = pc->unit->GetRelationColor(u);
			}
			else
				node2->tint = Vec4(2, 2, 2, 1);
		}
		drawBatch.Add(node2);
	}

	// item in hand
	Mesh* rightHandItem = nullptr;
	bool inHand = false;

	switch(u.weaponState)
	{
	case WeaponState::Hidden:
		break;
	case WeaponState::Taken:
		if(u.weaponTaken == W_BOW)
		{
			if(u.action == A_SHOOT)
			{
				if(u.animationState != AS_SHOOT_SHOT)
					rightHandItem = gameRes->aArrow;
			}
			else
				rightHandItem = gameRes->aArrow;
		}
		else if(u.weaponTaken == W_ONE_HANDED)
			inHand = true;
		break;
	case WeaponState::Taking:
		if(u.animationState == AS_TAKE_WEAPON_MOVED)
		{
			if(u.weaponTaken == W_BOW)
				rightHandItem = gameRes->aArrow;
			else
				inHand = true;
		}
		break;
	case WeaponState::Hiding:
		if(u.animationState == AS_TAKE_WEAPON_START)
		{
			if(u.weaponHiding == W_BOW)
				rightHandItem = gameRes->aArrow;
			else
				inHand = true;
		}
		break;
	}

	if(u.usedItem && u.action != A_USE_ITEM)
		rightHandItem = u.usedItem->mesh;

	if(rightHandItem == gameRes->aArrow && u.action == A_SHOOT && u.act.shoot.ability && u.act.shoot.ability->mesh)
		rightHandItem = u.act.shoot.ability->mesh;

	Matrix matScale;
	if(u.humanData)
	{
		Vec2 scale = u.humanData->GetScale();
		scale.x = 1.f / scale.x;
		scale.y = 1.f / scale.y;
		matScale = Matrix::Scale(scale.x, scale.y, scale.x);
	}
	else
		matScale = Matrix::IdentityMatrix;

	// weapon
	Mesh* mesh;
	if(u.HaveWeapon() && rightHandItem != (mesh = u.GetWeapon().mesh))
	{
		Mesh::Point* point = u.meshInst->mesh->GetPoint(inHand ? NAMES::pointWeapon : NAMES::pointHiddenWeapon);
		assert(point);

		SceneNode* node2 = SceneNode::Get();
		node2->SetMesh(mesh);
		node2->center = node->center;
		node2->mat = matScale * point->mat * u.meshInst->matBones[point->bone] * node->mat;
		node2->lights = node->lights;
		if(selected)
		{
			if(useGlow)
			{
				GlowNode& glow = Add1(drawBatch.glowNodes);
				glow.node = node2;
				glow.color = pc->unit->GetRelationColor(u);
			}
			else
				node2->tint = Vec4(2, 2, 2, 1);
		}
		drawBatch.Add(node2);

		// weapon hitbox
		if(drawHitbox && u.weaponState == WeaponState::Taken && u.weaponTaken == W_ONE_HANDED)
		{
			Mesh::Point* box = mesh->FindPoint("hit");
			assert(box && box->IsBox());

			DebugNode* debugNode = DebugNode::Get();
			debugNode->mat = box->mat * node2->mat * drawBatch.camera->matViewProj;
			debugNode->shape = MeshShape::Box;
			debugNode->color = Color::Black;
			drawBatch.debugNodes.push_back(debugNode);
		}
	}
	else if(u.action == A_ATTACK && drawHitbox)
	{
		Mesh::Point* hitbox = u.meshInst->mesh->GetPoint(Format("hitbox%d", u.act.attack.index + 1));
		if(!hitbox)
			hitbox = u.meshInst->mesh->FindPoint("hitbox");
		DebugNode* debugNode = DebugNode::Get();
		debugNode->mat = hitbox->mat * u.meshInst->matBones[hitbox->bone] * node->mat * drawBatch.camera->matViewProj;
		debugNode->shape = MeshShape::Box;
		debugNode->color = Color::Black;
		drawBatch.debugNodes.push_back(debugNode);
	}

	// shield
	if(u.HaveShield() && u.GetShield().mesh)
	{
		Mesh* shield = u.GetShield().mesh;
		Mesh::Point* point = u.meshInst->mesh->GetPoint(inHand ? NAMES::pointShield : NAMES::pointShieldHidden);
		assert(point);

		SceneNode* node2 = SceneNode::Get();
		node2->SetMesh(shield);
		node2->center = node->center;
		node2->mat = matScale * point->mat * u.meshInst->matBones[point->bone] * node->mat;
		node2->lights = node->lights;
		if(selected)
		{
			if(useGlow)
			{
				GlowNode& glow = Add1(drawBatch.glowNodes);
				glow.node = node2;
				glow.color = pc->unit->GetRelationColor(u);
			}
			else
				node2->tint = Vec4(2, 2, 2, 1);
		}
		drawBatch.Add(node2);

		// shield hitbox
		if(drawHitbox && u.weaponState == WeaponState::Taken && u.weaponTaken == W_ONE_HANDED)
		{
			Mesh::Point* box = shield->FindPoint("hit");
			assert(box && box->IsBox());

			DebugNode* debugNode = DebugNode::Get();
			debugNode->mat = box->mat * node2->mat * drawBatch.camera->matViewProj;
			debugNode->shape = MeshShape::Box;
			debugNode->color = Color::Black;
			drawBatch.debugNodes.push_back(debugNode);
		}
	}

	// other item (arrow/potion)
	if(rightHandItem)
	{
		Mesh::Point* point = u.meshInst->mesh->GetPoint(NAMES::pointWeapon);
		assert(point);

		SceneNode* node2 = SceneNode::Get();
		node2->SetMesh(rightHandItem);
		node2->center = node->center;
		node2->mat = matScale * point->mat * u.meshInst->matBones[point->bone] * node->mat;
		node2->lights = node->lights;
		if(selected)
		{
			if(useGlow)
			{
				GlowNode& glow = Add1(drawBatch.glowNodes);
				glow.node = node2;
				glow.color = pc->unit->GetRelationColor(u);
			}
			else
				node2->tint = Vec4(2, 2, 2, 1);
		}
		drawBatch.Add(node2);
	}

	// bow
	if(u.HaveBow())
	{
		bool inHand;

		switch(u.weaponState)
		{
		case WeaponState::Hiding:
			inHand = (u.weaponHiding == W_BOW && u.animationState == AS_TAKE_WEAPON_START);
			break;
		case WeaponState::Hidden:
			inHand = false;
			break;
		case WeaponState::Taking:
			inHand = (u.weaponTaken == W_BOW && u.animationState == AS_TAKE_WEAPON_MOVED);
			break;
		case WeaponState::Taken:
			inHand = (u.weaponTaken == W_BOW);
			break;
		}

		SceneNode* node2 = SceneNode::Get();
		if(u.action == A_SHOOT)
		{
			u.bowInstance->SetupBones();
			node2->SetMesh(u.bowInstance);
		}
		else
			node2->SetMesh(u.GetBow().mesh);
		node2->center = node->center;
		Mesh::Point* point = u.meshInst->mesh->GetPoint(inHand ? NAMES::pointBow : NAMES::pointShieldHidden);
		assert(point);
		Matrix m1;
		if(inHand)
			m1 = Matrix::RotationZ(-PI / 2) * point->mat * u.meshInst->matBones[point->bone];
		else
			m1 = point->mat * u.meshInst->matBones[point->bone];
		node2->mat = matScale * m1 * node->mat;
		node2->lights = node->lights;
		if(selected)
		{
			if(useGlow)
			{
				GlowNode& glow = Add1(drawBatch.glowNodes);
				glow.node = node2;
				glow.color = pc->unit->GetRelationColor(u);
			}
			else
				node2->tint = Vec4(2, 2, 2, 1);
		}
		drawBatch.Add(node2);
	}

	// hair/beard/eyebrows for humans
	if(u.data->type == UNIT_TYPE::HUMAN)
	{
		Human& h = *u.humanData;

		// eyebrows
		SceneNode* node2 = SceneNode::Get();
		node2->SetMesh(gameRes->aEyebrows, node->meshInst);
		node2->center = node->center;
		node2->mat = node->mat;
		node2->tint = h.hairColor * u.data->tint;
		node2->lights = node->lights;
		if(selected)
		{
			if(useGlow)
			{
				GlowNode& glow = Add1(drawBatch.glowNodes);
				glow.node = node2;
				glow.color = pc->unit->GetRelationColor(u);
			}
			else
			{
				node2->tint.x *= 2;
				node2->tint.y *= 2;
				node2->tint.z *= 2;
			}
		}
		drawBatch.Add(node2);

		// hair
		if(h.hair != -1)
		{
			SceneNode* node3 = SceneNode::Get();
			node3->SetMesh(gameRes->aHair[h.hair], node->meshInst);
			node3->center = node->center;
			node3->mat = node->mat;
			node3->tint = h.hairColor * u.data->tint;
			node3->lights = node->lights;
			if(selected)
			{
				if(useGlow)
				{
					GlowNode& glow = Add1(drawBatch.glowNodes);
					glow.node = node3;
					glow.color = pc->unit->GetRelationColor(u);
				}
				else
				{
					node3->tint.x *= 2;
					node3->tint.y *= 2;
					node3->tint.z *= 2;
				}
			}
			drawBatch.Add(node3);
		}

		// beard
		if(h.beard != -1)
		{
			SceneNode* node3 = SceneNode::Get();
			node3->SetMesh(gameRes->aBeard[h.beard], node->meshInst);
			node3->center = node->center;
			node3->mat = node->mat;
			node3->tint = h.hairColor * u.data->tint;
			node3->lights = node->lights;
			if(selected)
			{
				if(useGlow)
				{
					GlowNode& glow = Add1(drawBatch.glowNodes);
					glow.node = node3;
					glow.color = pc->unit->GetRelationColor(u);
				}
				else
				{
					node3->tint.x *= 2;
					node3->tint.y *= 2;
					node3->tint.z *= 2;
				}
			}
			drawBatch.Add(node3);
		}

		// mustache
		if(h.mustache != -1 && (h.beard == -1 || !gBeardAndMustache[h.beard]))
		{
			SceneNode* node3 = SceneNode::Get();
			node3->SetMesh(gameRes->aMustache[h.mustache], node->meshInst);
			node3->center = node->center;
			node3->mat = node->mat;
			node3->tint = h.hairColor * u.data->tint;
			node3->lights = node->lights;
			if(selected)
			{
				if(useGlow)
				{
					GlowNode& glow = Add1(drawBatch.glowNodes);
					glow.node = node3;
					glow.color = pc->unit->GetRelationColor(u);
				}
				else
				{
					node3->tint.x *= 2;
					node3->tint.y *= 2;
					node3->tint.z *= 2;
				}
			}
			drawBatch.Add(node3);
		}
	}

	// unit hitbox radius
	if(drawUnitRadius)
	{
		float h = u.GetHeight() / 2;
		DebugNode* debugNode = DebugNode::Get();
		debugNode->mat = Matrix::Scale(u.GetRadius(), h, u.GetRadius()) * Matrix::Translation(u.GetColliderPos() + Vec3(0, h, 0)) * drawBatch.camera->matViewProj;
		debugNode->shape = MeshShape::Cylinder;
		debugNode->color = Color::White;
		drawBatch.debugNodes.push_back(debugNode);
	}
	if(drawHitbox)
	{
		float h = u.GetHeight() / 2;
		Box box;
		u.GetBox(box);
		DebugNode* debugNode = DebugNode::Get();
		debugNode->mat = Matrix::Scale(box.SizeX() / 2, h, box.SizeZ() / 2) * Matrix::Translation(u.pos + Vec3(0, h, 0)) * drawBatch.camera->matViewProj;
		debugNode->shape = MeshShape::Box;
		debugNode->color = Color::Black;
		drawBatch.debugNodes.push_back(debugNode);
	}
}

//=================================================================================================
void Game::AddObjectToDrawBatch(FrustumPlanes& frustum, const Object& o)
{
	SceneNode* node = SceneNode::Get();
	node->mat = Matrix::Transform(o.pos, o.rot, o.scale);

	if(o.meshInst)
	{
		o.meshInst->SetupBones();
		node->SetMesh(o.meshInst);
	}
	else
		node->SetMesh(o.mesh);
	if(o.RequireNoCulling())
		node->flags |= SceneNode::F_NO_CULLING;
	if(!IsSet(o.mesh->head.flags, Mesh::F_SPLIT))
	{
		node->center = o.pos;
		node->radius = o.GetRadius();
		if(drawBatch.gatherLights)
			GatherDrawBatchLights(node);
		drawBatch.Add(node);
	}
	else
	{
		Mesh& mesh = *o.mesh;
		if(IsSet(mesh.head.flags, Mesh::F_TANGENTS))
			node->flags |= SceneNode::F_HAVE_TANGENTS;

		// for simplicity original node in unused and freed at end
		for(int i = 0; i < mesh.head.nSubs; ++i)
		{
			const Vec3 pos = Vec3::Transform(mesh.splits[i].pos, node->mat);
			const float radius = mesh.splits[i].radius * o.scale;
			if(frustum.SphereToFrustum(pos, radius))
			{
				SceneNode* node2 = SceneNode::Get();
				node2->mesh = node->mesh;
				node2->meshInst = node->meshInst;
				node2->flags = node->flags;
				node2->center = pos;
				node2->radius = radius;
				node2->mat = node->mat;
				node2->tint = node->tint;
				node2->texOverride = node->texOverride;
				drawBatch.Add(node2, i);
				if(drawBatch.gatherLights)
					GatherDrawBatchLights(node2);
			}
		}

		node->Free();
	}
}

//=================================================================================================
void Game::ListAreas(LocationPart& locPart)
{
	if(locPart.partType == LocationPart::Type::Outside)
	{
		if(gameLevel->cityCtx)
		{
			if(IsSet(gameLevel->cityCtx->flags, City::HaveExit))
			{
				for(vector<EntryPoint>::const_iterator entryId = gameLevel->cityCtx->entryPoints.begin(), entryEnd = gameLevel->cityCtx->entryPoints.end();
					entryId != entryEnd; ++entryId)
				{
					const EntryPoint& e = *entryId;
					Area& a = Add1(drawBatch.areas);
					a.v[0] = Vec3(e.exitRegion.v1.x, e.exitY, e.exitRegion.v2.y);
					a.v[1] = Vec3(e.exitRegion.v2.x, e.exitY, e.exitRegion.v2.y);
					a.v[2] = Vec3(e.exitRegion.v1.x, e.exitY, e.exitRegion.v1.y);
					a.v[3] = Vec3(e.exitRegion.v2.x, e.exitY, e.exitRegion.v1.y);
				}
			}

			for(vector<InsideBuilding*>::const_iterator it = gameLevel->cityCtx->insideBuildings.begin(), end = gameLevel->cityCtx->insideBuildings.end(); it != end; ++it)
			{
				const InsideBuilding& ib = **it;
				if(ib.canEnter)
				{
					Area& a = Add1(drawBatch.areas);
					a.v[0] = Vec3(ib.enterRegion.v1.x, ib.enterY, ib.enterRegion.v2.y);
					a.v[1] = Vec3(ib.enterRegion.v2.x, ib.enterY, ib.enterRegion.v2.y);
					a.v[2] = Vec3(ib.enterRegion.v1.x, ib.enterY, ib.enterRegion.v1.y);
					a.v[3] = Vec3(ib.enterRegion.v2.x, ib.enterY, ib.enterRegion.v1.y);
				}
			}
		}

		if(!gameLevel->cityCtx || !IsSet(gameLevel->cityCtx->flags, City::HaveExit))
		{
			const float H1 = -10.f;
			const float H2 = 30.f;

			// up
			{
				Area& a = Add1(drawBatch.areas);
				a.v[0] = Vec3(33.f, H1, 256.f - 33.f);
				a.v[1] = Vec3(33.f, H2, 256.f - 33.f);
				a.v[2] = Vec3(256.f - 33.f, H1, 256.f - 33.f);
				a.v[3] = Vec3(256.f - 33.f, H2, 256.f - 33.f);
			}

			// bottom
			{
				Area& a = Add1(drawBatch.areas);
				a.v[0] = Vec3(33.f, H1, 33.f);
				a.v[1] = Vec3(256.f - 33.f, H1, 33.f);
				a.v[2] = Vec3(33.f, H2, 33.f);
				a.v[3] = Vec3(256.f - 33.f, H2, 33.f);
			}

			// left
			{
				Area& a = Add1(drawBatch.areas);
				a.v[0] = Vec3(33.f, H1, 33.f);
				a.v[1] = Vec3(33.f, H2, 33.f);
				a.v[2] = Vec3(33.f, H1, 256.f - 33.f);
				a.v[3] = Vec3(33.f, H2, 256.f - 33.f);
			}

			// right
			{
				Area& a = Add1(drawBatch.areas);
				a.v[0] = Vec3(256.f - 33.f, H1, 256.f - 33.f);
				a.v[1] = Vec3(256.f - 33.f, H2, 256.f - 33.f);
				a.v[2] = Vec3(256.f - 33.f, H1, 33.f);
				a.v[3] = Vec3(256.f - 33.f, H2, 33.f);
			}
		}
		drawBatch.areaRange = 10.f;
	}
	else if(locPart.partType == LocationPart::Type::Inside)
	{
		InsideLocation* inside = static_cast<InsideLocation*>(gameLevel->location);
		InsideLocationLevel& lvl = inside->GetLevelData();

		if(inside->HavePrevEntry())
			ListEntry(lvl.prevEntryType, lvl.prevEntryPt, lvl.prevEntryDir);
		if(inside->HaveNextEntry())
			ListEntry(lvl.nextEntryType, lvl.nextEntryPt, lvl.nextEntryDir);
		drawBatch.areaRange = 5.f;
	}
	else
	{
		// exit from building
		Area& a = Add1(drawBatch.areas);
		const Box2d& region = static_cast<InsideBuilding&>(locPart).exitRegion;
		a.v[0] = Vec3(region.v1.x, 0.1f, region.v2.y);
		a.v[1] = Vec3(region.v2.x, 0.1f, region.v2.y);
		a.v[2] = Vec3(region.v1.x, 0.1f, region.v1.y);
		a.v[3] = Vec3(region.v2.x, 0.1f, region.v1.y);
		drawBatch.areaRange = 5.f;
	}

	// action area2
	if(pc->data.abilityReady || (pc->unit->action == A_CAST
		&& pc->unit->animationState == AS_CAST_ANIMATION && Any(pc->unit->act.cast.ability->type, Ability::Summon, Ability::Trap)))
		PrepareAreaPath();
	else if(pc->unit->action == A_CAST && Any(pc->unit->act.cast.ability->type, Ability::Point, Ability::Ray))
		pc->unit->targetPos = pc->RaytestTarget(pc->unit->act.cast.ability->range);
	else if(pc->unit->weaponState == WeaponState::Taken
		&& ((pc->unit->weaponTaken == W_BOW && Any(pc->unit->action, A_NONE, A_SHOOT))
			|| (pc->unit->weaponTaken == W_ONE_HANDED && IsSet(pc->unit->GetWeapon().flags, ITEM_WAND)
				&& Any(pc->unit->action, A_NONE, A_ATTACK, A_CAST, A_BLOCK, A_BASH))))
		pc->unit->targetPos = pc->RaytestTarget(50.f);

	if(drawHitbox && pc->ShouldUseRaytest())
	{
		Vec3 pos;
		if(pc->data.abilityReady)
			pos = pc->data.abilityPoint;
		else
			pos = pc->unit->targetPos;
		DebugNode* node = DebugNode::Get();
		node->mat = Matrix::Scale(0.25f) * Matrix::Translation(pos) * drawBatch.camera->matViewProj;
		node->shape = MeshShape::Sphere;
		node->color = Color::Green;
		drawBatch.debugNodes.push_back(node);
	}
}

//=================================================================================================
void Game::ListEntry(EntryType type, const Int2& pt, GameDirection dir)
{
	Area& a = Add1(drawBatch.areas);
	a.v[0] = a.v[1] = a.v[2] = a.v[3] = PtToPos(pt);
	switch(type)
	{
	case ENTRY_STAIRS_UP:
		switch(dir)
		{
		case GDIR_DOWN:
			a.v[0] += Vec3(-0.85f, 2.87f, 0.85f);
			a.v[1] += Vec3(0.85f, 2.87f, 0.85f);
			a.v[2] += Vec3(-0.85f, 0.83f, 0.85f);
			a.v[3] += Vec3(0.85f, 0.83f, 0.85f);
			break;
		case GDIR_UP:
			a.v[0] += Vec3(0.85f, 2.87f, -0.85f);
			a.v[1] += Vec3(-0.85f, 2.87f, -0.85f);
			a.v[2] += Vec3(0.85f, 0.83f, -0.85f);
			a.v[3] += Vec3(-0.85f, 0.83f, -0.85f);
			break;
		case GDIR_RIGHT:
			a.v[0] += Vec3(-0.85f, 2.87f, -0.85f);
			a.v[1] += Vec3(-0.85f, 2.87f, 0.85f);
			a.v[2] += Vec3(-0.85f, 0.83f, -0.85f);
			a.v[3] += Vec3(-0.85f, 0.83f, 0.85f);
			break;
		case GDIR_LEFT:
			a.v[0] += Vec3(0.85f, 2.87f, 0.85f);
			a.v[1] += Vec3(0.85f, 2.87f, -0.85f);
			a.v[2] += Vec3(0.85f, 0.83f, 0.85f);
			a.v[3] += Vec3(0.85f, 0.83f, -0.85f);
			break;
		}
		break;
	case ENTRY_STAIRS_DOWN:
	case ENTRY_STAIRS_DOWN_IN_WALL:
		switch(dir)
		{
		case GDIR_DOWN:
			a.v[0] += Vec3(-0.85f, 0.45f, 0.85f);
			a.v[1] += Vec3(0.85f, 0.45f, 0.85f);
			a.v[2] += Vec3(-0.85f, -1.55f, 0.85f);
			a.v[3] += Vec3(0.85f, -1.55f, 0.85f);
			break;
		case GDIR_UP:
			a.v[0] += Vec3(0.85f, 0.45f, -0.85f);
			a.v[1] += Vec3(-0.85f, 0.45f, -0.85f);
			a.v[2] += Vec3(0.85f, -1.55f, -0.85f);
			a.v[3] += Vec3(-0.85f, -1.55f, -0.85f);
			break;
		case GDIR_RIGHT:
			a.v[0] += Vec3(-0.85f, 0.45f, -0.85f);
			a.v[1] += Vec3(-0.85f, 0.45f, 0.85f);
			a.v[2] += Vec3(-0.85f, -1.55f, -0.85f);
			a.v[3] += Vec3(-0.85f, -1.55f, 0.85f);
			break;
		case GDIR_LEFT:
			a.v[0] += Vec3(0.85f, 0.45f, 0.85f);
			a.v[1] += Vec3(0.85f, 0.45f, -0.85f);
			a.v[2] += Vec3(0.85f, -1.55f, 0.85f);
			a.v[3] += Vec3(0.85f, -1.55f, -0.85f);
			break;
		}
		break;
	case ENTRY_DOOR:
		switch(dir)
		{
		case GDIR_DOWN:
			a.v[0] += Vec3(-0.85f, 0.1f, 0.85f);
			a.v[1] += Vec3(0.85f, 0.1f, 0.85f);
			a.v[2] += Vec3(-0.85f, 0.1f, 0.35f);
			a.v[3] += Vec3(0.85f, 0.1f, 0.35f);
			break;
		case GDIR_UP:
			a.v[0] += Vec3(0.85f, 0.1f, -0.85f);
			a.v[1] += Vec3(-0.85f, 0.1f, -0.85f);
			a.v[2] += Vec3(0.85f, 0.1f, -0.35f);
			a.v[3] += Vec3(-0.85f, 0.1f, -0.35f);
			break;
		case GDIR_RIGHT:
			a.v[0] += Vec3(-0.85f, 0.1f, -0.85f);
			a.v[1] += Vec3(-0.85f, 0.1f, 0.85f);
			a.v[2] += Vec3(-0.35f, 0.1f, -0.85f);
			a.v[3] += Vec3(-0.35f, 0.1f, 0.85f);
			break;
		case GDIR_LEFT:
			a.v[0] += Vec3(0.85f, 0.1f, 0.85f);
			a.v[1] += Vec3(0.85f, 0.1f, -0.85f);
			a.v[2] += Vec3(0.35f, 0.1f, 0.85f);
			a.v[3] += Vec3(0.35f, 0.1f, -0.85f);
			break;
		}
		break;
	}
}

//=================================================================================================
void Game::PrepareAreaPath()
{
	GameCamera& camera = gameLevel->camera;

	Ability* ability;
	if(pc->unit->action == A_CAST)
		ability = pc->unit->act.cast.ability;
	else
	{
		if(!pc->CanUseAbilityCheck())
			return;
		ability = pc->data.abilityReady;
	}

	switch(ability->type)
	{
	case Ability::Charge:
		{
			Area2* areaPtr = Area2::Get();
			Area2& area = *areaPtr;
			area.ok = 2;
			drawBatch.areas2.push_back(areaPtr);

			const Vec3 from = pc->unit->GetPhysicsPos();
			const float h = 0.06f;
			const float rot = Clip(pc->unit->rot + PI + pc->data.abilityRot);
			const int steps = 10;

			// find max line
			float t;
			Vec3 dir(sin(rot) * ability->range, 0, cos(rot) * ability->range);
			bool ignoreUnits = IsSet(ability->flags, Ability::IgnoreUnits);
			gameLevel->LineTest(pc->unit->cobj->getCollisionShape(), from, dir, [this, ignoreUnits](btCollisionObject* obj, bool)
			{
				int flags = obj->getCollisionFlags();
				if(IsSet(flags, CG_TERRAIN))
					return LT_IGNORE;
				if(IsSet(flags, CG_UNIT))
				{
					Unit* unit = reinterpret_cast<Unit*>(obj->getUserPointer());
					if(ignoreUnits || unit == pc->unit || !unit->IsStanding())
						return LT_IGNORE;
				}
				return LT_COLLIDE;
			}, t);

			float len = ability->range * t;

			if(gameLevel->location->outside && pc->unit->locPart->partType == LocationPart::Type::Outside)
			{
				// build line on terrain
				area.points.clear();
				area.faces.clear();

				Vec3 activePos = pc->unit->pos;
				Vec3 step = dir * t / (float)steps;
				Vec3 unitOffset(pc->unit->pos.x, 0, pc->unit->pos.z);
				float lenStep = len / steps;
				float activeStep = 0;
				Matrix mat = Matrix::RotationY(rot);
				for(int i = 0; i < steps; ++i)
				{
					float currentH = gameLevel->terrain->GetH(activePos) + h;
					area.points.push_back(Vec3::Transform(Vec3(-ability->width, currentH, activeStep), mat) + unitOffset);
					area.points.push_back(Vec3::Transform(Vec3(+ability->width, currentH, activeStep), mat) + unitOffset);

					activePos += step;
					activeStep += lenStep;
				}

				for(int i = 0; i < steps - 1; ++i)
				{
					area.faces.push_back(i * 2);
					area.faces.push_back((i + 1) * 2);
					area.faces.push_back(i * 2 + 1);
					area.faces.push_back((i + 1) * 2);
					area.faces.push_back(i * 2 + 1);
					area.faces.push_back((i + 1) * 2 + 1);
				}
			}
			else
			{
				// build line on flat terrain
				area.points.resize(4);
				area.points[0] = Vec3(-ability->width, h, 0);
				area.points[1] = Vec3(-ability->width, h, len);
				area.points[2] = Vec3(ability->width, h, 0);
				area.points[3] = Vec3(ability->width, h, len);

				Matrix mat = Matrix::RotationY(rot);
				for(int i = 0; i < 4; ++i)
					area.points[i] = Vec3::Transform(area.points[i], mat) + pc->unit->pos;

				area.faces.clear();
				area.faces.push_back(0);
				area.faces.push_back(1);
				area.faces.push_back(2);
				area.faces.push_back(1);
				area.faces.push_back(2);
				area.faces.push_back(3);
			}

			pc->data.abilityOk = true;
		}
		break;

	case Ability::Summon:
	case Ability::Trap:
		pc->data.abilityPoint = pc->RaytestTarget(ability->range);
		if(pc->data.rangeRatio < 1.f)
		{
			Area2* area = Area2::Get();
			drawBatch.areas2.push_back(area);

			const float radius = ability->width / 2;
			Vec3 dir = pc->data.abilityPoint - pc->unit->pos;
			dir.y = 0;
			float dist = dir.Length();
			dir.Normalize();

			Vec3 from = pc->data.abilityPoint;
			from.y = 0;

			bool ok = false;
			for(int i = 0; i < 20; ++i)
			{
				gameLevel->globalCol.clear();
				gameLevel->GatherCollisionObjects(*pc->unit->locPart, gameLevel->globalCol, from, radius);
				if(!gameLevel->Collide(gameLevel->globalCol, from, radius))
				{
					ok = true;
					break;
				}
				from -= dir * 0.1f;
				dist -= 0.1f;
				if(dist <= 0.f)
					break;
			}

			if(ok)
			{
				pc->data.abilityOk = true;
				if(gameLevel->location->outside && pc->unit->locPart->partType == LocationPart::Type::Outside)
					gameLevel->terrain->SetY(from);
				if(!pc->data.abilityReady)
					pc->unit->targetPos = from;
				else
					pc->data.abilityPoint = from;
				area->ok = 2;
			}
			else
			{
				pc->data.abilityOk = false;
				area->ok = 0;
			}
			PrepareAreaPathCircle(*area, pc->data.abilityPoint, radius);
		}
		else
			pc->data.abilityOk = false;
		break;

	case Ability::Target:
		{
			Unit* target;
			if(GKey.KeyDownAllowed(GK_MOVE_BACK))
			{
				// cast on self
				target = pc->unit;
			}
			else
			{
				// raytest - can be cast on alive units or dead team members
				const float range = ability->range + camera.dist;
				RaytestClosestUnitCallback clbk([=](Unit* unit)
				{
					if(unit == pc->unit)
						return false;
					if(!unit->IsAlive() && !unit->IsTeamMember())
						return false;
					return true;
				});
				Vec3 from = camera.from;
				Vec3 dir = (camera.to - from).Normalized();
				from += dir * camera.dist;
				Vec3 to = from + dir * range;
				phyWorld->rayTest(ToVector3(from), ToVector3(to), clbk);
				target = clbk.hit;
			}

			if(target)
			{
				pc->data.abilityOk = true;
				pc->data.abilityPoint = target->pos;
				pc->data.abilityTarget = target;
			}
			else
			{
				pc->data.abilityOk = false;
				pc->data.abilityTarget = nullptr;
			}
		}
		break;

	case Ability::Ray:
	case Ability::Point:
		pc->data.abilityPoint = pc->RaytestTarget(ability->range);
		pc->data.abilityOk = true;
		pc->data.abilityTarget = nullptr;
		break;

	case Ability::RangedAttack:
		pc->data.abilityOk = true;
		break;
	}
}

//=================================================================================================
void Game::PrepareAreaPathCircle(Area2& area, float radius, float range, float rot)
{
	const float h = 0.06f;
	const int circlePoints = 10;

	area.points.resize(circlePoints + 1);
	area.points[0] = Vec3(0, h, 0 + range);
	float angle = 0;
	for(int i = 0; i < circlePoints; ++i)
	{
		area.points[i + 1] = Vec3(sin(angle) * radius, h, cos(angle) * radius + range);
		angle += (PI * 2) / circlePoints;
	}

	bool outside = (gameLevel->location->outside && pc->unit->locPart->partType == LocationPart::Type::Outside);
	Matrix mat = Matrix::RotationY(rot);
	for(int i = 0; i < circlePoints + 1; ++i)
	{
		area.points[i] = Vec3::Transform(area.points[i], mat) + pc->unit->pos;
		if(outside)
			area.points[i].y = gameLevel->terrain->GetH(area.points[i]) + h;
	}

	area.faces.clear();
	for(int i = 0; i < circlePoints; ++i)
	{
		area.faces.push_back(0);
		area.faces.push_back(i + 1);
		area.faces.push_back((i + 2) == (circlePoints + 1) ? 1 : (i + 2));
	}
}

//=================================================================================================
void Game::PrepareAreaPathCircle(Area2& area, const Vec3& pos, float radius)
{
	const float h = 0.06f;
	const int circlePoints = 10;

	area.points.resize(circlePoints + 1);
	area.points[0] = pos + Vec3(0, h, 0);
	float angle = 0;
	for(int i = 0; i < circlePoints; ++i)
	{
		area.points[i + 1] = pos + Vec3(sin(angle) * radius, h, cos(angle) * radius);
		angle += (PI * 2) / circlePoints;
	}

	bool outside = (gameLevel->location->outside && pc->unit->locPart->partType == LocationPart::Type::Outside);
	if(outside)
	{
		for(int i = 0; i < circlePoints + 1; ++i)
			area.points[i].y = gameLevel->terrain->GetH(area.points[i]) + h;
	}

	area.faces.clear();
	for(int i = 0; i < circlePoints; ++i)
	{
		area.faces.push_back(0);
		area.faces.push_back(i + 1);
		area.faces.push_back((i + 2) == (circlePoints + 1) ? 1 : (i + 2));
	}
}

//=================================================================================================
void Game::GatherDrawBatchLights(SceneNode* node)
{
	assert(node);
	GatherDrawBatchLights(node, node->center.x, node->center.z, node->radius, node->subs & SceneNode::SPLIT_MASK, node->lights);
}

//=================================================================================================
void Game::GatherDrawBatchLights(SceneNode* node, float x, float z, float radius, int sub, array<Light*, 3>& lights)
{
	assert(radius > 0);

	TopN<Light*, 3, float, std::less<>> best(nullptr, drawBatch.camera->zfar);

	if(drawBatch.locPart->masks.empty())
	{
		for(Light& light : drawBatch.locPart->lights)
		{
			float dist = Distance(x, z, light.pos.x, light.pos.z);
			if(dist < light.range + radius)
				best.Add(&light, dist);
		}
	}
	else
	{
		const Vec2 objPos(x, z);
		const bool isSplit = (node && IsSet(node->mesh->head.flags, Mesh::F_SPLIT));

		for(GameLight& light : drawBatch.locPart->lights)
		{
			Vec2 lightPos;
			float dist;
			bool ok = true, masked = false;
			if(!isSplit)
			{
				dist = Distance(x, z, light.pos.x, light.pos.z);
				if(dist > light.range + radius || !best.CanAdd(dist))
					continue;
				if(!IsZero(dist))
				{
					lightPos = light.pos.XZ();
					float rangeSum = 0.f;

					// are there any masks between object and light?
					for(LightMask& mask : drawBatch.locPart->masks)
					{
						if(LineToRectangleSize(objPos, lightPos, mask.pos, mask.size))
						{
							// move light to one side of mask
							Vec2 newPos, newPos2;
							float newDist[2];
							if(mask.size.x > mask.size.y)
							{
								newPos.x = mask.pos.x - mask.size.x;
								newPos2.x = mask.pos.x + mask.size.x;
								newPos.y = newPos2.y = mask.pos.y;
							}
							else
							{
								newPos.x = newPos2.x = mask.pos.x;
								newPos.y = mask.pos.y - mask.size.y;
								newPos2.y = mask.pos.y + mask.size.y;
							}
							newDist[0] = Vec2::Distance(lightPos, newPos) + Vec2::Distance(newPos, objPos);
							newDist[1] = Vec2::Distance(lightPos, newPos2) + Vec2::Distance(newPos2, objPos);
							if(newDist[1] < newDist[0])
								newPos = newPos2;

							// recalculate distance
							rangeSum += Vec2::Distance(lightPos, newPos);
							dist = rangeSum + Distance(x, z, newPos.x, newPos.y);
							if(!best.CanAdd(dist))
							{
								ok = false;
								break;
							}
							lightPos = newPos;
							masked = true;
						}
					}
				}
			}
			else
			{
				const Vec2 subSize = node->mesh->splits[sub].box.SizeXZ();
				lightPos = light.pos.XZ();
				dist = DistanceRectangleToPoint(objPos, subSize, lightPos);
				if(dist > light.range + radius || !best.CanAdd(dist))
					continue;
				if(!IsZero(dist))
				{
					float rangeSum = 0.f;

					// are there any masks between object and light?
					for(LightMask& mask : drawBatch.locPart->masks)
					{
						if(LineToRectangleSize(objPos, lightPos, mask.pos, mask.size))
						{
							// move light to one side of mask
							Vec2 newPos, newPos2;
							float newDist[2];
							if(mask.size.x > mask.size.y)
							{
								newPos.x = mask.pos.x - mask.size.x;
								newPos2.x = mask.pos.x + mask.size.x;
								newPos.y = newPos2.y = mask.pos.y;
							}
							else
							{
								newPos.x = newPos2.x = mask.pos.x;
								newPos.y = mask.pos.y - mask.size.y;
								newPos2.y = mask.pos.y + mask.size.y;
							}
							newDist[0] = Vec2::Distance(lightPos, newPos) + DistanceRectangleToPoint(objPos, subSize, newPos);
							newDist[1] = Vec2::Distance(lightPos, newPos2) + DistanceRectangleToPoint(objPos, subSize, newPos2);
							if(newDist[1] < newDist[0])
								newPos = newPos2;

							// recalculate distance
							rangeSum += Vec2::Distance(lightPos, newPos);
							dist = rangeSum + DistanceRectangleToPoint(objPos, subSize, newPos);
							if(!best.CanAdd(dist))
							{
								ok = false;
								break;
							}
							lightPos = newPos;
							masked = true;
						}
					}
				}
			}

			if(!ok)
				continue;

			if(masked)
			{
				float range = light.range - Vec2::Distance(lightPos, light.pos.XZ());
				if(range > 0)
				{
					Light* tmpLight = DrawBatch::lightPool.Get();
					tmpLight->color = light.color;
					tmpLight->pos = Vec3(lightPos.x, light.pos.y, lightPos.y);
					tmpLight->range = range;
					best.Add(tmpLight, dist);
					drawBatch.tmpLights.push_back(tmpLight);
				}
			}
			else
				best.Add(&light, dist);
		}
	}

	for(int i = 0; i < 3; ++i)
		lights[i] = best[i];
}

//=================================================================================================
void Game::DrawScene()
{
	sceneMgr->SetScene(drawBatch.scene, drawBatch.camera);

	// sky
	if(drawBatch.scene->useLightDir && IsSet(drawFlags, DF_SKYBOX))
		skyboxShader->Draw(*gameRes->aSkybox, *drawBatch.camera);

	// terrain
	if(!drawBatch.terrainParts.empty())
		terrainShader->Draw(drawBatch.scene, drawBatch.camera, gameLevel->terrain, drawBatch.terrainParts);

	// dungeon
	if(!drawBatch.dungeonParts.empty())
		DrawDungeon(drawBatch.dungeonParts, drawBatch.dungeonPartGroups);

	// nodes
	if(!drawBatch.nodes.empty())
		sceneMgr->DrawSceneNodes(drawBatch);

	// grass
	DrawGrass();

	// debug nodes
	if(!drawBatch.debugNodes.empty())
		basicShader->DrawDebugNodes(drawBatch.debugNodes);
	if(pathfinding->IsDebugDraw())
	{
		basicShader->Prepare(gameLevel->camera);
		basicShader->SetAreaParams(Vec3::Zero, 100.f, 0.f);
		pathfinding->Draw(basicShader);
	}

	// blood
	if(!drawBatch.bloods.empty())
		DrawBloods(drawBatch.bloods);

	// particles
	if(!drawBatch.billboards.empty() || !drawBatch.pes.empty() || drawBatch.tpes)
	{
		particleShader->Prepare(gameLevel->camera);
		if(!drawBatch.billboards.empty())
			particleShader->DrawBillboards(drawBatch.billboards);
		if(drawBatch.tpes)
			particleShader->DrawTrailParticles(*drawBatch.tpes);
		if(!drawBatch.pes.empty())
			particleShader->DrawParticles(drawBatch.pes);
	}

	// alpha nodes
	if(!drawBatch.alphaNodes.empty())
		sceneMgr->DrawAlphaSceneNodes(drawBatch);

	// areas
	if(!drawBatch.areas.empty() || !drawBatch.areas2.empty())
		DrawAreas(drawBatch.areas, drawBatch.areaRange, drawBatch.areas2);
}

//=================================================================================================
void Game::DrawDungeon(const vector<DungeonPart>& parts, const vector<DungeonPartGroup>& groups)
{
	SuperShader* shader = sceneMgr->superShader;

	render->SetBlendState(Render::BLEND_NO);
	render->SetDepthState(Render::DEPTH_YES);
	render->SetRasterState(Render::RASTER_NORMAL);

	shader->Prepare();
	shader->SetCustomMesh(dungeonMeshBuilder->vb, dungeonMeshBuilder->ib, sizeof(VTangent));

	const bool useFog = sceneMgr->useFog && sceneMgr->useLighting;
	TexOverride* lastOverride = nullptr;

	for(vector<DungeonPart>::const_iterator it = parts.begin(), end = parts.end(); it != end; ++it)
	{
		const DungeonPart& dp = *it;

		// set textures
		if(lastOverride != dp.texOverride)
		{
			shader->SetShader(shader->GetShaderId(false, true, false, useFog, sceneMgr->useSpecularmap && dp.texOverride->specular != nullptr,
				sceneMgr->useNormalmap && dp.texOverride->normal != nullptr, sceneMgr->useLighting, false));
			shader->SetTexture(dp.texOverride, nullptr, 0);
			lastOverride = dp.texOverride;
		}

		const DungeonPartGroup& group = groups[dp.group];
		shader->DrawCustom(group.matWorld, group.matCombined, group.lights, dp.startIndex, dp.primitiveCount * 3);
	}
}

//=================================================================================================
void Game::DrawBloods(const vector<Blood*>& bloods)
{
	SuperShader* shader = sceneMgr->superShader;
	shader->PrepareDecals();
	Decal decal;
	for(const Blood* blood : bloods)
	{
		decal.pos = blood->pos;
		decal.normal = blood->normal;
		decal.rot = blood->rot;
		decal.scale = blood->size * blood->scale;
		decal.tex = gameRes->tBloodSplat[blood->type]->tex;
		decal.lights = &blood->lights;
		shader->DrawDecal(decal);
	}
}

//=================================================================================================
void Game::DrawAreas(const vector<Area>& areas, float range, const vector<Area2*>& areas2)
{
	basicShader->Prepare(*drawBatch.camera);

	if(!areas.empty())
	{
		Vec3 playerPos = pc->unit->pos;
		playerPos.y += 0.75f;

		basicShader->SetAreaParams(playerPos, range, 1.f);

		const Color color = Color(0.f, 1.f, 0.f, 0.5f);
		for(const Area& area : areas)
			basicShader->DrawQuad(area.v, color);

		basicShader->Draw();
	}

	if(!areas2.empty())
	{
		basicShader->SetAreaParams(Vec3::Zero, 100.f, 0.f);

		const Color colors[3] = {
			Color(1.f, 0.f, 0.f, 0.5f),
			Color(1.f, 1.f, 0.f, 0.5f),
			Color(0.f, 0.58f, 1.f, 0.5f)
		};

		for(Area2* area2 : areas2)
			basicShader->DrawArea(area2->points, area2->faces, colors[area2->ok]);

		basicShader->Draw();
	}
}

//=================================================================================================
void Game::UvModChanged()
{
	gameLevel->terrain->uvMod = uvMod;
	gameLevel->terrain->RebuildUv();
}

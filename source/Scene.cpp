#include "Pch.h"
#include "Game.h"

#include "Ability.h"
#include "City.h"
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

	draw_batch.Clear();
	draw_batch.locPart = &locPart;
	draw_batch.scene = lvlPart.scene;
	draw_batch.camera = &game_level->camera;
	draw_batch.gatherLights = !draw_batch.scene->useLightDir && sceneMgr->useLighting;
	ClearGrass();
	if(locPart.partType == LocationPart::Type::Outside)
	{
		ListQuadtreeNodes();
		ListGrass();
	}

	// existing nodes
	for(SceneNode* node : draw_batch.scene->nodes)
	{
		if(frustum.SphereToFrustum(node->center, node->radius))
		{
			if(draw_batch.gatherLights)
				GatherDrawBatchLights(node);
			draw_batch.Add(node);
		}
	}

	// terrain
	if(locPart.partType == LocationPart::Type::Outside && IsSet(draw_flags, DF_TERRAIN))
		game_level->terrain->ListVisibleParts(draw_batch.terrain_parts, frustum);

	// dungeon
	if(locPart.partType == LocationPart::Type::Inside && IsSet(draw_flags, DF_TERRAIN))
		dun_mesh_builder->ListVisibleParts(draw_batch, frustum);

	// units
	if(IsSet(draw_flags, DF_UNITS))
	{
		for(Unit* unit : locPart.units)
			ListDrawObjectsUnit(frustum, *unit);
	}

	// objects
	if(IsSet(draw_flags, DF_OBJECTS))
	{
		if(locPart.partType == LocationPart::Type::Outside)
		{
			for(LevelQuad* quad : level_quads)
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
	if(IsSet(draw_flags, DF_USABLES))
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
				if(draw_batch.gatherLights)
					GatherDrawBatchLights(node);
				if(pc->data.before_player == BP_USABLE && pc->data.before_player_ptr.usable == &use)
				{
					if(useGlow)
					{
						GlowNode& glow = Add1(draw_batch.glow_nodes);
						glow.node = node;
						glow.color = Color::White;
					}
					else
						node->tint = Vec4(2, 2, 2, 1);
				}
				draw_batch.Add(node);
			}
		}
	}

	// chests
	if(IsSet(draw_flags, DF_USABLES))
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
				if(draw_batch.gatherLights)
					GatherDrawBatchLights(node);
				if(pc->data.before_player == BP_CHEST && pc->data.before_player_ptr.chest == &chest)
				{
					if(useGlow)
					{
						GlowNode& glow = Add1(draw_batch.glow_nodes);
						glow.node = node;
						glow.color = Color::White;
					}
					else
						node->tint = Vec4(2, 2, 2, 1);
				}
				draw_batch.Add(node);
			}
		}
	}

	// doors
	if(IsSet(draw_flags, DF_USABLES))
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
				if(draw_batch.gatherLights)
					GatherDrawBatchLights(node);
				if(pc->data.before_player == BP_DOOR && pc->data.before_player_ptr.door == &door)
				{
					if(useGlow)
					{
						GlowNode& glow = Add1(draw_batch.glow_nodes);
						glow.node = node;
						glow.color = Color::White;
					}
					else
						node->tint = Vec4(2, 2, 2, 1);
				}
				draw_batch.Add(node);
			}
		}
	}

	// bloods
	if(IsSet(draw_flags, DF_BLOOD))
	{
		for(Blood& blood : locPart.bloods)
		{
			if(blood.size > 0.f && frustum.SphereToFrustum(blood.pos, blood.size * blood.scale))
			{
				if(draw_batch.gatherLights)
					GatherDrawBatchLights(nullptr, blood.pos.x, blood.pos.y, blood.size * blood.scale, 0, blood.lights);
				draw_batch.bloods.push_back(&blood);
			}
		}
	}

	// bullets
	if(IsSet(draw_flags, DF_BULLETS))
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
					if(draw_batch.gatherLights)
						GatherDrawBatchLights(node);
					draw_batch.Add(node);
				}
			}
			else
			{
				if(frustum.SphereToFrustum(bullet.pos, bullet.tex_size))
				{
					Billboard& bb = Add1(draw_batch.billboards);
					bb.pos = bullet.pos;
					bb.size = bullet.tex_size;
					bb.tex = bullet.tex;
				}
			}
		}
	}

	// traps
	if(IsSet(draw_flags, DF_TRAPS))
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
				if(draw_batch.gatherLights)
					GatherDrawBatchLights(node);
				draw_batch.Add(node);
			}
		}
	}

	// explosions
	if(IsSet(draw_flags, DF_EXPLOS))
	{
		game_res->aSpellball->EnsureIsLoaded();
		for(vector<Explo*>::iterator it = lvlPart.explos.begin(), end = lvlPart.explos.end(); it != end; ++it)
		{
			Explo& explo = **it;
			if(frustum.SphereToFrustum(explo.pos, explo.size))
			{
				SceneNode* node = SceneNode::Get();
				node->SetMesh(game_res->aSpellball);
				node->flags |= SceneNode::F_NO_LIGHTING | SceneNode::F_ALPHA_BLEND | SceneNode::F_NO_ZWRITE;
				node->center = explo.pos;
				node->radius *= explo.size;
				node->mat = Matrix::Scale(explo.size) * Matrix::Translation(explo.pos);
				node->texOverride = &explo.ability->tex_explode;
				node->tint = Vec4(1, 1, 1, 1.f - explo.size / explo.sizemax);
				draw_batch.Add(node);
			}
		}
	}

	// particles
	if(IsSet(draw_flags, DF_PARTICLES))
	{
		for(vector<ParticleEmitter*>::iterator it = lvlPart.pes.begin(), end = lvlPart.pes.end(); it != end; ++it)
		{
			ParticleEmitter& pe = **it;
			if(pe.alive && frustum.SphereToFrustum(pe.pos, pe.radius))
			{
				draw_batch.pes.push_back(&pe);
				if(draw_particle_sphere)
				{
					DebugNode* debug_node = DebugNode::Get();
					debug_node->mat = Matrix::Scale(pe.radius * 2) * Matrix::Translation(pe.pos) * draw_batch.camera->matViewProj;
					debug_node->shape = MeshShape::Sphere;
					debug_node->color = Color::Green;
					draw_batch.debug_nodes.push_back(debug_node);
				}
			}
		}

		if(lvlPart.tpes.empty())
			draw_batch.tpes = nullptr;
		else
			draw_batch.tpes = &lvlPart.tpes;
	}
	else
		draw_batch.tpes = nullptr;

	// portals
	if(IsSet(draw_flags, DF_PORTALS) && locPart.partType != LocationPart::Type::Building)
	{
		game_res->aPortal->EnsureIsLoaded();
		Portal* portal = game_level->location->portal;
		while(portal)
		{
			if(game_level->location->outside || game_level->dungeon_level == portal->at_level)
			{
				SceneNode* node = SceneNode::Get();
				node->SetMesh(game_res->aPortal);
				node->flags |= SceneNode::F_NO_LIGHTING | SceneNode::F_ALPHA_BLEND | SceneNode::F_NO_CULLING;
				node->center = portal->pos + Vec3(0, 0.67f + 0.305f, 0);
				node->mat = Matrix::Rotation(0, portal->rot, -portal_anim * PI * 2) * Matrix::Translation(node->center);
				draw_batch.Add(node);
			}
			portal = portal->next_portal;
		}
	}

	// areas
	if(IsSet(draw_flags, DF_AREA))
		ListAreas(locPart);

	// colliders
	if(draw_col)
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
				node->mat = Matrix::Scale(scale) * Matrix::RotationY(rot) * Matrix::Translation(it->pos) * draw_batch.camera->matViewProj;
				draw_batch.debug_nodes.push_back(node);
			}
		}
	}

	// physics
	if(draw_phy)
	{
		const btCollisionObjectArray& cobjs = phy_world->getCollisionObjectArray();
		const int count = cobjs.size();

		for(int i = 0; i < count; ++i)
		{
			const btCollisionObject* cobj = cobjs[i];
			const btCollisionShape* shape = cobj->getCollisionShape();
			Matrix m_world;
			cobj->getWorldTransform().getOpenGLMatrix(&m_world._11);

			switch(shape->getShapeType())
			{
			case BOX_SHAPE_PROXYTYPE:
				{
					const btBoxShape* box = (const btBoxShape*)shape;
					DebugNode* node = DebugNode::Get();
					node->shape = MeshShape::Box;
					node->color = Color(163, 73, 164);
					node->mat = Matrix::Scale(ToVec3(box->getHalfExtentsWithMargin())) * m_world * draw_batch.camera->matViewProj;
					draw_batch.debug_nodes.push_back(node);
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
					node->mat = Matrix::Scale(r, h + r, r) * m_world * draw_batch.camera->matViewProj;
					draw_batch.debug_nodes.push_back(node);
				}
				break;
			case CYLINDER_SHAPE_PROXYTYPE:
				{
					const btCylinderShape* cylinder = (const btCylinderShape*)shape;
					DebugNode* node = DebugNode::Get();
					node->shape = MeshShape::Cylinder;
					node->color = Color(163, 73, 164);
					Vec3 v = ToVec3(cylinder->getHalfExtentsWithoutMargin());
					node->mat = Matrix::Scale(v.x, v.y / 2, v.z) * m_world * draw_batch.camera->matViewProj;
					draw_batch.debug_nodes.push_back(node);
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
							Matrix m_child;
							compound->getChildTransform(i).getOpenGLMatrix(&m_child._11);
							node->mat = Matrix::Scale(ToVec3(box->getHalfExtentsWithMargin())) * m_child * m_world * draw_batch.camera->matViewProj;
							draw_batch.debug_nodes.push_back(node);
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
					node->mat = m_world * draw_batch.camera->matViewProj;
					node->trimesh = reinterpret_cast<SimpleMesh*>(shape->getUserPointer());
					draw_batch.debug_nodes.push_back(node);
				}
			default:
				break;
			}
		}
	}

	// glow
	draw_batch.tmpGlow = nullptr;
	if(PlayerController::data.before_player == BP_ITEM)
	{
		GroundItem* groundItem = PlayerController::data.before_player_ptr.item;
		if(useGlow)
		{
			GlowNode& glow = Add1(draw_batch.glow_nodes);
			glow.node = groundItem->node;
			glow.color = Color::White;
		}
		else
		{
			groundItem->node->tint = Vec4(2, 2, 2, 1);
			draw_batch.tmpGlow = groundItem->node;
		}
	}

	draw_batch.Process();
}

//=================================================================================================
void Game::ListDrawObjectsUnit(FrustumPlanes& frustum, Unit& u)
{
	u.meshInst->mesh->EnsureIsLoaded();
	if(!frustum.SphereToFrustum(u.visual_pos, u.GetSphereRadius()))
		return;

	// add stun effect
	if(u.IsAlive())
	{
		Effect* effect = u.FindEffect(EffectId::Stun);
		if(effect)
		{
			SceneNode* node = SceneNode::Get();
			node->SetMesh(game_res->aStun);
			node->flags |= SceneNode::F_NO_LIGHTING | SceneNode::F_ALPHA_BLEND | SceneNode::F_NO_CULLING | SceneNode::F_NO_ZWRITE;
			node->center = u.GetHeadPoint();
			node->mat = Matrix::RotationY(portal_anim * PI * 2) * Matrix::Translation(node->center);
			draw_batch.Add(node);
		}

		if(u.HaveEffect(EffectId::Rooted, EffectValue_Rooted_Vines))
		{
			SceneNode* node = SceneNode::Get();
			node->SetMesh(game_res->mVine);
			node->center = u.pos;
			node->mat = Matrix::Scale(u.GetUnitRadius() / 0.3f) * Matrix::Translation(node->center);
			draw_batch.Add(node);
		}
	}

	// set bones
	u.meshInst->SetupBones();

	bool selected = (pc->data.before_player == BP_UNIT && pc->data.before_player_ptr.unit == &u)
		|| (game_state == GS_LEVEL && ((pc->data.ability_ready && pc->data.ability_ok && pc->data.ability_target == u)
			|| (pc->unit->action == A_CAST && pc->unit->act.cast.target == u)));

	// add scene node
	SceneNode* node = SceneNode::Get();
	node->SetMesh(u.meshInst);
	if(IsSet(u.data->flags2, F2_ALPHA_BLEND))
		node->flags |= SceneNode::F_ALPHA_BLEND;
	node->center = u.visual_pos;
	node->mat = Matrix::Scale(u.data->scale) * Matrix::RotationY(u.rot) * Matrix::Translation(u.visual_pos);
	node->texOverride = u.data->GetTextureOverride();
	node->tint = u.data->tint;

	// light settings
	if(draw_batch.gatherLights)
		GatherDrawBatchLights(node);
	if(selected)
	{
		if(useGlow)
		{
			GlowNode& glow = Add1(draw_batch.glow_nodes);
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
	draw_batch.Add(node);
	if(u.HaveArmor() && u.GetArmor().armor_unit_type == ArmorUnitType::HUMAN && u.GetArmor().mesh)
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
				GlowNode& glow = Add1(draw_batch.glow_nodes);
				glow.node = node2;
				glow.color = pc->unit->GetRelationColor(u);
			}
			else
				node2->tint = Vec4(2, 2, 2, 1);
		}
		draw_batch.Add(node2);
	}

	// item in hand
	Mesh* right_hand_item = nullptr;
	bool in_hand = false;

	switch(u.weapon_state)
	{
	case WeaponState::Hidden:
		break;
	case WeaponState::Taken:
		if(u.weapon_taken == W_BOW)
		{
			if(u.action == A_SHOOT)
			{
				if(u.animation_state != AS_SHOOT_SHOT)
					right_hand_item = game_res->aArrow;
			}
			else
				right_hand_item = game_res->aArrow;
		}
		else if(u.weapon_taken == W_ONE_HANDED)
			in_hand = true;
		break;
	case WeaponState::Taking:
		if(u.animation_state == AS_TAKE_WEAPON_MOVED)
		{
			if(u.weapon_taken == W_BOW)
				right_hand_item = game_res->aArrow;
			else
				in_hand = true;
		}
		break;
	case WeaponState::Hiding:
		if(u.animation_state == AS_TAKE_WEAPON_START)
		{
			if(u.weapon_hiding == W_BOW)
				right_hand_item = game_res->aArrow;
			else
				in_hand = true;
		}
		break;
	}

	if(u.used_item && u.action != A_USE_ITEM)
		right_hand_item = u.used_item->mesh;

	if(right_hand_item == game_res->aArrow && u.action == A_SHOOT && u.act.shoot.ability && u.act.shoot.ability->mesh)
		right_hand_item = u.act.shoot.ability->mesh;

	Matrix mat_scale;
	if(u.human_data)
	{
		Vec2 scale = u.human_data->GetScale();
		scale.x = 1.f / scale.x;
		scale.y = 1.f / scale.y;
		mat_scale = Matrix::Scale(scale.x, scale.y, scale.x);
	}
	else
		mat_scale = Matrix::IdentityMatrix;

	// weapon
	Mesh* mesh;
	if(u.HaveWeapon() && right_hand_item != (mesh = u.GetWeapon().mesh))
	{
		Mesh::Point* point = u.meshInst->mesh->GetPoint(in_hand ? NAMES::point_weapon : NAMES::point_hidden_weapon);
		assert(point);

		SceneNode* node2 = SceneNode::Get();
		node2->SetMesh(mesh);
		node2->center = node->center;
		node2->mat = mat_scale * point->mat * u.meshInst->matBones[point->bone] * node->mat;
		node2->lights = node->lights;
		if(selected)
		{
			if(useGlow)
			{
				GlowNode& glow = Add1(draw_batch.glow_nodes);
				glow.node = node2;
				glow.color = pc->unit->GetRelationColor(u);
			}
			else
				node2->tint = Vec4(2, 2, 2, 1);
		}
		draw_batch.Add(node2);

		// weapon hitbox
		if(draw_hitbox && u.weapon_state == WeaponState::Taken && u.weapon_taken == W_ONE_HANDED)
		{
			Mesh::Point* box = mesh->FindPoint("hit");
			assert(box && box->IsBox());

			DebugNode* debug_node = DebugNode::Get();
			debug_node->mat = box->mat * node2->mat * draw_batch.camera->matViewProj;
			debug_node->shape = MeshShape::Box;
			debug_node->color = Color::Black;
			draw_batch.debug_nodes.push_back(debug_node);
		}
	}
	else if(u.action == A_ATTACK && draw_hitbox)
	{
		Mesh::Point* hitbox = u.meshInst->mesh->GetPoint(Format("hitbox%d", u.act.attack.index + 1));
		if(!hitbox)
			hitbox = u.meshInst->mesh->FindPoint("hitbox");
		DebugNode* debug_node = DebugNode::Get();
		debug_node->mat = hitbox->mat * u.meshInst->matBones[hitbox->bone] * node->mat * draw_batch.camera->matViewProj;
		debug_node->shape = MeshShape::Box;
		debug_node->color = Color::Black;
		draw_batch.debug_nodes.push_back(debug_node);
	}

	// shield
	if(u.HaveShield() && u.GetShield().mesh)
	{
		Mesh* shield = u.GetShield().mesh;
		Mesh::Point* point = u.meshInst->mesh->GetPoint(in_hand ? NAMES::point_shield : NAMES::point_shield_hidden);
		assert(point);

		SceneNode* node2 = SceneNode::Get();
		node2->SetMesh(shield);
		node2->center = node->center;
		node2->mat = mat_scale * point->mat * u.meshInst->matBones[point->bone] * node->mat;
		node2->lights = node->lights;
		if(selected)
		{
			if(useGlow)
			{
				GlowNode& glow = Add1(draw_batch.glow_nodes);
				glow.node = node2;
				glow.color = pc->unit->GetRelationColor(u);
			}
			else
				node2->tint = Vec4(2, 2, 2, 1);
		}
		draw_batch.Add(node2);

		// shield hitbox
		if(draw_hitbox && u.weapon_state == WeaponState::Taken && u.weapon_taken == W_ONE_HANDED)
		{
			Mesh::Point* box = shield->FindPoint("hit");
			assert(box && box->IsBox());

			DebugNode* debug_node = DebugNode::Get();
			debug_node->mat = box->mat * node2->mat * draw_batch.camera->matViewProj;
			debug_node->shape = MeshShape::Box;
			debug_node->color = Color::Black;
			draw_batch.debug_nodes.push_back(debug_node);
		}
	}

	// other item (arrow/potion)
	if(right_hand_item)
	{
		Mesh::Point* point = u.meshInst->mesh->GetPoint(NAMES::point_weapon);
		assert(point);

		SceneNode* node2 = SceneNode::Get();
		node2->SetMesh(right_hand_item);
		node2->center = node->center;
		node2->mat = mat_scale * point->mat * u.meshInst->matBones[point->bone] * node->mat;
		node2->lights = node->lights;
		if(selected)
		{
			if(useGlow)
			{
				GlowNode& glow = Add1(draw_batch.glow_nodes);
				glow.node = node2;
				glow.color = pc->unit->GetRelationColor(u);
			}
			else
				node2->tint = Vec4(2, 2, 2, 1);
		}
		draw_batch.Add(node2);
	}

	// bow
	if(u.HaveBow())
	{
		bool in_hand;

		switch(u.weapon_state)
		{
		case WeaponState::Hiding:
			in_hand = (u.weapon_hiding == W_BOW && u.animation_state == AS_TAKE_WEAPON_START);
			break;
		case WeaponState::Hidden:
			in_hand = false;
			break;
		case WeaponState::Taking:
			in_hand = (u.weapon_taken == W_BOW && u.animation_state == AS_TAKE_WEAPON_MOVED);
			break;
		case WeaponState::Taken:
			in_hand = (u.weapon_taken == W_BOW);
			break;
		}

		SceneNode* node2 = SceneNode::Get();
		if(u.action == A_SHOOT)
		{
			u.bow_instance->SetupBones();
			node2->SetMesh(u.bow_instance);
		}
		else
			node2->SetMesh(u.GetBow().mesh);
		node2->center = node->center;
		Mesh::Point* point = u.meshInst->mesh->GetPoint(in_hand ? NAMES::point_bow : NAMES::point_shield_hidden);
		assert(point);
		Matrix m1;
		if(in_hand)
			m1 = Matrix::RotationZ(-PI / 2) * point->mat * u.meshInst->matBones[point->bone];
		else
			m1 = point->mat * u.meshInst->matBones[point->bone];
		node2->mat = mat_scale * m1 * node->mat;
		node2->lights = node->lights;
		if(selected)
		{
			if(useGlow)
			{
				GlowNode& glow = Add1(draw_batch.glow_nodes);
				glow.node = node2;
				glow.color = pc->unit->GetRelationColor(u);
			}
			else
				node2->tint = Vec4(2, 2, 2, 1);
		}
		draw_batch.Add(node2);
	}

	// hair/beard/eyebrows for humans
	if(u.data->type == UNIT_TYPE::HUMAN)
	{
		Human& h = *u.human_data;

		// eyebrows
		SceneNode* node2 = SceneNode::Get();
		node2->SetMesh(game_res->aEyebrows, node->meshInst);
		node2->center = node->center;
		node2->mat = node->mat;
		node2->tint = h.hair_color * u.data->tint;
		node2->lights = node->lights;
		if(selected)
		{
			if(useGlow)
			{
				GlowNode& glow = Add1(draw_batch.glow_nodes);
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
		draw_batch.Add(node2);

		// hair
		if(h.hair != -1)
		{
			SceneNode* node3 = SceneNode::Get();
			node3->SetMesh(game_res->aHair[h.hair], node->meshInst);
			node3->center = node->center;
			node3->mat = node->mat;
			node3->tint = h.hair_color * u.data->tint;
			node3->lights = node->lights;
			if(selected)
			{
				if(useGlow)
				{
					GlowNode& glow = Add1(draw_batch.glow_nodes);
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
			draw_batch.Add(node3);
		}

		// beard
		if(h.beard != -1)
		{
			SceneNode* node3 = SceneNode::Get();
			node3->SetMesh(game_res->aBeard[h.beard], node->meshInst);
			node3->center = node->center;
			node3->mat = node->mat;
			node3->tint = h.hair_color * u.data->tint;
			node3->lights = node->lights;
			if(selected)
			{
				if(useGlow)
				{
					GlowNode& glow = Add1(draw_batch.glow_nodes);
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
			draw_batch.Add(node3);
		}

		// mustache
		if(h.mustache != -1 && (h.beard == -1 || !g_beard_and_mustache[h.beard]))
		{
			SceneNode* node3 = SceneNode::Get();
			node3->SetMesh(game_res->aMustache[h.mustache], node->meshInst);
			node3->center = node->center;
			node3->mat = node->mat;
			node3->tint = h.hair_color * u.data->tint;
			node3->lights = node->lights;
			if(selected)
			{
				if(useGlow)
				{
					GlowNode& glow = Add1(draw_batch.glow_nodes);
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
			draw_batch.Add(node3);
		}
	}

	// unit hitbox radius
	if(draw_unit_radius)
	{
		float h = u.GetUnitHeight() / 2;
		DebugNode* debug_node = DebugNode::Get();
		debug_node->mat = Matrix::Scale(u.GetUnitRadius(), h, u.GetUnitRadius()) * Matrix::Translation(u.GetColliderPos() + Vec3(0, h, 0)) * draw_batch.camera->matViewProj;
		debug_node->shape = MeshShape::Cylinder;
		debug_node->color = Color::White;
		draw_batch.debug_nodes.push_back(debug_node);
	}
	if(draw_hitbox)
	{
		float h = u.GetUnitHeight() / 2;
		Box box;
		u.GetBox(box);
		DebugNode* debug_node = DebugNode::Get();
		debug_node->mat = Matrix::Scale(box.SizeX() / 2, h, box.SizeZ() / 2) * Matrix::Translation(u.pos + Vec3(0, h, 0)) * draw_batch.camera->matViewProj;
		debug_node->shape = MeshShape::Box;
		debug_node->color = Color::Black;
		draw_batch.debug_nodes.push_back(debug_node);
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
		if(draw_batch.gatherLights)
			GatherDrawBatchLights(node);
		draw_batch.Add(node);
	}
	else
	{
		Mesh& mesh = *o.mesh;
		if(IsSet(mesh.head.flags, Mesh::F_TANGENTS))
			node->flags |= SceneNode::F_HAVE_TANGENTS;

		// for simplicity original node in unused and freed at end
		for(int i = 0; i < mesh.head.n_subs; ++i)
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
				draw_batch.Add(node2, i);
				if(draw_batch.gatherLights)
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
		if(game_level->city_ctx)
		{
			if(IsSet(game_level->city_ctx->flags, City::HaveExit))
			{
				for(vector<EntryPoint>::const_iterator entry_it = game_level->city_ctx->entry_points.begin(), entry_end = game_level->city_ctx->entry_points.end();
					entry_it != entry_end; ++entry_it)
				{
					const EntryPoint& e = *entry_it;
					Area& a = Add1(draw_batch.areas);
					a.v[0] = Vec3(e.exit_region.v1.x, e.exit_y, e.exit_region.v2.y);
					a.v[1] = Vec3(e.exit_region.v2.x, e.exit_y, e.exit_region.v2.y);
					a.v[2] = Vec3(e.exit_region.v1.x, e.exit_y, e.exit_region.v1.y);
					a.v[3] = Vec3(e.exit_region.v2.x, e.exit_y, e.exit_region.v1.y);
				}
			}

			for(vector<InsideBuilding*>::const_iterator it = game_level->city_ctx->inside_buildings.begin(), end = game_level->city_ctx->inside_buildings.end(); it != end; ++it)
			{
				const InsideBuilding& ib = **it;
				Area& a = Add1(draw_batch.areas);
				a.v[0] = Vec3(ib.enter_region.v1.x, ib.enter_y, ib.enter_region.v2.y);
				a.v[1] = Vec3(ib.enter_region.v2.x, ib.enter_y, ib.enter_region.v2.y);
				a.v[2] = Vec3(ib.enter_region.v1.x, ib.enter_y, ib.enter_region.v1.y);
				a.v[3] = Vec3(ib.enter_region.v2.x, ib.enter_y, ib.enter_region.v1.y);
			}
		}

		if(!game_level->city_ctx || !IsSet(game_level->city_ctx->flags, City::HaveExit))
		{
			const float H1 = -10.f;
			const float H2 = 30.f;

			// up
			{
				Area& a = Add1(draw_batch.areas);
				a.v[0] = Vec3(33.f, H1, 256.f - 33.f);
				a.v[1] = Vec3(33.f, H2, 256.f - 33.f);
				a.v[2] = Vec3(256.f - 33.f, H1, 256.f - 33.f);
				a.v[3] = Vec3(256.f - 33.f, H2, 256.f - 33.f);
			}

			// bottom
			{
				Area& a = Add1(draw_batch.areas);
				a.v[0] = Vec3(33.f, H1, 33.f);
				a.v[1] = Vec3(256.f - 33.f, H1, 33.f);
				a.v[2] = Vec3(33.f, H2, 33.f);
				a.v[3] = Vec3(256.f - 33.f, H2, 33.f);
			}

			// left
			{
				Area& a = Add1(draw_batch.areas);
				a.v[0] = Vec3(33.f, H1, 33.f);
				a.v[1] = Vec3(33.f, H2, 33.f);
				a.v[2] = Vec3(33.f, H1, 256.f - 33.f);
				a.v[3] = Vec3(33.f, H2, 256.f - 33.f);
			}

			// right
			{
				Area& a = Add1(draw_batch.areas);
				a.v[0] = Vec3(256.f - 33.f, H1, 256.f - 33.f);
				a.v[1] = Vec3(256.f - 33.f, H2, 256.f - 33.f);
				a.v[2] = Vec3(256.f - 33.f, H1, 33.f);
				a.v[3] = Vec3(256.f - 33.f, H2, 33.f);
			}
		}
		draw_batch.area_range = 10.f;
	}
	else if(locPart.partType == LocationPart::Type::Inside)
	{
		InsideLocation* inside = static_cast<InsideLocation*>(game_level->location);
		InsideLocationLevel& lvl = inside->GetLevelData();

		if(inside->HavePrevEntry())
			ListEntry(lvl.prevEntryType, lvl.prevEntryPt, lvl.prevEntryDir);
		if(inside->HaveNextEntry())
			ListEntry(lvl.nextEntryType, lvl.nextEntryPt, lvl.nextEntryDir);
		draw_batch.area_range = 5.f;
	}
	else
	{
		// exit from building
		Area& a = Add1(draw_batch.areas);
		const Box2d& region = static_cast<InsideBuilding&>(locPart).exit_region;
		a.v[0] = Vec3(region.v1.x, 0.1f, region.v2.y);
		a.v[1] = Vec3(region.v2.x, 0.1f, region.v2.y);
		a.v[2] = Vec3(region.v1.x, 0.1f, region.v1.y);
		a.v[3] = Vec3(region.v2.x, 0.1f, region.v1.y);
		draw_batch.area_range = 5.f;
	}

	// action area2
	if(pc->data.ability_ready || (pc->unit->action == A_CAST
		&& pc->unit->animation_state == AS_CAST_ANIMATION && Any(pc->unit->act.cast.ability->type, Ability::Summon, Ability::Trap)))
		PrepareAreaPath();
	else if(pc->unit->action == A_CAST && Any(pc->unit->act.cast.ability->type, Ability::Point, Ability::Ray))
		pc->unit->target_pos = pc->RaytestTarget(pc->unit->act.cast.ability->range);
	else if(pc->unit->weapon_state == WeaponState::Taken
		&& ((pc->unit->weapon_taken == W_BOW && Any(pc->unit->action, A_NONE, A_SHOOT))
			|| (pc->unit->weapon_taken == W_ONE_HANDED && IsSet(pc->unit->GetWeapon().flags, ITEM_WAND)
				&& Any(pc->unit->action, A_NONE, A_ATTACK, A_CAST, A_BLOCK, A_BASH))))
		pc->unit->target_pos = pc->RaytestTarget(50.f);

	if(draw_hitbox && pc->ShouldUseRaytest())
	{
		Vec3 pos;
		if(pc->data.ability_ready)
			pos = pc->data.ability_point;
		else
			pos = pc->unit->target_pos;
		DebugNode* node = DebugNode::Get();
		node->mat = Matrix::Scale(0.25f) * Matrix::Translation(pos) * draw_batch.camera->matViewProj;
		node->shape = MeshShape::Sphere;
		node->color = Color::Green;
		draw_batch.debug_nodes.push_back(node);
	}
}

//=================================================================================================
void Game::ListEntry(EntryType type, const Int2& pt, GameDirection dir)
{
	Area& a = Add1(draw_batch.areas);
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
	GameCamera& camera = game_level->camera;

	Ability* ability;
	if(pc->unit->action == A_CAST)
		ability = pc->unit->act.cast.ability;
	else
	{
		if(!pc->CanUseAbilityCheck())
			return;
		ability = pc->data.ability_ready;
	}

	switch(ability->type)
	{
	case Ability::Charge:
		{
			Area2* area_ptr = Area2::Get();
			Area2& area = *area_ptr;
			area.ok = 2;
			draw_batch.areas2.push_back(area_ptr);

			const Vec3 from = pc->unit->GetPhysicsPos();
			const float h = 0.06f;
			const float rot = Clip(pc->unit->rot + PI + pc->data.ability_rot);
			const int steps = 10;

			// find max line
			float t;
			Vec3 dir(sin(rot) * ability->range, 0, cos(rot) * ability->range);
			bool ignore_units = IsSet(ability->flags, Ability::IgnoreUnits);
			game_level->LineTest(pc->unit->cobj->getCollisionShape(), from, dir, [this, ignore_units](btCollisionObject* obj, bool)
			{
				int flags = obj->getCollisionFlags();
				if(IsSet(flags, CG_TERRAIN))
					return LT_IGNORE;
				if(IsSet(flags, CG_UNIT))
				{
					Unit* unit = reinterpret_cast<Unit*>(obj->getUserPointer());
					if(ignore_units || unit == pc->unit || !unit->IsStanding())
						return LT_IGNORE;
				}
				return LT_COLLIDE;
			}, t);

			float len = ability->range * t;

			if(game_level->location->outside && pc->unit->locPart->partType == LocationPart::Type::Outside)
			{
				// build line on terrain
				area.points.clear();
				area.faces.clear();

				Vec3 active_pos = pc->unit->pos;
				Vec3 step = dir * t / (float)steps;
				Vec3 unit_offset(pc->unit->pos.x, 0, pc->unit->pos.z);
				float len_step = len / steps;
				float active_step = 0;
				Matrix mat = Matrix::RotationY(rot);
				for(int i = 0; i < steps; ++i)
				{
					float current_h = game_level->terrain->GetH(active_pos) + h;
					area.points.push_back(Vec3::Transform(Vec3(-ability->width, current_h, active_step), mat) + unit_offset);
					area.points.push_back(Vec3::Transform(Vec3(+ability->width, current_h, active_step), mat) + unit_offset);

					active_pos += step;
					active_step += len_step;
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

			pc->data.ability_ok = true;
		}
		break;

	case Ability::Summon:
	case Ability::Trap:
		pc->data.ability_point = pc->RaytestTarget(ability->range);
		if(pc->data.range_ratio < 1.f)
		{
			Area2* area = Area2::Get();
			draw_batch.areas2.push_back(area);

			const float radius = ability->width / 2;
			Vec3 dir = pc->data.ability_point - pc->unit->pos;
			dir.y = 0;
			float dist = dir.Length();
			dir.Normalize();

			Vec3 from = pc->data.ability_point;
			from.y = 0;

			bool ok = false;
			for(int i = 0; i < 20; ++i)
			{
				game_level->global_col.clear();
				game_level->GatherCollisionObjects(*pc->unit->locPart, game_level->global_col, from, radius);
				if(!game_level->Collide(game_level->global_col, from, radius))
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
				pc->data.ability_ok = true;
				if(game_level->location->outside && pc->unit->locPart->partType == LocationPart::Type::Outside)
					game_level->terrain->SetY(from);
				if(!pc->data.ability_ready)
					pc->unit->target_pos = from;
				else
					pc->data.ability_point = from;
				area->ok = 2;
			}
			else
			{
				pc->data.ability_ok = false;
				area->ok = 0;
			}
			PrepareAreaPathCircle(*area, pc->data.ability_point, radius);
		}
		else
			pc->data.ability_ok = false;
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
				phy_world->rayTest(ToVector3(from), ToVector3(to), clbk);
				target = clbk.hit;
			}

			if(target)
			{
				pc->data.ability_ok = true;
				pc->data.ability_point = target->pos;
				pc->data.ability_target = target;
			}
			else
			{
				pc->data.ability_ok = false;
				pc->data.ability_target = nullptr;
			}
		}
		break;

	case Ability::Ray:
	case Ability::Point:
		pc->data.ability_point = pc->RaytestTarget(ability->range);
		pc->data.ability_ok = true;
		pc->data.ability_target = nullptr;
		break;

	case Ability::RangedAttack:
		pc->data.ability_ok = true;
		break;
	}
}

//=================================================================================================
void Game::PrepareAreaPathCircle(Area2& area, float radius, float range, float rot)
{
	const float h = 0.06f;
	const int circle_points = 10;

	area.points.resize(circle_points + 1);
	area.points[0] = Vec3(0, h, 0 + range);
	float angle = 0;
	for(int i = 0; i < circle_points; ++i)
	{
		area.points[i + 1] = Vec3(sin(angle) * radius, h, cos(angle) * radius + range);
		angle += (PI * 2) / circle_points;
	}

	bool outside = (game_level->location->outside && pc->unit->locPart->partType == LocationPart::Type::Outside);
	Matrix mat = Matrix::RotationY(rot);
	for(int i = 0; i < circle_points + 1; ++i)
	{
		area.points[i] = Vec3::Transform(area.points[i], mat) + pc->unit->pos;
		if(outside)
			area.points[i].y = game_level->terrain->GetH(area.points[i]) + h;
	}

	area.faces.clear();
	for(int i = 0; i < circle_points; ++i)
	{
		area.faces.push_back(0);
		area.faces.push_back(i + 1);
		area.faces.push_back((i + 2) == (circle_points + 1) ? 1 : (i + 2));
	}
}

//=================================================================================================
void Game::PrepareAreaPathCircle(Area2& area, const Vec3& pos, float radius)
{
	const float h = 0.06f;
	const int circle_points = 10;

	area.points.resize(circle_points + 1);
	area.points[0] = pos + Vec3(0, h, 0);
	float angle = 0;
	for(int i = 0; i < circle_points; ++i)
	{
		area.points[i + 1] = pos + Vec3(sin(angle) * radius, h, cos(angle) * radius);
		angle += (PI * 2) / circle_points;
	}

	bool outside = (game_level->location->outside && pc->unit->locPart->partType == LocationPart::Type::Outside);
	if(outside)
	{
		for(int i = 0; i < circle_points + 1; ++i)
			area.points[i].y = game_level->terrain->GetH(area.points[i]) + h;
	}

	area.faces.clear();
	for(int i = 0; i < circle_points; ++i)
	{
		area.faces.push_back(0);
		area.faces.push_back(i + 1);
		area.faces.push_back((i + 2) == (circle_points + 1) ? 1 : (i + 2));
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

	TopN<Light*, 3, float, std::less<>> best(nullptr, draw_batch.camera->zfar);

	if(draw_batch.locPart->masks.empty())
	{
		for(Light& light : draw_batch.locPart->lights)
		{
			float dist = Distance(x, z, light.pos.x, light.pos.z);
			if(dist < light.range + radius)
				best.Add(&light, dist);
		}
	}
	else
	{
		const Vec2 obj_pos(x, z);
		const bool is_split = (node && IsSet(node->mesh->head.flags, Mesh::F_SPLIT));

		for(GameLight& light : draw_batch.locPart->lights)
		{
			Vec2 light_pos;
			float dist;
			bool ok = true, masked = false;
			if(!is_split)
			{
				dist = Distance(x, z, light.pos.x, light.pos.z);
				if(dist > light.range + radius || !best.CanAdd(dist))
					continue;
				if(!IsZero(dist))
				{
					light_pos = light.pos.XZ();
					float range_sum = 0.f;

					// are there any masks between object and light?
					for(LightMask& mask : draw_batch.locPart->masks)
					{
						if(LineToRectangleSize(obj_pos, light_pos, mask.pos, mask.size))
						{
							// move light to one side of mask
							Vec2 new_pos, new_pos2;
							float new_dist[2];
							if(mask.size.x > mask.size.y)
							{
								new_pos.x = mask.pos.x - mask.size.x;
								new_pos2.x = mask.pos.x + mask.size.x;
								new_pos.y = new_pos2.y = mask.pos.y;
							}
							else
							{
								new_pos.x = new_pos2.x = mask.pos.x;
								new_pos.y = mask.pos.y - mask.size.y;
								new_pos2.y = mask.pos.y + mask.size.y;
							}
							new_dist[0] = Vec2::Distance(light_pos, new_pos) + Vec2::Distance(new_pos, obj_pos);
							new_dist[1] = Vec2::Distance(light_pos, new_pos2) + Vec2::Distance(new_pos2, obj_pos);
							if(new_dist[1] < new_dist[0])
								new_pos = new_pos2;

							// recalculate distance
							range_sum += Vec2::Distance(light_pos, new_pos);
							dist = range_sum + Distance(x, z, new_pos.x, new_pos.y);
							if(!best.CanAdd(dist))
							{
								ok = false;
								break;
							}
							light_pos = new_pos;
							masked = true;
						}
					}
				}
			}
			else
			{
				const Vec2 sub_size = node->mesh->splits[sub].box.SizeXZ();
				light_pos = light.pos.XZ();
				dist = DistanceRectangleToPoint(obj_pos, sub_size, light_pos);
				if(dist > light.range + radius || !best.CanAdd(dist))
					continue;
				if(!IsZero(dist))
				{
					float range_sum = 0.f;

					// are there any masks between object and light?
					for(LightMask& mask : draw_batch.locPart->masks)
					{
						if(LineToRectangleSize(obj_pos, light_pos, mask.pos, mask.size))
						{
							// move light to one side of mask
							Vec2 new_pos, new_pos2;
							float new_dist[2];
							if(mask.size.x > mask.size.y)
							{
								new_pos.x = mask.pos.x - mask.size.x;
								new_pos2.x = mask.pos.x + mask.size.x;
								new_pos.y = new_pos2.y = mask.pos.y;
							}
							else
							{
								new_pos.x = new_pos2.x = mask.pos.x;
								new_pos.y = mask.pos.y - mask.size.y;
								new_pos2.y = mask.pos.y + mask.size.y;
							}
							new_dist[0] = Vec2::Distance(light_pos, new_pos) + DistanceRectangleToPoint(obj_pos, sub_size, new_pos);
							new_dist[1] = Vec2::Distance(light_pos, new_pos2) + DistanceRectangleToPoint(obj_pos, sub_size, new_pos2);
							if(new_dist[1] < new_dist[0])
								new_pos = new_pos2;

							// recalculate distance
							range_sum += Vec2::Distance(light_pos, new_pos);
							dist = range_sum + DistanceRectangleToPoint(obj_pos, sub_size, new_pos);
							if(!best.CanAdd(dist))
							{
								ok = false;
								break;
							}
							light_pos = new_pos;
							masked = true;
						}
					}
				}
			}

			if(!ok)
				continue;

			if(masked)
			{
				float range = light.range - Vec2::Distance(light_pos, light.pos.XZ());
				if(range > 0)
				{
					Light* tmp_light = DrawBatch::light_pool.Get();
					tmp_light->color = light.color;
					tmp_light->pos = Vec3(light_pos.x, light.pos.y, light_pos.y);
					tmp_light->range = range;
					best.Add(tmp_light, dist);
					draw_batch.tmp_lights.push_back(tmp_light);
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
	sceneMgr->SetScene(draw_batch.scene, draw_batch.camera);

	// sky
	if(draw_batch.scene->useLightDir && IsSet(draw_flags, DF_SKYBOX))
		skybox_shader->Draw(*game_res->aSkybox, *draw_batch.camera);

	// terrain
	if(!draw_batch.terrain_parts.empty())
		terrain_shader->Draw(draw_batch.scene, draw_batch.camera, game_level->terrain, draw_batch.terrain_parts);

	// dungeon
	if(!draw_batch.dungeon_parts.empty())
		DrawDungeon(draw_batch.dungeon_parts, draw_batch.dungeon_part_groups);

	// nodes
	if(!draw_batch.nodes.empty())
		sceneMgr->DrawSceneNodes(draw_batch);

	// grass
	DrawGrass();

	// debug nodes
	if(!draw_batch.debug_nodes.empty())
		basic_shader->DrawDebugNodes(draw_batch.debug_nodes);
	if(pathfinding->IsDebugDraw())
	{
		basic_shader->Prepare(game_level->camera);
		basic_shader->SetAreaParams(Vec3::Zero, 100.f, 0.f);
		pathfinding->Draw(basic_shader);
	}

	// blood
	if(!draw_batch.bloods.empty())
		DrawBloods(draw_batch.bloods);

	// particles
	if(!draw_batch.billboards.empty() || !draw_batch.pes.empty() || draw_batch.tpes)
	{
		particle_shader->Prepare(game_level->camera);
		if(!draw_batch.billboards.empty())
			particle_shader->DrawBillboards(draw_batch.billboards);
		if(draw_batch.tpes)
			particle_shader->DrawTrailParticles(*draw_batch.tpes);
		if(!draw_batch.pes.empty())
			particle_shader->DrawParticles(draw_batch.pes);
	}

	// alpha nodes
	if(!draw_batch.alphaNodes.empty())
		sceneMgr->DrawAlphaSceneNodes(draw_batch);

	// areas
	if(!draw_batch.areas.empty() || !draw_batch.areas2.empty())
		DrawAreas(draw_batch.areas, draw_batch.area_range, draw_batch.areas2);
}

//=================================================================================================
void Game::DrawDungeon(const vector<DungeonPart>& parts, const vector<DungeonPartGroup>& groups)
{
	SuperShader* shader = sceneMgr->superShader;

	render->SetBlendState(Render::BLEND_NO);
	render->SetDepthState(Render::DEPTH_YES);
	render->SetRasterState(Render::RASTER_NORMAL);

	shader->Prepare();
	shader->SetCustomMesh(dun_mesh_builder->vb, dun_mesh_builder->ib, sizeof(VTangent));

	const bool useFog = sceneMgr->useFog && sceneMgr->useLighting;
	TexOverride* last_override = nullptr;

	for(vector<DungeonPart>::const_iterator it = parts.begin(), end = parts.end(); it != end; ++it)
	{
		const DungeonPart& dp = *it;

		// set textures
		if(last_override != dp.tex_o)
		{
			shader->SetShader(shader->GetShaderId(false, true, false, useFog, sceneMgr->useSpecularmap && dp.tex_o->specular != nullptr,
				sceneMgr->useNormalmap && dp.tex_o->normal != nullptr, sceneMgr->useLighting, false));
			shader->SetTexture(dp.tex_o, nullptr, 0);
			last_override = dp.tex_o;
		}

		const DungeonPartGroup& group = groups[dp.group];
		shader->DrawCustom(group.mat_world, group.mat_combined, group.lights, dp.start_index, dp.primitive_count * 3);
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
		decal.tex = game_res->tBloodSplat[blood->type]->tex;
		decal.lights = &blood->lights;
		shader->DrawDecal(decal);
	}
}

//=================================================================================================
void Game::DrawAreas(const vector<Area>& areas, float range, const vector<Area2*>& areas2)
{
	basic_shader->Prepare(*draw_batch.camera);

	if(!areas.empty())
	{
		Vec3 playerPos = pc->unit->pos;
		playerPos.y += 0.75f;

		basic_shader->SetAreaParams(playerPos, range, 1.f);

		const Color color = Color(0.f, 1.f, 0.f, 0.5f);
		for(const Area& area : areas)
			basic_shader->DrawQuad(area.v, color);

		basic_shader->Draw();
	}

	if(!areas2.empty())
	{
		basic_shader->SetAreaParams(Vec3::Zero, 100.f, 0.f);

		const Color colors[3] = {
			Color(1.f, 0.f, 0.f, 0.5f),
			Color(1.f, 1.f, 0.f, 0.5f),
			Color(0.f, 0.58f, 1.f, 0.5f)
		};

		for(Area2* area2 : areas2)
			basic_shader->DrawArea(area2->points, area2->faces, colors[area2->ok]);

		basic_shader->Draw();
	}
}

//=================================================================================================
void Game::UvModChanged()
{
	game_level->terrain->uv_mod = uv_mod;
	game_level->terrain->RebuildUv();
}

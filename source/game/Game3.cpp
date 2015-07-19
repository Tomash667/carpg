#include "Pch.h"
#include "Game3.h"

int bone, aix, state;
float value;

/*
void PostacPredraw2(void* ptr, MATRIX* m, int n)
{
	if(state != n)
		return;

	Unit3& u = *(Unit3*)ptr;

	MATRIX mat2;

	if(aix == 0)
		D3DXMatrixRotationX(&mat2, value);
	else if(aix == 1)
		D3DXMatrixRotationY(&mat2, value);
	else
		D3DXMatrixRotationZ(&mat2, value);

	m[bone] = m[bone] * mat2;
}*/

void PostacPredraw2(void* ptr, MATRIX* m, int n)
{
	if(n != 0)
		return;

	Unit3& u = *(Unit3*)ptr;

	int bone_glowa = u.ani->ani->GetBone("glowa")->id,
		bone_usta = u.ani->ani->GetBone("usta")->id,
		bone_szyja = u.ani->ani->GetBone("szyja")->id,
		bone_kregoslup = u.ani->ani->GetBone("kregoslup")->id;

	MATRIX mat2;
	float rot = u.look_rot;

	

	if(not_zero(rot) || not_zero(u.look_rotX))
	{
		D3DXMatrixRotationYawPitchRoll(&mat2, 0, u.look_rotX/3, rot/3);

		m[bone_glowa] = m[bone_glowa] * mat2;
		m[bone_usta] = m[bone_usta] * mat2;
		m[bone_szyja] = mat2 * m[bone_szyja];
		m[bone_kregoslup] = mat2 * m[bone_kregoslup];
	}
}

void Game3::InitGame()
{
	AddDir("data2");
	podloga = LoadMesh("podloga.qmsh");
	czlowiek = LoadMesh("czlowiek.qmsh");
	rurka = LoadMesh("rurka.qmsh");
	bron = LoadMesh("sejmitar.qmsh");

	eAni = CompileShader("anim.fx");
	eMesh = CompileShader("mesh.fx");
	eParticle = CompileShader("particle.fx");
	eSkybox = CompileShader("skybox.fx");
	eTerrain = CompileShader("terrain.fx");
	eArea = CompileShader("area.fx");

	SetupShaders();

	u1.pos = VEC3(-5,0,0);
	u1.rot = PI*3/2;
	u1.ani = new AnimeshInstance(czlowiek);
	u1.ani->ptr = &u1;
	u1.anim = A3_STOI;
	u1.prev_anim = A3_BRAK;
	u1.look_rot = 0.f;
	u1.look_rotX = 0.f;

	u2.pos = VEC3(5,0,0);
	u2.rot = PI/2;
	u2.ani = new AnimeshInstance(czlowiek);
	u2.ani->ptr = NULL;
	u2.anim = A3_STOI;
	u2.prev_anim = A3_BRAK;

	rotY = 4.2875104f;
	u1.look_pos = u2.pos;

	AnimeshInstance::Predraw = PostacPredraw2;
}

void Game3::SetupShaders()
{
	techAnim = eAni->GetTechniqueByName("anim");
	techHair = eAni->GetTechniqueByName("hair");
	techAnimDir = eAni->GetTechniqueByName("anim_dir");
	techHairDir = eAni->GetTechniqueByName("hair_dir");
	techMesh = eMesh->GetTechniqueByName("mesh");
	techMeshDir = eMesh->GetTechniqueByName("mesh_dir");
	techMeshSimple = eMesh->GetTechniqueByName("mesh_simple");
	techMeshSimple2 = eMesh->GetTechniqueByName("mesh_simple2");
	techMeshExplo = eMesh->GetTechniqueByName("mesh_explo");
	techParticle = eParticle->GetTechniqueByName("particle");
	techTrail = eParticle->GetTechniqueByName("trail");
	techSkybox = eSkybox->GetTechniqueByName("skybox");
	techTerrain = eTerrain->GetTechniqueByName("terrain");
	techArea = eArea->GetTechniqueByName("area");
	assert(techAnim && techHair && techAnimDir && techHairDir && techMesh && techMeshDir && techMeshSimple && techMeshSimple2 && techMeshExplo && techParticle && techTrail && techSkybox &&
		techTerrain && techArea);

	hAniCombined = eAni->GetParameterByName(NULL, "matCombined");
	hAniWorld = eAni->GetParameterByName(NULL, "matWorld");
	hAniBones = eAni->GetParameterByName(NULL, "matBones");
	hAniTex = eAni->GetParameterByName(NULL, "texDiffuse");
	hAniFogColor = eAni->GetParameterByName(NULL, "fogColor");
	hAniFogParam = eAni->GetParameterByName(NULL, "fogParam");
	hAniTint = eAni->GetParameterByName(NULL, "tint");
	hAniHairColor = eAni->GetParameterByName(NULL, "hairColor");
	hAniAmbientColor = eAni->GetParameterByName(NULL, "ambientColor");
	hAniLightDir = eAni->GetParameterByName(NULL, "lightDir");
	hAniLightColor = eAni->GetParameterByName(NULL, "lightColor");
	hAniLights = eAni->GetParameterByName(NULL, "lights");
	assert(hAniCombined && hAniWorld && hAniBones && hAniTex && hAniFogColor && hAniFogParam && hAniTint && hAniHairColor && hAniAmbientColor && hAniLightDir && hAniLightColor && hAniLights);

	hMeshCombined = eMesh->GetParameterByName(NULL, "matCombined");
	hMeshWorld = eMesh->GetParameterByName(NULL, "matWorld");
	hMeshTex = eMesh->GetParameterByName(NULL, "texDiffuse");
	hMeshFogColor = eMesh->GetParameterByName(NULL, "fogColor");
	hMeshFogParam = eMesh->GetParameterByName(NULL, "fogParam");
	hMeshTint = eMesh->GetParameterByName(NULL, "tint");
	hMeshAmbientColor = eMesh->GetParameterByName(NULL, "ambientColor");
	hMeshLightDir = eMesh->GetParameterByName(NULL, "lightDir");
	hMeshLightColor = eMesh->GetParameterByName(NULL, "lightColor");
	hMeshLights = eMesh->GetParameterByName(NULL, "lights");
	assert(hMeshCombined && hMeshWorld && hMeshTex && hMeshFogColor && hMeshFogParam && hMeshTint && hMeshAmbientColor && hMeshLightDir && hMeshLightColor && hMeshLights);

	hParticleCombined = eParticle->GetParameterByName(NULL, "matCombined");
	hParticleTex = eParticle->GetParameterByName(NULL, "tex0");
	assert(hParticleCombined && hParticleTex);

	hSkyboxCombined = eSkybox->GetParameterByName(NULL, "matCombined");
	hSkyboxTex = eSkybox->GetParameterByName(NULL, "tex0");
	assert(hSkyboxCombined && hSkyboxTex);

	hTerrainCombined = eTerrain->GetParameterByName(NULL, "matCombined");
	hTerrainWorld = eTerrain->GetParameterByName(NULL, "matWorld");
	hTerrainTexBlend = eTerrain->GetParameterByName(NULL, "texBlend");
	hTerrainTex[0] = eTerrain->GetParameterByName(NULL, "tex0");
	hTerrainTex[1] = eTerrain->GetParameterByName(NULL, "tex1");
	hTerrainTex[2] = eTerrain->GetParameterByName(NULL, "tex2");
	hTerrainTex[3] = eTerrain->GetParameterByName(NULL, "tex3");
	hTerrainTex[4] = eTerrain->GetParameterByName(NULL, "tex4");
	hTerrainColorAmbient = eTerrain->GetParameterByName(NULL, "colorAmbient");
	hTerrainColorDiffuse = eTerrain->GetParameterByName(NULL, "colorDiffuse");
	hTerrainLightDir = eTerrain->GetParameterByName(NULL, "lightDir");
	hTerrainFogColor = eTerrain->GetParameterByName(NULL, "fogColor");
	hTerrainFogParam = eTerrain->GetParameterByName(NULL, "fogParam");
	assert(hTerrainCombined && hTerrainWorld && hTerrainTexBlend && hTerrainTex[0] && hTerrainTex[1] && hTerrainTex[2] && hTerrainTex[3] && hTerrainTex[4] &&
		hTerrainColorAmbient && hTerrainColorDiffuse && hTerrainLightDir && hTerrainFogColor && hTerrainFogParam);

	hAreaCombined = eArea->GetParameterByName(NULL, "matCombined");
	hAreaColor = eArea->GetParameterByName(NULL, "color");
	assert(hAreaCombined && hAreaColor);
}

void Game3::OnChar(char c)
{

}

void Game3::OnCleanup()
{

}

void Game3::OnDraw()
{
	MATRIX mat, matView, matProj, matViewProj, matViewInv;

	const VEC3 cam_h(0, 1.8f, 0);
	VEC3 dist(0,-5.f,0);

	D3DXMatrixRotationYawPitchRoll(&mat, u1.rot, rotY, 0);
	D3DXVec3TransformCoord(&dist, &dist, &mat);

	VEC3 to = u1.pos + cam_h;
	VEC3 from = to + dist;

	D3DXMatrixLookAtLH(&matView, &from, &to, &VEC3(0,1,0));
	D3DXMatrixPerspectiveFovLH(&matProj, PI/4, float(wnd_size.x)/wnd_size.y, 0.1f, 20.f);
	matViewProj = matView * matProj;
	D3DXMatrixInverse(&matViewInv, NULL, &matView);

	VEC4 fog_color(1,1,1,1);
	VEC4 fog_params(10.f,20.f,10.f,0);
	VEC4 light_dir(1, 1, 1, 0);
	D3DXVec4Normalize(&light_dir, &light_dir);
	VEC4 ambient_color(0.3f, 0.3f, 0.3f, 1);
	VEC4 light_color(1, 1, 1, 1);

	V( eMesh->SetVector(hMeshFogColor, &fog_color) );
	V( eMesh->SetVector(hMeshFogParam, &fog_params) );
	V( eMesh->SetVector(hMeshLightDir, &light_dir) );
	V( eMesh->SetVector(hMeshLightColor, &light_color) );
	V( eMesh->SetVector(hMeshAmbientColor, &ambient_color) );
	V( eMesh->SetVector(hMeshTint, &VEC4(1,1,1,1)) );
	V( eAni->SetVector(hAniFogColor, &fog_color) );
	V( eAni->SetVector(hAniFogParam, &fog_params) );
	V( eAni->SetVector(hAniLightDir, &light_dir) );
	V( eAni->SetVector(hAniLightColor, &light_color) );
	V( eAni->SetVector(hAniAmbientColor, &ambient_color) );
	V( eAni->SetVector(hAniTint, &VEC4(1,1,1,1)) );

	UINT passes;

	// pod³oga
	V( eMesh->SetTechnique(techMeshDir) );
	V( eMesh->Begin(&passes, 0) );
	V( eMesh->BeginPass(0) );

	MATRIX matWorld, matCombined;
	D3DXMatrixIdentity(&matWorld);
	matCombined = matWorld * matViewProj;

	V( eMesh->SetMatrix(hMeshCombined, &matCombined) );
	V( eMesh->SetMatrix(hMeshWorld, &matWorld) );

	Animesh& mesh = *podloga;

	//V( device->SetFVF(mesh.fvf) );
	V( device->SetStreamSource(0, mesh.vb, 0, mesh.vertex_size) );
	V( device->SetIndices(mesh.ib) );

	for(int i=0; i<mesh.head.n_subs; ++i)
	{
		V( eMesh->SetTexture(hMeshTex, mesh.GetTexture(i)) );
		V( eMesh->CommitChanges() );
		V( device->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, mesh.subs[i].min_ind, mesh.subs[i].n_ind,
			mesh.subs[i].first*3, mesh.subs[i].tris) );
	}

	V( eMesh->EndPass() );
	V( eMesh->End() );

	/*{
		V( eMesh->Begin(&passes, 0) );
		V( eMesh->BeginPass(0) );

		MATRIX mat2, mat1;
		D3DXMatrixRotationYawPitchRoll(&mat2, 0, u1.look_rotX, u1.look_rot+u1.rot);
		D3DXMatrixTranslation(&mat1, VEC3(u1.pos.x, u1.pos.y+1.8f, u1.pos.z));
		matWorld = mat2*mat1;
		matCombined = matWorld * matViewProj;

		V( eMesh->SetMatrix(hMeshCombined, &matCombined) );
		V( eMesh->SetMatrix(hMeshWorld, &matWorld) );

		Animesh& mesh = *rurka;

		V( device->SetFVF(mesh.fvf) );
		V( device->SetStreamSource(0, mesh.vb, 0, mesh.vertex_size) );
		V( device->SetIndices(mesh.ib) );

		for(int i=0; i<mesh.head.n_subs; ++i)
		{
			V( eMesh->SetTexture(hMeshTex, mesh.GetTexture(i)) );
			V( eMesh->CommitChanges() );
			V( device->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, mesh.subs[i].min_ind, mesh.subs[i].n_ind,
				mesh.subs[i].first*3, mesh.subs[i].tris) );
		}

		V( eMesh->EndPass() );
		V( eMesh->End() );
	}*/

	// ludzie
	eAni->SetTechnique(techAnimDir);
	eMesh->SetTechnique(techMeshDir);

	DrawUnit(u1, matViewProj);
	DrawUnit(u2, matViewProj);

	struct CV
	{
		VEC3 pos;
		DWORD color;
	};

	

	CV pts[2];
	pts[0].pos = u1.GetEyePos();
	pts[0].color = BLACK;
	pts[1].pos = u2.GetEyePos();
	pts[1].color = BLACK;

	device->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
	device->SetRenderState(D3DRS_ZENABLE, FALSE);
	//device->SetFVF(D3DFVF_XYZ|D3DFVF_DIFFUSE);
	D3DXMatrixIdentity(&matWorld);
	device->SetTransform(D3DTS_WORLD, &matWorld);
	device->SetTransform(D3DTS_VIEW, &matView);
	device->SetTransform(D3DTS_PROJECTION, &matProj);
	device->DrawPrimitiveUP(D3DPT_LINELIST, 1, pts, sizeof(CV));

	/*pts[0].color = RED;
	pts[1].color = RED;
	VEC3 dir(0,10,0);
	MATRIX mat2;
	D3DXMatrixRotationYawPitchRoll(&mat2, 0, u1.look_rotX, u1.look_rot+u1.rot);
	D3DXVec3TransformCoord(&pts[1].pos, &dir, &mat2);
	pts[1].pos += pts[0].pos;
	device->DrawPrimitiveUP(D3DPT_LINELIST, 1, pts, sizeof(CV));*/

	device->SetRenderState(D3DRS_CULLMODE, D3DCULL_CCW);
	device->SetRenderState(D3DRS_ZENABLE, TRUE);

	clear_color = WHITE;

	sprite->Begin(D3DXSPRITE_ALPHABLEND);
	RECT rect={0,0,800,600};
	font->DrawTextA(sprite, Format("Axis: %c\nBone: %d (%s)\nState: %d\nValue: %g", (aix == 0 ? 'X' : (aix == 1 ? 'Y' : 'Z')), bone, czlowiek->bones[bone].name.c_str(), state, value), -1, &rect,
		DT_LEFT, BLACK);
	sprite->End();
}

void Game3::DrawUnit(Unit3& u, MATRIX& matViewProj)
{
	Animesh& mesh = *u.ani->ani;

	//V( device->SetFVF(mesh.fvf) );
	V( device->SetStreamSource(0, mesh.vb, 0, mesh.vertex_size) );
	V( device->SetIndices(mesh.ib) );

	UINT passes;

	eAni->Begin(&passes, 0);
	eAni->BeginPass(0);

	u.ani->SetupBones();
	eAni->SetMatrixArray(hAniBones, &u.ani->mat_bones[0], u.ani->ani->head.n_bones);

	MATRIX matPos, matRot, matWorld, matCombined;
	D3DXMatrixTranslation(&matPos, u.pos);
	D3DXMatrixRotationY(&matRot, u.rot);
	matWorld = matRot * matPos;
	matCombined = matWorld * matViewProj;

	eAni->SetMatrix(hAniCombined, &matCombined);
	eAni->SetMatrix(hAniWorld, &matWorld);

	for(int i=0; i<mesh.head.n_subs; ++i)
	{
		V( eAni->SetTexture(hAniTex, mesh.GetTexture(i)) );
		V( eAni->CommitChanges() );
		V( device->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, mesh.subs[i].min_ind, mesh.subs[i].n_ind,
			mesh.subs[i].first*3, mesh.subs[i].tris) );
	}

	eAni->EndPass();
	eAni->End();

	//=============================================================================================================
	// broñ
	V( eMesh->Begin(&passes, 0) );
	V( eMesh->BeginPass(0) );

	Animesh& weapon = *bron;
	Animesh::Point* point = u.ani->ani->GetPoint("bron");
	assert(point);

	MATRIX m1, m2, m3;

	m2 = point->mat * u.ani->mat_bones[point->bone] * matWorld;

	D3DXMatrixMultiply(&m3, &m2, &matViewProj);
	V( eMesh->SetMatrix(hMeshCombined, &m3) );
	V( eMesh->SetMatrix(hMeshWorld, &m2) );

	//V( device->SetFVF(weapon.fvf) );
	V( device->SetStreamSource(0, weapon.vb, 0, weapon.vertex_size) );
	V( device->SetIndices(weapon.ib) );

	for(word submesh=0; submesh<weapon.head.n_subs; ++submesh)
	{
		V( eMesh->SetTexture(hMeshTex, weapon.GetTexture(submesh)) );
		V( eMesh->CommitChanges() );
		V( device->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, weapon.subs[submesh].min_ind, weapon.subs[submesh].n_ind,
			weapon.subs[submesh].first*3, weapon.subs[submesh].tris) );
	}

	eMesh->EndPass();
	eMesh->End();

	// hitbox broni
	/*if(draw_hitbox && u.stan_broni == BRON_WYJETA && u.wyjeta == W_ONE_HANDED)
	{
		V( device->SetRenderState(D3DRS_ZENABLE, FALSE) );
		V( device->SetRenderState(D3DRS_FILLMODE, D3DFILL_WIREFRAME) );

		Animesh::Point* box = weapon.FindPoint("hit");
		assert(box && box->IsBox());

		D3DXMatrixMultiply(&m4, &box->mat, &m2);
		D3DXMatrixMultiply(&m3, &m4, &cam.matViewProj);

		V( eMesh->SetMatrix(hMeshCombined, &m3) );
		V( eMesh->SetMatrix(hMeshWorld, &m4) );

		V( device->SetFVF(aBox->fvf) );
		V( device->SetStreamSource(0, aBox->vb, 0, aBox->vertex_size) );
		V( device->SetIndices(aBox->ib) );

		V( eMesh->SetTexture(hMeshTex, NULL) );
		V( eMesh->CommitChanges() );

		V( device->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, aBox->subs[0].min_ind,aBox->subs[0].n_ind,
			aBox->subs[0].first*3, aBox->subs[0].tris) );

		V( device->SetRenderState(D3DRS_ZENABLE, TRUE) );
		V( device->SetRenderState(D3DRS_FILLMODE, D3DFILL_SOLID) );
	}*/
}

void Game3::OnReload()
{
	
}

void Game3::OnReset()
{

}

void Game3::OnResize()
{

}

void Game3::OnTick(float dt)
{
	if(Key.Pressed(VK_ESCAPE) || (Key.Down(VK_MENU) && Key.Down(VK_F4)))
	{
		EngineShutdown();
		return;
	}

	int rotate = 0;
	if(Key.Down('Q'))
		--rotate;
	if(Key.Down('E'))
		++rotate;
	if(rotate)
		u1.rot = clip(u1.rot+3.f*dt*rotate);
	int move = 0;
	if(Key.Down('W'))
		++move;
	if(Key.Down('S'))
		--move;
	if(move)
	{
		u1.pos += VEC3(sin(u1.rot+PI), 0, cos(u1.rot+PI))*5.f*dt*float(move);
		u1.anim = A3_IDZIE;
	}
	else
		u1.anim = A3_BOJOWA;

	rotY -= float(mouse_dif.y)/600;
	u1.rot += float(mouse_dif.x)/600;

	if(Key.Pressed(VK_LBUTTON))
		u1.ani->Play(rand2()%2 == 0 ? "atak_s_1" : "atak_s_2", PLAY_PRIO2|PLAY_ONCE|PLAY_BLEND_WAIT, 1);
	if(Key.Pressed(VK_RBUTTON))
	{
		cstring c;
		switch(rand2()%5)
		{
		default:
		case 0:
			c = "atak1";
			break;
		case 1:
			c = "atak2";
			break;
		case 2:
			c = "atak3";
			break;
		case 3:
			c = "atak4";
			break;
		case 4:
			c = "atak5";
			break;
		}
		u1.ani->Play(c, PLAY_PRIO2|PLAY_ONCE|PLAY_BLEND_WAIT, 1);
	}

	if(u1.anim != u1.prev_anim)
	{
		u1.prev_anim = u1.anim;
		switch(u1.anim)
		{
		case A3_STOI:
			u1.ani->Play("stoi", PLAY_PRIO2, 0);
			break;
		case A3_IDZIE:
			u1.ani->Play("idzie", PLAY_PRIO2, 0);
			break;
		case A3_BOJOWA:
			u1.ani->Play("bojowy", PLAY_PRIO2, 0);
			break;
		case A3_BOJOWA2:
			u1.ani->Play("bojowy2", PLAY_PRIO2, 0);
			break;
		}
	}
	u1.ani->Update(dt);
	u1.ani->need_update = true;

	if(u2.anim != u2.prev_anim)
	{
		u2.prev_anim = u2.anim;
		switch(u2.anim)
		{
		case A3_STOI:
			u2.ani->Play("stoi", PLAY_PRIO2, 0);
			break;
		case A3_IDZIE:
			u2.ani->Play("idzie", PLAY_PRIO2, 0);
			break;
		case A3_BOJOWA:
			u2.ani->Play("bojowy", PLAY_PRIO2, 0);
			break;
		case A3_BOJOWA2:
			u2.ani->Play("bojowy2", PLAY_PRIO2, 0);
			break;
		}
	}
	u2.ani->Update(dt);

	if(Key.Pressed(VK_ADD))
		++bone;
	if(Key.Pressed(VK_SUBTRACT) && bone)
		--bone;
	if(Key.Down('X'))
		aix = 0;
	if(Key.Down('Y'))
		aix = 1;
	if(Key.Down('Z'))
		aix = 2;
	if(Key.Down('1'))
		state = 0;
	if(Key.Down('2'))
		state = 1;
	if(Key.Down('3'))
		state = 2;
	if(Key.Down(VK_LEFT))
		value -= dt;
	if(Key.Down(VK_RIGHT))
		value += dt;

	if(Key.Down(VK_UP))
		u2.pos.y += 2.f*dt;
	if(Key.Down(VK_DOWN))
		u2.pos.y -= 2.f*dt;

	float new_rot = shortestArc(u1.rot, lookat_angle(u1.pos, u1.look_pos));

	const float angle_limit = PI*3/4;
	if(new_rot > angle_limit)
		new_rot = angle_limit;
	else if(new_rot < -angle_limit)
		new_rot = -angle_limit;

	VEC3 p1 = u1.pos,
		 p2 = u2.pos;

	float d = distance(p1, p2);
	float new_rot_x = sin((p2.y-p1.y)/d);

	float sum = abs(new_rot) + abs(new_rot_x);
	if(sum > angle_limit)
	{
		sum /= angle_limit;
		new_rot /= sum;
		new_rot_x /= sum;
	}

	if(u1.look_rotX > new_rot_x)
	{
		u1.look_rotX -= 12.f*dt;
		if(u1.look_rotX < new_rot_x)
			u1.look_rotX = new_rot_x;
	}
	else
	{
		u1.look_rotX += 12.f*dt;
		if(u1.look_rotX > new_rot_x)
			u1.look_rotX = new_rot_x;
	}

	if(u1.look_rot > new_rot)
	{
		u1.look_rot -= 12.f*dt;
		if(u1.look_rot < new_rot)
			u1.look_rot = new_rot;
	}
	else
	{
		u1.look_rot += 12.f*dt;
		if(u1.look_rot > new_rot)
			u1.look_rot = new_rot;
	}

	//LOG(Format("Rot: %g, RotX: %g", u1.look_rot, u1.look_rotX));
}

VEC3 Unit3::GetEyePos()
{
	const Animesh::Point& point = *ani->ani->GetPoint("oczy");
	MATRIX matBone = point.mat * ani->mat_bones[point.bone];

	MATRIX matPos, matRot;
	D3DXMatrixTranslation(&matPos, pos);
	D3DXMatrixRotationY(&matRot, rot);

	matBone = matBone * (matRot * matPos);

	VEC3 pos(0,0,0), out;
	D3DXVec3TransformCoord(&out, &pos, &matBone);

	return out;
}

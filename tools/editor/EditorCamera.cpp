#include "Pch.h"
#include "EditorCamera.h"

#include <Engine.h>
#include <File.h>
#include <Input.h>

const float pitchLimit = PI / 2 - 0.1f;

//=================================================================================================
EditorCamera::EditorCamera() : allowMove(true)
{
	from = Vec3(0, 2, -2);
	to = Vec3::Zero;
	LookAt(to);
}

//=================================================================================================
void EditorCamera::LookAt(const Vec3& pos)
{
	Vec3 v = (pos - from).Normalize();
	yaw = Clip(atan2(v.x, v.z));
	pitch = asin(-v.y);
	if(pitch > pitchLimit)
		pitch = pitchLimit;
	else if(pitch < -pitchLimit)
		pitch = -pitchLimit;
	rotChanged = true;
	Update(0.f);
}

//=================================================================================================
void EditorCamera::Update(float dt)
{
	const Int2& dif = app::input->GetMouseDif();
	if(dif != Int2::Zero)
	{
		yaw = Clip(yaw + float(dif.x) / 300);
		pitch += float(dif.y) / 300;
		if(pitch > pitchLimit)
			pitch = pitchLimit;
		else if(pitch < -pitchLimit)
			pitch = -pitchLimit;
		rotChanged = true;
	}

	float forward = 0, right = 0, up = 0;
	if(allowMove)
	{
		if(app::input->IsModifier(KEY_NONE))
		{
			if(app::input->Down(Key::W) || app::input->Down(Key::Up))
				forward += 10.f;
			if(app::input->Down(Key::S) || app::input->Down(Key::Down))
				forward -= 10.f;
			if(app::input->Down(Key::A) || app::input->Down(Key::Left))
				right -= 10.f;
			if(app::input->Down(Key::D) || app::input->Down(Key::Right))
				right += 10.f;
		}
		else if(app::input->IsModifier(KEY_SHIFT))
		{
			if(app::input->Down(Key::W) || app::input->Down(Key::Up))
				up += 10.f;
			if(app::input->Down(Key::S) || app::input->Down(Key::Down))
				up -= 10.f;
		}
	}

	if(rotChanged || forward != 0 || right != 0 || up != 0)
	{
		Matrix mat_rot = Matrix::Rotation(pitch, yaw, 0);

		if(forward != 0)
			from += Vec3::Transform(Vec3::Forward, mat_rot) * forward * dt;

		if(right != 0)
			from += Vec3::Transform(Vec3::Right, mat_rot) * right * dt;

		if(up != 0)
			from += Vec3::Transform(Vec3::Up, mat_rot) * up * dt;

		to = from + Vec3::Transform(Vec3(0, 0, 1), mat_rot);
		rotChanged = false;
		Matrix matView = Matrix::CreateLookAt(from, to);
		Matrix matProj = Matrix::CreatePerspectiveFieldOfView(PI / 4, app::engine->GetWindowAspect(), znear, zfar);
		mat_view_proj = matView * matProj;
		mat_view_inv = matView.Inverse();
	}
}

//=================================================================================================
void EditorCamera::Save(FileWriter& f)
{
	f << from;
	f << yaw;
	f << pitch;
}

//=================================================================================================
void EditorCamera::Load(FileReader& f)
{
	f >> from;
	f >> yaw;
	f >> pitch;
	rotChanged = true;
}

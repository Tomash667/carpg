#pragma once

#include <Camera.h>

struct EditorCamera : public Camera
{
	EditorCamera();
	void LookAt(const Vec3& pos);
	void Update(float dt);
	void Save(FileWriter& f);
	void Load(FileReader& f);

	float yaw, pitch;
	bool rotChanged, allowMove;
};

#pragma once

#include "SceneNode2.h"

class Camera2 : public SceneNode2
{
public:
	const FrustumPlanes& GetFrustum() const;
};

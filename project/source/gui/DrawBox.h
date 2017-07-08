#pragma once

#include "Control.h"
#include "Resource.h"

class Scene2;
class SceneNode2;

namespace gui
{
	class DrawBox : public Control
	{
	public:
		DrawBox();

		void Draw(ControlDrawData* cdd = nullptr) override;
		void Update(float dt) override;
		Mesh* GetMesh() const { return mesh; }

		TEX GetTexture() const { return tex; }

		bool IsMesh() const { return mesh != nullptr; }
		bool IsTexture() const { return tex != nullptr; }

		void SetMesh(Mesh* mesh);
		void SetTexture(TEX tex);

	private:
		Scene2* scene;
		SceneNode2* node;
		Mesh* mesh;
		TEX tex;
		INT2 tex_size, click_point;
		float scale, default_scale;
		VEC2 move;
		bool clicked;
	};
}

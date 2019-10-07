#pragma once

class GameResources
{
public:
	static constexpr int ITEM_IMAGE_SIZE = 64;

	GameResources();
	~GameResources();
	void Init();
	void GenerateItemIconTask(TaskData& task_data);
	void GenerateItemIcon(Item& item);
	void DrawItemIcon(const Item& item, RenderTarget* target, float rot);

private:
	typedef std::map<Mesh*, Texture*> ItemTextureMap;
	ItemTextureMap item_texture_map;
	vector<Texture*> over_item_textures;
	Texture missing_item_texture;
	RenderTarget* rt_item;
	Scene* scene;
	SceneNode* node, *light;
	Camera* camera;
};

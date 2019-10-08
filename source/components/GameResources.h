#pragma once

class GameResources
{
public:
	static constexpr int ITEM_IMAGE_SIZE = 64;

	~GameResources();
	void Init();
	void LoadData();
	void GenerateItemIconTask(TaskData& task_data);
	void GenerateItemIcon(Item& item);
	void DrawItemIcon(const Item& item, RenderTarget* target, float rot);

	Mesh* mesh_human;

private:
	void CreateMissingTexture();

	typedef std::map<Mesh*, Texture*> ItemTextureMap;
	ItemTextureMap item_texture_map;
	vector<Texture*> over_item_textures;
	RenderTarget* rt_item;
	Texture missing_item_texture;
};

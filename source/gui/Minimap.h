#pragma once

//-----------------------------------------------------------------------------
#include "GamePanel.h"

//-----------------------------------------------------------------------------
// Location minimap
class Minimap : public GamePanel
{
public:
	Minimap();
	void LoadData();
	void Draw() override;
	void Update(float dt) override;
	void Event(GuiEvent e) override;
	bool NeedCursor() const override { return false; }
	void Show();
	void Hide();
	void Build();
	Vec2 GetMapPosition(Unit& unit);

	// przekszata³ca z pozycji œwiata do punktu na mapie (gdzie punkt (0,0) to lewy dolny róg mapy, a (1,1) to prawy górny)
	Vec2 TransformCoord(const Vec2& pt)
	{
		return Vec2(pt.x / (2 * minimapSize), 1.f - (pt.y / (2 * minimapSize)));
	}

	Vec2 TransformTile(const Int2& tile)
	{
		return Vec2((float(tile.x) + 0.5f) / minimapSize, 1.f - (float(tile.y) + 0.5f) / minimapSize);
	}

	// konwersji z (0-1) do punktu na ekranie
	Vec2 Convert(const Vec2& pt)
	{
		return Vec2(pt.x*size.x + globalPos.x, pt.y*size.y + globalPos.y);
	}

	// przekszta³ca punkt z pozycji œwiata do pixela na ekranie
	Vec2 PosToPoint(const Vec2& ps)
	{
		return Convert(TransformCoord(ps));
	}

	Vec2 TileToPoint(const Int2& tile)
	{
		return Convert(TransformTile(tile));
	}

	City* city;

private:
	struct Text
	{
		cstring text;
		Int2 size;
		// pos i anchor s¹ jako punkt(0-1)
		Vec2 pos, anchor;
	};

	Texture* GetEntryImage(EntryType type);

	vector<Text> texts;
	TexturePtr tUnit[5], tStairsDown, tStairsUp, tBag, tBagImportant, tPortal, tChest, tDoor;
	int minimapSize;
};

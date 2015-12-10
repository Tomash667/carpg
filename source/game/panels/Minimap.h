#pragma once

//-----------------------------------------------------------------------------
#include "GamePanel.h"

//-----------------------------------------------------------------------------
struct City;

//-----------------------------------------------------------------------------
class Minimap : public GamePanel
{
public:
	Minimap();
	void Draw(ControlDrawData* cdd = nullptr);
	void Update(float dt);
	void Event(GuiEvent e);
	bool NeedCursor() const { return false; }

	void Show();
	void Hide();
	void Build();

	// przekszata³ca z pozycji œwiata do punktu na mapie (gdzie punkt (0,0) to lewy dolny róg mapy, a (1,1) to prawy górny)
	inline VEC2 TransformCoord(const VEC2& pt)
	{
		return VEC2(pt.x/(2*minimap_size), 1.f-(pt.y/(2*minimap_size)));
	}
	
	inline VEC2 TransformTile(const INT2& tile)
	{
		return VEC2((float(tile.x)+0.5f)/minimap_size, 1.f-(float(tile.y)+0.5f)/minimap_size);
	}

	// konwersji z (0-1) do punktu na ekranie
	inline VEC2 Convert(const VEC2& pt)
	{
		return VEC2(pt.x*size.x+global_pos.x, pt.y*size.y+global_pos.y);
	}

	// przekszta³ca punkt z pozycji œwiata do pixela na ekranie
	inline VEC2 PosToPoint(const VEC2& ps)
	{
		return Convert(TransformCoord(ps));
	}

	inline VEC2 TileToPoint(const INT2& tile)
	{
		return Convert(TransformTile(tile));
	}

	struct Text
	{
		cstring text;
		INT2 size;
		// pos i anchor s¹ jako punkt(0-1)
		VEC2 pos, anchor;
	};

	int minimap_size;
	MATRIX m1;
	vector<Text> texts;
	City* city;
};

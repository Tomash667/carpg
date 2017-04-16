#pragma once

// helper class to calculate clipping
class GuiRect
{
private:
	float m_left, m_top, m_right, m_bottom, m_u, m_v, m_u2, m_v2;

public:
	void Set(uint width, uint height)
	{
		m_left = 0;
		m_top = 0;
		m_right = (float)width;
		m_bottom = (float)height;
		m_u = 0;
		m_v = 0;
		m_u2 = 1;
		m_v2 = 1;
	}

	void Set(uint width, uint height, const RECT& part)
	{
		m_left = (float)part.left;
		m_top = (float)part.top;
		m_right = (float)part.right;
		m_bottom = (float)part.bottom;
		m_u = m_left / width;
		m_v = m_top / height;
		m_u2 = m_right / width;
		m_v2 = m_bottom / height;
	}

	void Set(const BOX2D& box, const BOX2D* uv)
	{
		m_left = box.v1.x;
		m_top = box.v1.y;
		m_right = box.v2.x;
		m_bottom = box.v2.y;
		if(uv)
		{
			m_u = uv->v1.x;
			m_v = uv->v1.y;
			m_u2 = uv->v2.x;
			m_v2 = uv->v2.y;
		}
		else
		{
			m_u = 0;
			m_v = 0;
			m_u2 = 1;
			m_v2 = 1;
		}
	}

	void Set(const INT2& pos, const INT2& size)
	{
		m_left = (float)pos.x;
		m_right = (float)(pos.x + size.x);
		m_top = (float)pos.y;
		m_bottom = (float)(pos.y + size.y);
	}

	void Set(const VEC2& pos1, const VEC2& pos2, const VEC2& uv1, const VEC2& uv2)
	{
		m_left = pos1.x;
		m_right = pos2.x;
		m_top = pos1.y;
		m_bottom = pos2.y;
		m_u = uv1.x;
		m_v = uv1.y;
		m_u2 = uv2.x;
		m_v2 = uv2.y;
	}

	void Transform(const MATRIX* mat)
	{
		VEC2 leftTop(m_left, m_top);
		VEC2 rightBottom(m_right, m_bottom);
		D3DXVec2TransformCoord(&leftTop, &leftTop, mat);
		D3DXVec2TransformCoord(&rightBottom, &rightBottom, mat);
		m_left = leftTop.x;
		m_top = leftTop.y;
		m_right = rightBottom.x;
		m_bottom = rightBottom.y;
	}

	bool Clip(const RECT& clipping)
	{
		BOX2D box(clipping);
		return Clip(box);
	}

	bool Clip(const BOX2D& clipping)
	{
		int result = RequireClip(clipping);
		if(result == -1)
			return false; // no intersection
		else if(result == 0)
			return true; // fully inside

		const float left = max(m_left, clipping.Left());
		const float right = min(m_right, clipping.Right());
		const float top = max(m_top, clipping.Top());
		const float bottom = min(m_bottom, clipping.Bottom());
		const VEC2 orig_size(m_right - m_left, m_bottom - m_top);
		const float u = lerp(m_u, m_u2, (left - m_left) / orig_size.x);
		m_u2 = lerp(m_u, m_u2, 1.f - (m_right - right) / orig_size.x);
		m_u = u;
		const float v = lerp(m_v, m_v2, (top - m_top) / orig_size.y);
		m_v2 = lerp(m_v, m_v2, 1.f - (m_bottom - bottom) / orig_size.y);
		m_v = v;
		m_left = left;
		m_top = top;
		m_right = right;
		m_bottom = bottom;
		return true;
	}

	int RequireClip(const BOX2D& c)
	{
		if(m_left >= c.Right() || c.Left() >= m_right || m_top >= c.Bottom() || c.Top() >= m_bottom)
			return -1; // no intersection
		if(m_left >= c.Left() && m_right <= c.Right() && m_top >= c.Top() && m_bottom <= c.Bottom())
			return 0; // fully inside
		return 1; // require clip
	}

	void Populate(VParticle*& v, const VEC4& col)
	{
		v->pos = VEC3(m_left, m_top, 0);
		v->color = col;
		v->tex = VEC2(m_u, m_v);
		++v;

		v->pos = VEC3(m_right, m_top, 0);
		v->color = col;
		v->tex = VEC2(m_u2, m_v);
		++v;

		v->pos = VEC3(m_left, m_bottom, 0);
		v->color = col;
		v->tex = VEC2(m_u, m_v2);
		++v;

		v->pos = VEC3(m_left, m_bottom, 0);
		v->color = col;
		v->tex = VEC2(m_u, m_v2);
		++v;

		v->pos = VEC3(m_right, m_top, 0);
		v->color = col;
		v->tex = VEC2(m_u2, m_v);
		++v;

		v->pos = VEC3(m_right, m_bottom, 0);
		v->color = col;
		v->tex = VEC2(m_u2, m_v2);
		++v;
	}
};

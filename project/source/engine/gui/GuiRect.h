#pragma once

#include "VertexDeclaration.h"

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

	void Set(uint width, uint height, const Rect& part)
	{
		m_left = (float)part.Left();
		m_top = (float)part.Top();
		m_right = (float)part.Right();
		m_bottom = (float)part.Bottom();
		m_u = m_left / width;
		m_v = m_top / height;
		m_u2 = m_right / width;
		m_v2 = m_bottom / height;
	}

	void Set(const Box2d& box, const Box2d* uv)
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

	void Set(const Int2& pos, const Int2& size)
	{
		m_left = (float)pos.x;
		m_right = (float)(pos.x + size.x);
		m_top = (float)pos.y;
		m_bottom = (float)(pos.y + size.y);
	}

	void Set(const Vec2& pos1, const Vec2& pos2, const Vec2& uv1, const Vec2& uv2)
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

	void Transform(const Matrix& mat)
	{
		Vec2 leftTop(m_left, m_top);
		Vec2 rightBottom(m_right, m_bottom);
		leftTop = Vec2::Transform(leftTop, mat);
		rightBottom = Vec2::Transform(rightBottom, mat);
		m_left = leftTop.x;
		m_top = leftTop.y;
		m_right = rightBottom.x;
		m_bottom = rightBottom.y;
	}

	bool Clip(const Rect& clipping)
	{
		Box2d box(clipping);
		return Clip(box);
	}

	bool Clip(const Box2d& clipping)
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
		const Vec2 orig_size(m_right - m_left, m_bottom - m_top);
		const float u = Lerp(m_u, m_u2, (left - m_left) / orig_size.x);
		m_u2 = Lerp(m_u, m_u2, 1.f - (m_right - right) / orig_size.x);
		m_u = u;
		const float v = Lerp(m_v, m_v2, (top - m_top) / orig_size.y);
		m_v2 = Lerp(m_v, m_v2, 1.f - (m_bottom - bottom) / orig_size.y);
		m_v = v;
		m_left = left;
		m_top = top;
		m_right = right;
		m_bottom = bottom;
		return true;
	}

	int RequireClip(const Box2d& c)
	{
		if(m_left >= c.Right() || c.Left() >= m_right || m_top >= c.Bottom() || c.Top() >= m_bottom)
			return -1; // no intersection
		if(m_left >= c.Left() && m_right <= c.Right() && m_top >= c.Top() && m_bottom <= c.Bottom())
			return 0; // fully inside
		return 1; // require clip
	}

	void Populate(VParticle*& v, const Vec4& col)
	{
		v->pos = Vec3(m_left, m_top, 0);
		v->color = col;
		v->tex = Vec2(m_u, m_v);
		++v;

		v->pos = Vec3(m_right, m_top, 0);
		v->color = col;
		v->tex = Vec2(m_u2, m_v);
		++v;

		v->pos = Vec3(m_left, m_bottom, 0);
		v->color = col;
		v->tex = Vec2(m_u, m_v2);
		++v;

		v->pos = Vec3(m_left, m_bottom, 0);
		v->color = col;
		v->tex = Vec2(m_u, m_v2);
		++v;

		v->pos = Vec3(m_right, m_top, 0);
		v->color = col;
		v->tex = Vec2(m_u2, m_v);
		++v;

		v->pos = Vec3(m_right, m_bottom, 0);
		v->color = col;
		v->tex = Vec2(m_u2, m_v2);
		++v;
	}

	Rect ToRect() const
	{
		return Rect((int)m_left, (int)m_top, (int)m_right, (int)m_bottom);
	}
};

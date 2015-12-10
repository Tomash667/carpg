#include "Pch.h"
#include "Base.h"
#include "Terrain.h"

//-----------------------------------------------------------------------------
void CalculateNormal(VTerrain& v1,VTerrain& v2,VTerrain& v3)
{
    VEC3 normal;
    VEC3 v01 = v2.pos - v1.pos;
	VEC3 v02 = v3.pos - v1.pos;

    D3DXVec3Cross(&normal, &v01, &v02);

    D3DXVec3Normalize(&normal, &normal);

    v1.normal = normal;
	v2.normal = normal;
	v3.normal = normal;
}

//-----------------------------------------------------------------------------
void CalculateNormal(VEC3& out,const VEC3& v1,const VEC3& v2,const VEC3& v3)
{
	VEC3 v01 = v2 - v1;
	VEC3 v02 = v3 - v1;

	D3DXVec3Cross(&out, &v01, &v02);
	D3DXVec3Normalize(&out, &out);
}

//=================================================================================================
Terrain::Terrain() : parts(nullptr), h(nullptr), mesh(nullptr), texSplat(nullptr), tex(), state(0), uv_mod(DEFAULT_UV_MOD)
{
}

//=================================================================================================
Terrain::~Terrain()
{
	if(mesh)
		mesh->Release();

	if(texSplat)
		texSplat->Release();

	delete[] parts;
	// h jest przechowywany w OutsideLocation wiêc nie mo¿na tu usuwaæ
	//delete[] h;
	state = 0;
}

//=================================================================================================
void Terrain::Init(IDirect3DDevice9* dev, const TerrainOptions& o)
{
	assert(state == 0);
	assert(dev && o.tile_size > 0.f && o.n_parts > 0 && o.tiles_per_part > 0 /*&& is_pow2(o.tex_size)*/);

	device = dev;
	pos = VEC3(0,0,0);
	tile_size = o.tile_size;
	n_parts = o.n_parts;
	n_parts2 = n_parts * n_parts;
	tiles_per_part = o.tiles_per_part;
	n_tiles = n_parts * tiles_per_part;
	n_tiles2 = n_tiles * n_tiles;
	tiles_size = tile_size * n_tiles;
	hszer = n_tiles + 1;
	hszer2 = hszer * hszer;
	n_tris = n_tiles2 * 2;
	n_verts = n_tiles2 * 6;
	part_tris = tiles_per_part*tiles_per_part*2;
	part_verts = tiles_per_part*tiles_per_part*6;
	tex_size = o.tex_size;
	box.v1 = VEC3(0,0,0);
	box.v2.x = box.v2.z = tile_size * n_tiles;
	box.v2.y = 0;

	h = new float[hszer2];
	parts = new Part[n_parts2];
	VEC3 diff = box.v2 - box.v1;
	diff /= float(n_parts);
	diff.y = 0;
	for(uint z=0; z<n_parts; ++z)
	{
		for(uint x=0; x<n_parts; ++x)
		{
			parts[x+z*n_parts].box.v1 = box.v1 + VEC3(diff.x*x,0,diff.z*z);
			parts[x+z*n_parts].box.v2 = parts[x+z*n_parts].box.v1 + diff;
		}
	}

	CreateSplatTexture();

	state = 1;
}

//=================================================================================================
void Terrain::CreateSplatTexture()
{
	DWORD usage = D3DUSAGE_AUTOGENMIPMAP;

	HRESULT hr = D3DXCreateTexture(device, tex_size, tex_size, 1, usage, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, &texSplat);
	if(FAILED(hr))
		throw Format("Failed to create terrain texture with size '%d' (%d)!", tex_size, hr);
}

//=================================================================================================
void Terrain::Build(bool smooth)
{
	assert(state == 1);

	HRESULT hr = D3DXCreateMeshFVF(n_tris, n_verts, D3DXMESH_MANAGED|D3DXMESH_32BIT, VTerrain::fvf, device, &mesh);
	if(FAILED(hr))
		throw Format("Failed to create new terrain mesh (%d)!", hr);

	VTerrain* v;
	//word* idx;
	uint* idx;
	uint n = 0;
	//uint index = 0;

	V( mesh->LockVertexBuffer(0, (void**)&v) );
	V( mesh->LockIndexBuffer(0, (void**)&idx) );

#define TRI(xx,zz,uu,vv) v[n++] = VTerrain((x+xx)*tile_size, h[x+xx+(z+zz)*hszer], (z+zz)*tile_size, float(uu)/uv_mod, float(vv)/uv_mod,\
	((float)(x+xx)) / n_tiles, ((float)(z+zz)) / n_tiles)

	for(uint z=0; z<n_tiles; ++z)
	{
		for(uint x=0; x<n_tiles; ++x)
		{
			int u1 = (x%uv_mod);
			int u2 = ((x+1)%uv_mod);
			if(u2 == 0)
				u2 = uv_mod;
			int v1 = (z%uv_mod);
			int v2 = ((z+1)%uv_mod);
			if(v2 == 0)
				v2 = uv_mod;

			TRI(0,0,u1,v1);
			TRI(0,1,u1,v2);
			TRI(1,0,u2,v1);
			CalculateNormal(v[n-3],v[n-2],v[n-1]);

			TRI(0,1,u1,v2);
			TRI(1,1,u2,v2);
			TRI(1,0,u2,v1);
			CalculateNormal(v[n-3],v[n-2],v[n-1]);
		}
	}
#undef TRI

	for(uint z=0; z<n_parts; ++z)
	{
		for(uint x=0; x<n_parts; ++x)
		{
			const uint z_start = z*tiles_per_part,
			z_end = z_start + tiles_per_part,
			x_start = x*tiles_per_part,
			x_end = x_start + tiles_per_part;

			for(uint zz=z_start; zz<z_end; ++zz)
			{
				for(uint xx=x_start; xx<x_end; ++xx)
				{
					for(uint j=0; j<6; ++j)
					{
						*idx = (xx+zz*n_tiles)*6+j;
						++idx;
					}
				}
			}
		}
	}

	V( mesh->UnlockIndexBuffer() );

	state = 2;

	if(smooth)
		SmoothNormals(v);

	V( mesh->UnlockVertexBuffer() );
}

//=================================================================================================
void Terrain::Rebuild(bool smooth)
{
	assert(state == 2);

	VTerrain* v;

	V( mesh->LockVertexBuffer(0, (void**)&v) );

#define TRI(xx,zz) v[n++].pos.y = h[x+xx+(z+zz)*hszer]

	uint n = 0;
	for(uint z=0; z<n_tiles; ++z)
	{
		for(uint x=0; x<n_tiles; ++x)
		{
			TRI(0,0);
			TRI(0,1);
			TRI(1,0);
			CalculateNormal(v[n-3],v[n-2],v[n-1]);

			TRI(0,1);
			TRI(1,1);
			TRI(1,0);
			CalculateNormal(v[n-3],v[n-2],v[n-1]);
		}
	}
#undef TRI

	if(smooth)
		SmoothNormals(v);

	V( mesh->UnlockVertexBuffer() );
}

//=================================================================================================
void Terrain::RebuildUv()
{
	assert(state == 2);

	VTerrain* v;

	V( mesh->LockVertexBuffer(0, (void**)&v) );

#define TRI(uu,vv) v[n++].tex = VEC2(float(uu)/uv_mod, float(vv)/uv_mod)

	uint n = 0;
	for(uint z=0; z<n_tiles; ++z)
	{
		for(uint x=0; x<n_tiles; ++x)
		{
			int u1 = (x%uv_mod);
			int u2 = ((x+1)%uv_mod);
			if(u2 == 0)
				u2 = uv_mod;
			int v1 = (z%uv_mod);
			int v2 = ((z+1)%uv_mod);
			if(v2 == 0)
				v2 = uv_mod;

			TRI(u1,v1);
			TRI(u1,v2);
			TRI(u2,v1);

			TRI(u1,v2);
			TRI(u2,v2);
			TRI(u2,v1);
		}
	}
#undef TRI

	V( mesh->UnlockVertexBuffer() );
}

//=================================================================================================
void Terrain::Make(bool smooth)
{
	assert(state != 0);
	if(state == 1)
		Build(smooth);
	else
		Rebuild(smooth);
}

//=================================================================================================
void Terrain::ApplyTextures(ID3DXEffect* effect)
{
	assert(state > 1);

	assert(effect);

	V( effect->SetTexture("tex0", tex[0]) );
	V( effect->SetTexture("tex1", tex[1]) );
	V( effect->SetTexture("tex2", tex[2]) );
	V( effect->SetTexture("tex3", tex[3]) );
	V( effect->SetTexture("tex4", tex[4]) );
	V( effect->SetTexture("texBlend", texSplat) );
}

//=================================================================================================
void Terrain::ApplyStreamSource()
{
	VB vb;
	IB ib;

	V( mesh->GetVertexBuffer(&vb) );
	V( mesh->GetIndexBuffer(&ib) );
	V( device->SetStreamSource(0, vb, 0, sizeof(VTerrain)) );
	V( device->SetIndices(ib) );
}

//=================================================================================================
void Terrain::SetHeight(float height)
{
	assert(state > 0);

	for(uint i=0; i<hszer2; ++i)
		h[i] = height;

	box.v1.y = height-0.1f;
	box.v2.y = height+0.1f;

	for(uint i=0; i<n_parts2; ++i)
	{
		parts[i].box.v1.y = height-0.1f;
		parts[i].box.v2.y = height+0.1f;
	}
}

//=================================================================================================
void Terrain::RandomizeHeight(float hmin,float hmax)
{
	assert(state > 0);
	assert(hmin < hmax);

	for(uint i=0; i<hszer2; ++i)
		h[i] = random(hmin,hmax);
}

//=================================================================================================
void Terrain::RoundHeight()
{
	assert(state > 0);

	float sum;
	int ile;

#define H(xx,zz) h[x+(xx)+(z+(zz))*hszer]

	// aby przyœpieszyæ t¹ funkcje mo¿na by obliczyæ najpierw dla krawêdzi a
	// potem œrodek w osobnych pêtlach, wtedy nie potrzeba by by³o ¿adnych
	// warunków, na razie jest to jednak nie potrzebna bo ta funkcja nie jest
	// u¿ywana realtime tylko w edytorze przy tworzeniu losowego terenu
	for(uint z=0; z<hszer; ++z)
	{
		for(uint x=0; x<hszer; ++x)
		{
			ile = 1;
			sum = H(0,0);

			// wysokoœæ z prawej
			if(x < hszer-1)
			{
				sum += H(1,0);
				++ile;
			}
			// wysokoœæ z lewej
			if(x > 0)
			{
				sum += H(-1,0);
				++ile;
			}
			// wysokoœæ z góry
			if(z < hszer-1)
			{
				sum += H(0,1);
				++ile;
			}
			// wysokoœæ z do³u
			if(z > 0)
			{
				sum += H(0,-1);
				++ile;
			}
			// mo¿na by dodaæ jeszcze elementy na ukos
			H(0,0) = sum / ile;
		}
	}
}

//=================================================================================================
void Terrain::CalculateBox()
{
	assert(state > 0);

	float smax = -inf(),
		smin = inf(),
		pmax, pmin, hc;

	for(uint i=0; i<n_parts*n_parts; ++i)
	{
		pmax = -inf();
		pmin = inf();

		const uint z_start = (i/n_parts)*tiles_per_part,
			z_end = z_start + tiles_per_part,
			x_start = (i%n_parts)*tiles_per_part,
			x_end = x_start + tiles_per_part;

		for(uint z=z_start; z<=z_end; ++z)
		{
			for(uint x=x_start; x<=x_end; ++x)
			{
				hc = h[x+z*hszer];
				if(hc > pmax)
					pmax = hc;
				if(hc < pmin)
					pmin = hc;
			}
		}

		pmin -= 0.1f;
		pmax += 0.1f;

		parts[i].box.v1.y = pmin;
		parts[i].box.v2.y = pmax;

		if(pmax > smax)
			smax = pmax;
		if(pmin < smin)
			smin = pmin;
	}

	box.v1.y = smin;
	box.v2.y = smax;
}

//=================================================================================================
void Terrain::Randomize()
{
	assert(state > 0);

	RandomizeHeight(0.f,8.f);
	RoundHeight();
	RoundHeight();
	RoundHeight();
	CalculateBox();
	//RandomizeTexture();
}

//=================================================================================================
void Terrain::SmoothNormals()
{
	assert(state > 0);

	VTerrain* v;
	V( mesh->LockVertexBuffer(0, (void**)&v) );

	SmoothNormals(v);

	V( mesh->UnlockVertexBuffer() );
}

//=================================================================================================
void Terrain::SmoothNormals(VTerrain* v)
{
	assert(state > 0);
	assert(v);

	VEC3 normal, normal2;
	uint sum;

	// 1,3         4
	//(0,1)		 (1,1)
	// O-----------O
	// |\          |
	// |  \        |
	// |    \      |
	// |      \    |
	// |        \  |
	// |          \|
	// O-----------O
	//(0,0)		 (1,0)
	//  0         2,5

#undef W
#define W(xx,zz,idx) v[(x+(xx)+(z+(zz))*n_tiles)*6+(idx)]
#define CalcNormal(xx,zz,i0,i1,i2) CalculateNormal(normal2,W(xx,zz,i0).pos,W(xx,zz,i1).pos,W(xx,zz,i2).pos);\
	normal += normal2

	// wyg³adŸ normalne
	for(uint z=0; z<n_tiles; ++z)
	{
		for(uint x=0; x<n_tiles; ++x)
		{
			bool has_left = (x > 0),
				 has_right = (x < n_tiles-1),
				 has_bottom = (z > 0),
				 has_top = (z < n_tiles-1);

			//------------------------------------------
			// PUNKT (0,0)
			sum = 1;
			normal = W(0,0,0).normal;

			if(has_left)
			{
				CalcNormal(-1,0,0,1,2);
				CalcNormal(-1,0,3,4,5);
				sum += 2;

				if(has_bottom)
				{
					CalcNormal(-1,-1,3,4,5);
					++sum;
				}
			}
			if(has_bottom)
			{
				CalcNormal(0,-1,0,1,2);
				CalcNormal(0,-1,3,4,5);
				sum += 2;
			}

			normal *= (1.f/sum);
			W(0,0,0).normal = normal;

			//------------------------------------------
			// PUNKT (0,1)
			normal = W(0,0,1).normal;
			normal += W(0,0,3).normal;
			sum = 2;

			if(has_left)
			{
				CalcNormal(-1,0,3,4,5);
				++sum;

				if(has_top)
				{
					CalcNormal(-1,1,0,1,2);
					CalcNormal(-1,1,3,4,5);
					sum += 2;
				}
			}
			if(has_top)
			{
				CalcNormal(0,1,0,1,2);
				++sum;
			}

			normal *= (1.f/sum);
			W(0,0,1).normal = normal;
			W(0,0,3).normal = normal;

			//------------------------------------------
			// PUNKT (1,0)
			normal = W(0,0,2).normal;
			normal += W(0,0,5).normal;
			sum = 2;

			if(has_right)
			{
				CalcNormal(1,0,0,1,2);
				++sum;

				if(has_bottom)
				{
					CalcNormal(1,-1,0,1,2);
					CalcNormal(1,-1,3,4,5);
					sum += 2;
				}
			}
			if(has_bottom)
			{
				CalcNormal(0,-1,3,4,5);
				++sum;
			}

			normal *= (1.f/sum);
			W(0,0,2).normal = normal;
			W(0,0,5).normal = normal;

			//------------------------------------------
			// PUNKT (1,1)
			normal = W(0,0,4).normal;
			sum = 1;

			if(has_right)
			{
				CalcNormal(1,0,0,1,2);
				CalcNormal(1,0,3,4,5);
				sum += 2;

				if(has_top)
				{
					CalcNormal(1,1,0,1,2);
					++sum;
				}
			}
			if(has_top)
			{
				CalcNormal(0,1,0,1,2);
				CalcNormal(0,1,3,4,5);
				sum += 2;
			}

			normal *= (1.f/sum);
			W(0,0,4).normal = normal;
		}
	}
}

//=================================================================================================
float Terrain::GetH(float x, float z) const
{
	assert(state > 0);

	// sprawdŸ czy nie jest poza terenem
	assert(x >= 0.f && z >= 0.f);

	// oblicz które to kafle
	uint tx,tz;
	tx = (uint)floor(x/tile_size);
	tz = (uint)floor(z/tile_size);

	// sprawdŸ czy nie jest to poza terenem
	//assert_return(tx < n_tiles && tz < n_tiles, 0.f);
	// teren na samej krawêdzi wykrywa jako b³¹d
	if(tx == n_tiles)
	{
		if(equal(x,tiles_size))
			--tx;
		else
			assert(tx < n_tiles);
	}
	if(tz == n_tiles)
	{
		if(equal(z,tiles_size))
			--tz;
		else
			assert(tz < n_tiles);
	}

	// oblicz offset od kafla do punktu
	float offsetx, offsetz;
	offsetx = (x - tile_size*tx)/tile_size;
	offsetz = (z - tile_size*tz)/tile_size;

	// pobierz wysokoœci na krawêdziach
	float hTopLeft = h[tx+(tz+1)*hszer];
	float hTopRight = h[tx+1+(tz+1)*hszer];
	float hBottomLeft = h[tx+tz*hszer];
	float hBottomRight = h[tx+1+tz*hszer];

	// sprawdŸ który to trójk¹t (prawy górny czy lewy dolny)
	if((offsetx*offsetx + offsetz*offsetz) < ((1-offsetx)*(1-offsetx) + (1-offsetz)*(1-offsetz)))
	{
		// lewy dolny trójk¹t
		float dX = hBottomRight - hBottomLeft;
		float dZ = hTopLeft - hBottomLeft;
		return dX * offsetx + dZ * offsetz + hBottomLeft;
	}
	else
	{
		// prawy górny trójk¹t
		float dX = hTopLeft - hTopRight;
		float dZ = hBottomRight - hTopRight;
		return dX * (1 - offsetx) + dZ * (1 - offsetz) + hTopRight;
	}
}

//=================================================================================================
void Terrain::GetAngle(float x, float z, VEC3& angle) const
{
	// sprawdŸ czy nie jest to poza map¹
	assert(x >= 0.f && z >= 0.f);

	// oblicz które to kafle
	uint tx,tz;
	tx = (uint)floor(x/tile_size);
	tz = (uint)floor(z/tile_size);

	// sprawdŸ czy nie jest to poza map¹
	if(tx == n_tiles)
	{
		if(equal(x,tiles_size))
			--tx;
		else
			assert(tx < n_tiles);
	}
	if(tz == n_tiles)
	{
		if(equal(z,tiles_size))
			--tz;
		else
			assert(tz < n_tiles);
	}
	
	// oblicz offset od kafla do punktu
	float offsetx, offsetz;
	offsetx = (x - tile_size*tx)/tile_size;
	offsetz = (z - tile_size*tz)/tile_size;

	// pobierz wysokoœci na krawêdziach
	float hTopLeft = h[tx+(tz+1)*hszer];
	float hTopRight = h[tx+1+(tz+1)*hszer];
	float hBottomLeft = h[tx+tz*hszer];
	float hBottomRight = h[tx+1+tz*hszer];
	VEC3 v1,v2,v3;

	// sprawdŸ który to trójk¹t (prawy górny czy lewy dolny)
	if((offsetx*offsetx + offsetz*offsetz) < ((1-offsetx)*(1-offsetx) + (1-offsetz)*(1-offsetz)))
	{
		// lewy dolny trójk¹t
		v1 = VEC3(tile_size*tx,		hBottomLeft,	tile_size*tz);
		v2 = VEC3(tile_size*tx,		hTopLeft,		tile_size*(tz+1));
		v3 = VEC3(tile_size*(tx+1),	hBottomRight,	tile_size*(tz+1));
	}
	else
	{
		// prawy górny trójk¹t
		v1 = VEC3(tile_size*tx,		hTopLeft,		tile_size*(tz+1));
		v2 = VEC3(tile_size*(tx+1), hTopRight,		tile_size*(tz+1));
		v3 = VEC3(tile_size*(tx+1),	hBottomRight,	tile_size*tz);
	}

	// oblicz wektor normalny dla tych punktów
	VEC3 v01 = v2 - v1;
	VEC3 v02 = v3 - v1;

	D3DXVec3Cross(&angle, &v01, &v02);
	D3DXVec3Normalize(&angle, &angle);
}

// return false if there is no collision
bool Terrain::RaytestSimple(const VEC3& from, const VEC3& to) const
{
	VEC3 dir = to - from;
	float fout;

	if(!RayToBox(from, dir, box, &fout) || fout > 1.f)
		return false;

	// transform ray to terrain space
	VEC3 rayFrom = from - GetPos();
	VEC3 rayTo = to - GetPos();
	/*const VEC3& b = GetBox().v2;
	bool checked = false;
	bool above;

	if(rayFrom.x >= 0.f && rayFrom.z >= 0.f && rayFrom.x <= b.x && rayFrom.z <= b.z)
	{
		float lh = GetH(rayFrom.x, rayFrom.z);
		above = lh >= rayFrom.y;
		checked = true;
	}

	int x0 = (int)floor(rayFrom.x/tile_size);
	int z0 = (int)floor(rayFrom.z/tile_size);
	int x1 = (int)floor(rayTo.x/tile_size);
	int z1 = (int)floor(rayTo.z/tile_size);
	int dx = abs(x1-x0);
	int dz = abs(z1-z0);
	int sx = (x0 < x1 ? 1 : -1);
	int sz = (z0 < z1 ? 1 : -1);
	int err = dx-dy;
	const int nt = int(n_tiles+1);
	bool inside = false;
	
	while(true)
	{
		if(x0 >= 0 && y0 >= 0 && x0 < nt && y0 < nt)
		{
		}
		else if(inside)
			break;
	}
 
   loop
     setPixel(x0,y0)
     if x0 = x1 and y0 = y1 exit loop
     e2 := 2*err
     if e2 > -dy then 
       err := err - dy
       x0 := x0 + sx
     end if
     if e2 <  dx then 
       err := err + dx
       y0 := y0 + sy 
     end if
   end loop
	int tmp;

				var steep:Boolean = Math.abs(y1 - y0) > Math.abs(x1 - x0);
				var tmp:int;
	if (steep)
	{
		std::swap(x0, y0);

		tmp = x0;
		x0 = y0;
		y0 = tmp;
		tmp = x1;
		x1 = y1;
		y1 = tmp;
	}
	if (x0 > x1)
	{
		tmp = x0;
		x0 = x1;
		x1 = tmp;
		tmp = y0;
		y0 = y1;
		y1 = tmp;
	}
				var deltax:int = x1 - x0, deltay:int = Math.abs(y1 - y0), ystep:int, tty:int = y0, ttx:int;
				var error:int = deltax / 2;
				
				if (y0 < y1)
					ystep = 1;
				else
					ystep = -1;
				for (ttx = x0; ttx < x1; ++ttx)
				{
					if (steep)
						b.setPixel(tty, ttx, 0);
					else
						b.setPixel(ttx, tty, 0);
					error -= deltay;
					if (error < 0)
					{
						tty += ystep;
						error += deltax;
					}
				}*/

	// easier options
	/*const float dist = distance(from, to);
	const uint ti = uint(dist/tile_size) + 5;
	bool above = (GetH(rayFrom.x, rayFrom.z) >= rayFrom.y);
	bool first = true;
	for(uint i=1; i<ti; ++i)
	{
		const float dy = float(i)/ti;
		const float xx = rayFrom.x+dir.x*dy;
		const float zz = rayFrom.z+dir.z*dy;
		if(xx >= 0.f && zz >= 0.f && xx <= box.v2.x && zz <= box.v2.z)
		{
			first = false;
			if((GetH(xx, zz) >= (rayFrom.y+dir.y*dy)) != above)
				return true;
		}
		else if(!first)
			return false;
	}*/

	// another try, this time without checking outside terrain
	const float dist = distance(from, to);
	const uint ti = uint(dist/tile_size) + 5;
	int state;
	bool above;

	if(rayFrom.x >= 0.f && rayFrom.z >= 0.f && rayFrom.x <= box.v2.x && rayFrom.z <= box.v2.z)
	{
		state = 1;
		above = (GetH(rayFrom.x, rayFrom.z) >= rayFrom.y);
	}
	else
		state = 0;

	for(uint i=1; i<ti; ++i)
	{
		const float dy = float(i)/ti;
		const float xx = rayFrom.x+dir.x*dy;
		const float zz = rayFrom.z+dir.z*dy;
		if(xx >= 0.f && zz >= 0.f && xx <= box.v2.x && zz <= box.v2.z)
		{
			const float yy = rayFrom.y + dir.y*dy;
			if(state == 0)
			{
				state = 1;
				above = (GetH(xx, zz) >= yy);
			}
			else
			{
				if((GetH(xx, zz) >= yy) != above)
					return true;
			}
		}
		else if(state == 1)
			return false;
	}

	return false;
}

//=================================================================================================
float Terrain::Raytest(const VEC3& from, const VEC3& to) const
{
	VEC3 dir = to - from;
	float fout;

	if(!RayToBox(from, dir, box, &fout) || fout > 1.f)
		return -1.f;

	MATRIX m;
	D3DXMatrixTranslation(&m, pos);
	D3DXMatrixInverse(&m, nullptr, &m);
	VEC3 rayPos, rayDir;
	D3DXVec3TransformCoord(&rayPos, &from, &m);
	D3DXVec3TransformNormal(&rayDir, &dir, &m);
	BOOL hit;
	DWORD face;
	float bar1, bar2;

	{
		//START_PROFILE("Intersect");
		V( D3DXIntersect(mesh, &rayPos, &rayDir, &hit, &face, &bar1, &bar2, &fout, nullptr, nullptr) );
	}

	if(hit)
	{
		// tu by³ bug - fout zwraca odrazu wynik w przedziale (0..1) a nie tak jak myœla³em ¿e jest to dystans od startu do przeciêcia
		//printf("from %g,%g,%g\tto %g,%g,%g\tdist %g\thit %g\n", from.x, from.y, from.z, to.x, to.y, to.z, distance(from, to), fout);
		//fout /= distance(from, to);
		if(fout > 1.f)
			return -1.f;
		else
			return fout;
	}
	else
		return -1.f;
}

//=================================================================================================
void Terrain::FillGeometry(vector<Tri>& tris, vector<VEC3>& verts)
{
	uint vcount = (n_tiles+1)*(n_tiles+1);
	verts.reserve(vcount);

	for(uint z = 0; z <= (n_tiles+1); ++z)
	{
		for(uint x = 0; x <= (n_tiles+1); ++x)
		{
			verts.push_back(VEC3(x*tile_size, h[x+z*hszer], z*tile_size));
		}
	}

	tris.reserve(n_tiles * n_tiles * 3);

#define XZ(xx,zz) (x+(xx)+(z+(zz))*(n_tiles+1))

	for(uint z = 0; z < n_tiles; ++z)
	{
		for(uint x = 0; x < n_tiles; ++x)
		{
			tris.push_back(Tri(XZ(0,0), XZ(0,1), XZ(1,0)));
			tris.push_back(Tri(XZ(1,0), XZ(0,1), XZ(1,1)));
		}
	}

#undef XZ
}

//=================================================================================================
void Terrain::FillGeometryPart(vector<Tri>& tris, vector<VEC3>& verts, int px, int pz, const VEC3& offset) const
{
	assert(px >= 0 && pz >= 0 && uint(px) < n_parts && uint(pz) < n_parts);

	verts.reserve((tiles_per_part+1)*(tiles_per_part+1));

	for(uint z = pz*tiles_per_part; z <= (pz+1)*tiles_per_part; ++z)
	{
		for(uint x = px*tiles_per_part; x <= (px+1)*tiles_per_part; ++x)
		{
			verts.push_back(VEC3(x*tile_size + offset.x, h[x+z*hszer] + offset.y, z*tile_size + offset.z));
		}
	}

	tris.reserve(tiles_per_part * tiles_per_part * 3);

#define XZ(xx,zz) (x+(xx)+(z+(zz))*(tiles_per_part+1))

	for(uint z = 0; z < tiles_per_part; ++z)
	{
		for(uint x = 0; x < tiles_per_part; ++x)
		{
			tris.push_back(Tri(XZ(0,0), XZ(0,1), XZ(1,0)));
			tris.push_back(Tri(XZ(1,0), XZ(0,1), XZ(1,1)));
		}
	}

#undef XZ
}

//=================================================================================================
void Terrain::RemoveHeightMap(bool _delete)
{
	assert(h);

	if(_delete)
		delete[] h;

	h = nullptr;
}

//=================================================================================================
void Terrain::SetHeightMap(float* _h)
{
	assert(_h);

	h = _h;
}

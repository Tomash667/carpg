#pragma once

#include "Core.h"
#include "FileFormat.h"

// Engine types
struct Font;
struct MeshInstance;
struct ParticleEmitter;
struct StartupOptions;
struct Terrain;
struct TrailParticleEmitter;
class Engine;
class ResourceManager;
class SoundManager;

// Resource types
struct Mesh;
struct Resource;
struct Sound;
struct Texture;
struct VertexData;
typedef Mesh* MeshPtr;
typedef Sound* SoundPtr;
typedef Texture* TexturePtr;
typedef VertexData* VertexDataPtr;

// Gui types
class DialogBox;
class PickItemDialog;

// Windows types
struct HWND__;
typedef HWND__* HWND;

// DirectX types
struct _D3DPRESENT_PARAMETERS_;
struct _D3DXMACRO;
struct ID3DXEffect;
struct ID3DXEffectPool;
struct ID3DXFont;
struct ID3DXMesh;
struct ID3DXSprite;
struct IDirect3D9;
struct IDirect3DDevice9;
struct IDirect3DIndexBuffer9;
struct IDirect3DSurface9;
struct IDirect3DTexture9;
struct IDirect3DVertexBuffer9;
struct IDirect3DVertexDeclaration9;
typedef _D3DPRESENT_PARAMETERS_ D3DPRESENT_PARAMETERS;
typedef _D3DXMACRO D3DXMACRO;
typedef const char* D3DXHANDLE;
typedef ID3DXFont* FONT;
typedef IDirect3DIndexBuffer9* IB;
typedef IDirect3DSurface9* SURFACE;
typedef IDirect3DTexture9* TEX;
typedef IDirect3DVertexBuffer9* VB;

// FMod types
namespace FMOD
{
	class Channel;
	class ChannelGroup;
	class Sound;
	class System;
}
typedef FMOD::Sound* SOUND;

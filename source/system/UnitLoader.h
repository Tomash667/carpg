#pragma once

//-----------------------------------------------------------------------------
#include "ContentLoader.h"
#include "StatProfile.h"

//-----------------------------------------------------------------------------
class UnitLoader : public ContentLoader
{
	struct DialogRequest
	{
		string unit;
		string dialog;
	};

	friend class Content;

	void DoLoading() override;
	static void Cleanup();
	void InitTokenizer() override;
	void LoadEntity(int top, const string& id) override;
	void Finalize() override;
	void ParseUnit(const string& id);
	void ParseProfile(Scoped<StatProfile>& profile);
	void ParseSubprofile(Scoped<StatProfile::Subprofile>& subprofile);
	WEAPON_TYPE GetWeaponType();
	ARMOR_TYPE GetArmorType();
	void ParseItems(Scoped<ItemScript>& script);
	void AddItem(ItemScript* script);
	void ParseAbilities(Scoped<AbilityList>& list);
	void ParseSounds(Scoped<SoundPack>& pack);
	void ParseFrames(Scoped<FrameInfo>& frames);
	void ParseTextures(Scoped<TexPack>& pack);
	void ParseIdles(Scoped<IdlePack>& pack);
	void ParseAlias(const string& id);
	void ParseGroup(const string& id);
	void ParseGroupAlias(const string& id);
	void ProcessDialogRequests();

	vector<DialogRequest> dialogRequests;
};

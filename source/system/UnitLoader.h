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
	void ParseProfile(Ptr<StatProfile>& profile);
	void ParseSubprofile(Ptr<StatProfile::Subprofile>& subprofile);
	WEAPON_TYPE GetWeaponType();
	ARMOR_TYPE GetArmorType();
	void ParseItems(Ptr<ItemScript>& script);
	void AddItem(ItemScript* script);
	void ParseAbilities(Ptr<AbilityList>& list);
	void ParseSounds(Ptr<SoundPack>& pack);
	void ParseFrames(Ptr<FrameInfo>& frames);
	void ParseTextures(Ptr<TexPack>& pack);
	void ParseIdles(Ptr<IdlePack>& pack);
	void ParseAlias(const string& id);
	void ParseGroup(const string& id);
	void ParseGroupAlias(const string& id);
	void ProcessDialogRequests();

	vector<DialogRequest> dialogRequests;
};

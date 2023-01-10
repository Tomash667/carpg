#pragma once

//-----------------------------------------------------------------------------
#include "ContentLoader.h"
#include "BaseObject.h"

//-----------------------------------------------------------------------------
enum ObjectProperty;

//-----------------------------------------------------------------------------
class ObjectLoader : public ContentLoader
{
	friend class Content;
private:
	void DoLoading() override;
	static void Cleanup();
	void InitTokenizer() override;
	void LoadEntity(int top, const string& id) override;
	void Finalize() override;
	void ParseObject(const string& id);
	void ParseObjectProperty(ObjectProperty prop, BaseObject* obj);
	void ParseUsable(const string& id);
	void ParseGroup(const string& id);
	void ParseAlias(const string& id);
	void CalculateCrc();
	void UpdateObjectGroupCrc(Crc& crc, ObjectGroup::EntryList& list);
};

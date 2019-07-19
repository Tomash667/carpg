#pragma once

//-----------------------------------------------------------------------------
struct ObjectEntity
{
	enum Type
	{
		NONE,
		OBJECT,
		USABLE,
		CHEST
	} type;
	union
	{
		Object* object;
		Usable* usable;
		Chest* chest;
	};

	ObjectEntity(nullptr_t) : object(nullptr), type(NONE) {}
	ObjectEntity(Object* object) : object(object), type(OBJECT) {}
	ObjectEntity(Usable* usable) : usable(usable), type(USABLE) {}
	ObjectEntity(Chest* chest) : chest(chest), type(CHEST) {}

	operator bool()
	{
		return type != NONE;
	}
	operator Object* ()
	{
		assert(type == OBJECT || type == NONE);
		return object;
	}
	operator Usable* ()
	{
		assert(type == USABLE || type == NONE);
		return usable;
	}
	operator Chest* ()
	{
		assert(type == CHEST || type == NONE);
		return chest;
	}

	bool IsChest() const { return type == CHEST; }
	bool IsObject() const { return type == OBJECT; }
	bool IsUsable() const { return type == USABLE; }
};

#pragma once

#include "Type.h"

class TypeVectorContainer : public Type::Container
{
	typedef typename vector<TypeItem> Vector;
	typedef typename Vector::iterator It;

	struct Enumerator : public Type::Container::Enumerator
	{
		Enumerator(Vector& v);

		bool Next() override;

		It it, end;
	};

public:
	TypeVectorContainer(Type& type, Vector& v);
	~TypeVectorContainer();

	template<typename T>
	TypeVectorContainer(Type* type, vector<T>& items) : TypeVectorContainer(*type, (Vector&)items) {}

	void Add(TypeItem item) override;
	Ptr<Type::Container::Enumerator> GetEnumerator() override;
	uint Count() override;
	TypeItem Find(const string& id) override;
	void Merge(vector<TypeEntity*>& items, vector<TypeEntity*>& removed_items) override;

private:
	Type& type;
	Vector& items;
	uint id_offset;
};

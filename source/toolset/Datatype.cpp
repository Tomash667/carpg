#include "Pch.h"
#include "Base.h"
#include "Datatype.h"
#include "Building.h"

void A()
{
	Datatype* bg = new Datatype;
	bg->dtid = DT_BuildingGroup;
	bg->id = "building_group";
	bg->multi_id = "building_groups";
	bg->AddId(offsetof(BuildingGroup, id));
}

struct BuildingSchemeHandler : public CustomFieldHandler
{

};

struct BuildingShiftHandler : public CustomFieldHandler
{

};

void B()
{
	Datatype* b = new Datatype;
	b->dtid = DT_Building;
	b->id = "building";
	b->multi_id = "buildings";
	b->AddId(offsetof(Building, id));
	b->AddMesh("mesh_id", offsetof(Building, mesh_id), offsetof(Building, mesh));
	b->AddMesh("inside_mesh_id", offsetof(Building, inside_mesh_id), offsetof(Building, inside_mesh), true);
	b->AddFlags("flags", offsetof(Building, flags)); // define flags
	b->AddCustomField("scheme", new BuildingSchemeHandler);
	b->AddReference("group", DT_BuildingGroup, offsetof(Building, group), true);
	b->AddReference("unit", DT_Unit, offsetof(Building, unit), true);
	b->AddCustomField("shift", new BuildingShiftHandler, true);

	// name, offset
}

import bpy
from math import *
from mathutils import Vector
from bpy.props import *

bl_info = {
	"name" : "Capsule",
	"author" : "David Ludwig",
	"version" : (1,1),
	"blender" : (2,65,0),
	"location" : "View3D > Add > Mesh > Capsule",
	"description" : "Add a capsule mesh",
	"category" : "Add Mesh"
}

def create_mesh_object(context, verts, edges, faces, name):
	
	#create new mesh
	mesh = bpy.data.meshes.new(name)
	
	#make a mesh from a list of verts/edges/faces
	mesh.from_pydata(verts, edges, faces)
	
	#update mesh geometry after adding other stuff
	mesh.update()
	
	from bpy_extras import object_utils
	return object_utils.object_data_add(context, mesh, operator=None)


class OBJECT_OT_AddCapsule(bpy.types.Operator):
	"""Add a Capsule mesh"""
	bl_idname = "mesh.primitive_capsule_add"
	bl_label = "Add a capsule"
	bl_options = {'REGISTER', 'UNDO'}
	segments = IntProperty(
			name="Capsule Segments",
			description="The number of segments",
			default=16,
			min=4,
			max=500)
	rings = IntProperty(
			name="Cap Rings",
			description="The number of rings on the cap",
			default=9,
			min=1,
			max=500)
	radius = FloatProperty(
			name="Radius",
			description="The radius of the capsule",
			default=1.0,
			min=0.01,
			max=100.0,
			unit="LENGTH")
	length = FloatProperty(
			name="Length",
			description="The length of the capsule body",
			default=3.0,
			min=0,
			max=100)
			
	def execute(self, context):
		
		segments = self.segments
		rings = self.rings
		radius = self.radius
		length = self.length
		
		verts = []
		faces = []
		
		verts.append(Vector((0, 0, (radius + (length/2)))))
		for r in range(1, rings + 1):
			ra = (90 / rings) * r
			rradius = sin(radians(ra)) * radius
			z = cos(radians(ra)) * radius
			for s in range(0, segments):
				a = (360 / segments) * s
				x = sin(radians(a)) * rradius
				y = cos(radians(a)) * rradius
				verts.append(Vector((x, y, z + (length/2))))
		for r in range(rings, rings*2):
			ra = (90 / rings) * r
			rradius = sin(radians(ra)) * radius
			z = cos(radians(ra)) * radius
			for s in range(0, segments):
				a = (360 / segments) * s
				x = sin(radians(a)) * rradius
				y = cos(radians(a)) * rradius
				verts.append(Vector((x, y, z - (length/2))))
		verts.append(Vector((0, 0, -(radius + (length/2)))))
		
		for r in range(2, (rings * 2) + 1):
			l = (r * segments) - segments
			x = True
			for v in range(l, r * segments):
				if x:
					faces.append([v + 1, v+(segments - 1) + 1, v-segments+(segments - 1) + 1, v-segments + 1])
					x = False
				if not v % segments == 0:
					#faces.append([v, v+1, v - segments+1, v - segments])
					faces.append([v + 1, v, v - segments, (v - segments)+1])
		
		
		#caps
		for v in range(1, segments):
			faces.append([v+1, v, 0])
		faces.append([0, 1, segments])
		for v in range(len(verts) - (segments + 1), len(verts) - 1):
			faces.append([len(verts) - 1, v, v+1])
		faces.append([len(verts) - 1, len(verts) - 2, len(verts) - (segments+1)])
		
		base = create_mesh_object(context, verts, [], faces, "Capsule")
		
		return {'FINISHED'}
	
class INFO_MT_mesh_capsule_add(bpy.types.Menu):
	
	bl_idname = "INFO_MT_mesh_capsule_add"
	bl_label = "Capsule"
	
	def draw(self, context):
		layout = self.layout
		layout.operator_context = "INVOKE_REGION_WIN"
		layout.operator("mesh.primitive_capsule_add",
						text="Capsule")
						
def add_object_button(self, context):
	self.layout.operator(
		OBJECT_OT_AddCapsule.bl_idname,
		text="Capsule",
		icon='PLUGIN')

def register():
	bpy.utils.register_module(__name__)
	bpy.types.INFO_MT_mesh_add.append(add_object_button)
	
def unregister():
	bpy.utils.unregister_module(__name__)
	bpy.types.INFO_MT_mesh_add.remove(add_object_button)
	
	
if __name__ == "__main__":
	register()

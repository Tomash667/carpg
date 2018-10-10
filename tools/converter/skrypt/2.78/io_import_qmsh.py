bl_info = {
    "name": "Qmsh",
    "author": "Tomashu",
    "version": (0, 20, 0),
    "blender": (2, 7, 8),
    "location": "File > Import > Qmsh",
    "description": "Import from Qmsh",
    "warning": "",
    "category": "Import-Export"}

import bpy
from bpy.props import StringProperty, BoolProperty
from math import pi
import subprocess
from mathutils import Vector, Quaternion, Matrix
import os
import struct
import bmesh
from random import random

################################################################################
def IsSet(flags, bit):
	return (flags & bit) != 0

################################################################################
class ImporterException(Exception):
	pass
		
################################################################################
class Box:
	def __init__(self, min, max):
		self.v1 = min
		self.v2 = max

################################################################################
class Vertex:
	def Read(self, f, type):
		self.pos = f.ReadVec3()
		if type == 'ANI' or type == 'TANG_ANI':
			self.weights = f.ReadFloat()
			self.indices = f.ReadUint()
		if type != 'POS':
			self.normal = f.ReadVec3()
			self.tex = f.ReadVec2()
		if type == 'TANG' or type == 'TANG_ANI':
			self.tangent = f.ReadVec3()
			self.binormal = f.ReadVec3()
		
################################################################################
class FileReader:
	def __init__(self, filepath):
		if not os.path.isfile(filepath):
			raise ImporterException("File not exists.")
		self.f = open(filepath, "rb")
	def __enter__(self):
		return self
	def __exit__(self, exc_type, exc_value, traceback):
		self.f.close()
	def Read(self, count):
		r = self.f.read(count)
		if len(r) != count:
			raise ImporterException("Unexpected end of file.")
		return r
	def ReadEOF(self):
		r = self.f.read(1)
		if len(r) != 0:
			raise ImportException("End of file expected.")
	def ReadByte(self):
		return struct.unpack('B', self.Read(1))[0]
	def ReadWord(self):
		return struct.unpack('H', self.Read(2))[0]
	def ReadInt(self):
		return struct.unpack('i', self.Read(4))[0]
	def ReadUint(self):
		return struct.unpack('I', self.Read(4))[0]
	def ReadFloat(self):
		return struct.unpack('f', self.Read(4))[0]
	def ReadVec2(self):
		return Vector((self.ReadFloat(), self.ReadFloat()))
	def ReadVec3(self):
		return Vector((self.ReadFloat(), self.ReadFloat(), self.ReadFloat()))
	def ReadQuaternion(self):
		return Quaternion((self.ReadFloat(), self.ReadFloat(), self.ReadFloat(), self.ReadFloat()))
	def ReadBox(self):
		return Box(self.ReadVec3(), self.ReadVec3())
	def ReadMatrix33(self):
		return Matrix(((self.ReadFloat(), self.ReadFloat(), self.ReadFloat(), 0), \
			(self.ReadFloat(), self.ReadFloat(), self.ReadFloat(), 0), \
			(self.ReadFloat(), self.ReadFloat(), self.ReadFloat(), 0), \
			(self.ReadFloat(), self.ReadFloat(), self.ReadFloat(), 1)))
	def ReadMatrix(self):
		return Matrix(((self.ReadFloat(), self.ReadFloat(), self.ReadFloat(), self.ReadFloat()), \
			(self.ReadFloat(), self.ReadFloat(), self.ReadFloat(), self.ReadFloat()), \
			(self.ReadFloat(), self.ReadFloat(), self.ReadFloat(), self.ReadFloat()), \
			(self.ReadFloat(), self.ReadFloat(), self.ReadFloat(), self.ReadFloat())))
	def ReadString(self):
		len = self.ReadByte()
		s = self.Read(len)
		return s.decode()
		
################################################################################
class QmshHeader:
	def IsTangents(self):
		return IsSet(self.flags, 1)
	def IsAnimated(self):
		return IsSet(self.flags, 2)
	def IsStatic(self):
		return IsSet(self.flags, 4)
	def IsPhysics(self):
		return IsSet(self.flags, 8)
	def IsSplit(self):
		return IsSet(self.flags, 16)
	def Read(self, f):
		self.sign = f.Read(4)
		self.version = f.ReadByte()
		self.flags = f.ReadByte()
		self.n_verts = f.ReadWord()
		self.n_tris = f.ReadWord()
		self.n_subs = f.ReadWord()
		self.n_bones = f.ReadWord()
		self.n_anims = f.ReadWord()
		self.n_points = f.ReadWord()
		self.n_groups = f.ReadWord()
		self.radius = f.ReadFloat()
		self.bbox = f.ReadBox()
		self.points_offset = f.ReadUint()
		self.cam_pos = f.ReadVec3()
		self.cam_target = f.ReadVec3()
		self.cam_up = f.ReadVec3()
		if self.sign != b"QMSH":
			raise ImporterException("Invalid file signature " + str(self.sign));
		if self.version != 20:
			raise ImporterException("Invalid file version " + str(self.version))
		if self.n_bones >= 32:
			raise ImporterException("Too many bones (" + str(self.n_bones) + ")")
		if self.n_subs == 0:
			raise ImporterException("Missing model mesh.")
		if self.IsAnimated() and not self.IsStatic():
			if self.n_bones == 0:
				raise ImporterException("No bones.")
			if self.n_groups == 0:
				raise ImporterException("No bone groups.")
		if self.IsSplit():
			raise ImporterException("Split mesh not implemented.")
	def SetVertexSize(self):
		self.vertex_size = 3 * 4
		self.vertex_type = 'POS'
		if not self.IsPhysics():
			if self.IsAnimated():
				if self.IsTangents():
					self.vertex_size = 16 * 4
					self.vertex_type = 'TANG_ANI'
				else:
					self.vertex_size = 10 * 4
					self.vertex_type = 'ANI'
			else:
				if self.IsTangents():
					self.vertex_size = 14 * 4
					self.vertex_type = 'TANG'
				else:
					self.vertex_size = 8 * 4
					self.vertex_type = 'NORMAL'
					
################################################################################
class QmshSubmesh:
	def Read(self, f, head):
		self.first = f.ReadWord()
		self.tris = f.ReadWord()
		self.min_ind = f.ReadWord()
		self.n_ind = f.ReadWord()
		self.name = f.ReadString()
		self.tex = f.ReadString()
		self.specular_color = f.ReadVec3()
		self.specular_intensity = f.ReadFloat()
		self.specular_hardness = f.ReadInt()
		if head.IsTangents():
			self.tex_normal = f.ReadString()
			self.normal_factor = f.ReadFloat()
		else:
			self.tex_normal = ''
		self.tex_specular = f.ReadString()
		if self.tex_specular != '':
			self.specular_factor = f.ReadFloat()
			self.specular_color_factor = f.ReadFloat()
			
################################################################################
class QmshBone:
	def Read(self, f, bones):
		self.childs = []
		self.parent = f.ReadWord()
		self.mat = f.ReadMatrix33()
		self.name = f.ReadString()
		bones[self.parent].childs.append(self)
		
################################################################################
class QmshBoneGroup:
	def Read(self, f):
		self.name = f.ReadString()
		self.parent = f.ReadWord()
		self.bones = []
		count = f.ReadByte()
		for i in range(count):
			self.bones.append(f.ReadByte())
		
################################################################################
class QmshAnimation:
	class Keyframe:
		class KeyframeBone:
			pass
	def Read(self, f, head):
		self.name = f.ReadString()
		self.length = f.ReadFloat()
		self.n_frames = f.ReadWord()
		self.frames = []
		for i in range(self.n_frames):
			frame = QmshAnimation.Keyframe()
			frame.time = f.ReadFloat()
			frame.bones = []
			for j in range(head.n_bones):
				bone = QmshAnimation.Keyframe.KeyframeBone()
				bone.pos = f.ReadVec3()
				bone.rot = f.ReadQuaternion()
				bone.scale = f.ReadFloat()
				frame.bones.append(bone)
			self.frames.append(frame)
			
################################################################################
class QmshPoint:
	def Read(self, f):
		self.name = f.ReadString()
		self.mat = f.ReadMatrix()
		self.bone = f.ReadWord()
		self.type = f.ReadWord()
		self.size = f.ReadVec3()
		self.rot = f.ReadVec3()

################################################################################
class Qmsh:
	def Read(self, f):
		self.ReadHeader(f)
		self.ReadVertices(f)
		self.ReadTriangles(f)
		self.ReadSubmeshes(f)
		if self.head.IsAnimated() and not self.head.IsStatic():
			self.ReadBones(f)
			self.ReadAnimations(f)
			self.head.n_bones += 1 # add zero bone
		self.ReadPoints(f)
		if self.head.IsAnimated() and not self.head.IsStatic():
			self.ReadBoneGroups(f)
		f.ReadEOF()
	def ReadHeader(self, f):
		self.head = QmshHeader()
		self.head.Read(f)
		self.head.SetVertexSize()
	def ReadVertices(self, f):
		self.verts = []
		for i in range(self.head.n_verts):
			v = Vertex()
			v.Read(f, self.head.vertex_type)
			self.verts.append(v)
	def ReadTriangles(self, f):
		self.tris = []
		for i in range(self.head.n_tris):
			tri = [f.ReadWord(), f.ReadWord(), f.ReadWord()]
			self.tris.append(tri)
	def ReadSubmeshes(self, f):
		self.subs = []
		for i in range(self.head.n_subs):
			sub = QmshSubmesh()
			sub.Read(f, self.head)
			self.subs.append(sub)
	def ReadBones(self, f):
		self.bones = []
		zero_bone = QmshBone()
		zero_bone.parent = 0
		zero_bone.name = "zero"
		zero_bone.id = 0
		zero_bone.mat = Matrix.Identity(4)
		zero_bone.childs = []
		self.bones.append(zero_bone)
		for i in range(1, self.head.n_bones+1):
			bone = QmshBone()
			bone.id = i
			bone.Read(f, self.bones)
			self.bones.append(bone)
	def ReadAnimations(self, f):
		self.anims = []
		for i in range(self.head.n_anims):
			anim = QmshAnimation()
			anim.Read(f, self.head)
			self.anims.append(anim)
	def ReadPoints(self, f):
		self.points = []
		for i in range(self.head.n_points):
			point = QmshPoint()
			point.Read(f)
			self.points.append(point)
	def ReadBoneGroups(self, f):
		self.groups = []
		for i in range(self.head.n_groups):
			group = QmshBoneGroup()
			group.Read(f)
			self.groups.append(group)

################################################################################
def Import(filepath):
	print("Loading file "+filepath+".")
	with FileReader(filepath) as f:
		mesh = Qmsh()
		mesh.Read(f)
	mesh_data = bpy.data.meshes.new(name='Mesh')
	bm = bmesh.new()
	# create vertices
	new_verts = []
	for v in mesh.verts:
		new_verts.append(bm.verts.new(v.pos))
	# create faces
	new_faces = []
	for f in mesh.tris:
		new_faces.append(bm.faces.new((new_verts[f[0]], new_verts[f[1]], new_verts[f[2]])))
	bm.faces.ensure_lookup_table()
	index = 0
	for sub in mesh.subs:
		# add material
		mat = bpy.data.materials.new(sub.name)
		mat.diffuse_color = (random(), random(), random())
		mat.specular_color = sub.specular_color
		mat.specular_hardness = sub.specular_hardness
		mat.specular_intensity = sub.specular_intensity
		mesh_data.materials.append(mat)
		# add texture
		tex = bpy.data.textures.new('', 'IMAGE')
		tex_slot = mat.texture_slots.add()
		tex_slot.texture = tex
		# add image
		img = bpy.data.images.new(sub.tex, 1, 1)
		img.filepath = sub.tex
		img.source = 'FILE'
		tex.image = img
		# set material index
		for i in range(sub.first, sub.first+sub.tris):
			bm.faces[i].material_index = index
		index += 1
	bm.to_mesh(mesh_data)
	obj = bpy.data.objects.new(name='Obj', object_data=mesh_data)
	bpy.context.scene.objects.link(obj)
	print("Import finished.");
	
################################################################################
# Klasa importera
class QmshImporter(bpy.types.Operator):
	"""Import from Qmsh format (.qmsh)"""
	
	bl_idname = "import.qmsh"
	bl_label = "Import QMSH"
	
	filepath = StringProperty(subtype='FILE_PATH')
		
	def export_convert(self):
		return Import(self.filepath)
		
	def execute(self, context):
		try:
			self.export_convert()
		except ImporterException as error:
			msg = 'Exporter error: '+str(error)
			print(msg)
			bpy.ops.error.message('INVOKE_DEFAULT', message = msg)
		return {"FINISHED"}
	
	def invoke(self, context, event):
		mode = bpy.context.mode
		if mode == "EDIT_MESH" or mode == "EDIT_ARMATURE":
			mode = "OBJECT"
		if not self.filepath:
			self.filepath = bpy.path.ensure_ext(os.path.splitext(bpy.data.filepath)[0], ".qmsh")
		WindowManager = context.window_manager
		WindowManager.fileselect_add(self)
		return {"RUNNING_MODAL"}

################################################################################
# funkcje pluginu
def menu_func(self, context):
    self.layout.operator(QmshImporter.bl_idname, text="Qmsh (.qmsh)")

def register():
    bpy.utils.register_module(__name__)
    bpy.types.INFO_MT_file_import.append(menu_func)

def unregister():
    bpy.utils.unregister_module(__name__)
    bpy.types.INFO_MT_file_import.remove(menu_func)

if __name__ == "__main__":
	register()

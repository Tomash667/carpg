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
import configparser
from bpy_extras.io_utils import ImportHelper

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
def ConvertVec3(v):
	return (v[0], v[2], v[1])
	
################################################################################
def ConvertMatrix(m):
	o = Matrix()
	o[0][0] = m[0][0];
	o[0][3] = m[0][3];
	o[3][0] = m[3][0];
	o[3][3] = m[3][3];
	o[0][1] = m[0][2];
	o[0][2] = m[0][1];
	o[3][2] = m[3][1];
	o[3][1] = m[3][2];
	o[2][0] = m[1][0];
	o[1][0] = m[2][0];
	o[1][3] = m[2][3];
	o[2][3] = m[1][3];
	o[1][1] = m[2][2];
	o[2][2] = m[1][1];
	o[1][2] = m[2][1];
	o[2][1] = m[1][2];
	print(o)
	return o

################################################################################
class Vertex:
	def Read(self, f, type):
		self.pos = ConvertVec3(f.ReadVec3())
		if type == 'ANI' or type == 'TANG_ANI':
			self.weights = f.ReadFloat()
			self.indices = f.ReadUint()
		if type != 'POS':
			self.normal = f.ReadVec3()
			self.tex = f.ReadVec2()
			self.tex[1] = 1.0 - self.tex[1]
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
			raise ImporterException("End of file expected.")
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
			if self.tex_normal != '':
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
		self.mat = ConvertMatrix(f.ReadMatrix())
		self.bone = f.ReadWord()
		self.type = f.ReadWord()
		self.size = ConvertVec3(f.ReadVec3())
		self.rot = ConvertVec3(f.ReadVec3())

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
		self.broken_tris = []
		for i in range(self.head.n_tris):
			tri = [f.ReadWord(), f.ReadWord(), f.ReadWord()]
			tri = (tri[0], tri[2], tri[1]) # convert to blender order
			if tri[0] == tri[1] or tri[0] == tri[2] or tri[1] == tri[2]:
				self.broken_tris.append(i)
			self.tris.append(tri)
		# try to fix broken faces, will be removed later
		if len(self.broken_tris) > 0:
			index = 2
			for i in self.broken_tris:
				face = self.GetFreeFace(index)
				index = face[2] + 1
				self.tris[i] = face
						
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
	def IsFaceUsed(self, face):
		face = sorted(face)
		for tri in self.tris:
			if face == sorted(tri):
				return True
		return False
	def GetFreeFace(self, index):
		face = (0,1,index)
		while True:
			if self.IsFaceUsed(face):
				face = (face[0], face[1], face[2]+1)
			else:
				return face
			
################################################################################
class Config:
	def __init__(self):
		config_path = bpy.utils.user_resource('CONFIG', path='scripts', create=True)
		self.filepath = os.path.join(config_path, 'io_import_qmsh.cfg')
		self.config = configparser.ConfigParser()
		self.config.read(self.filepath)
		if not self.config.has_section('settings'):
			self.config['settings'] = {'loadImages': 'True',
				'imagesPath': 'D:\\carpg\\bin\\data\\textures'}
		s = self.config['settings']
		self.loadImages = s.getboolean('loadImages')
		self.imagesPath = s['imagesPath']
	def Save(self):
		s = self.config['settings']
		s['loadImages'] = str(self.loadImages)
		s['imagesPath'] = self.imagesPath
		with open(self.filepath, 'w') as configfile:
			self.config.write(configfile)

################################################################################
def FindImage(filename, imagesPath):
	for root, dirs, files in os.walk(imagesPath):
		for file in files:
			if file == filename:
				return os.path.join(root, file)
	return None

################################################################################
def LoadImage(filename, config):
	if config.loadImages:
		filepath = FindImage(filename, config.imagesPath)
		if not filepath is None:
			img = bpy.data.images.load(filepath)
			return img
	img = bpy.data.images.new(sub.tex, 1, 1)
	img.filepath = sub.tex
	img.source = 'FILE'
	return img

################################################################################
def Import(filepath, config):
	print("INFO: Loading file "+filepath+".")
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
	for f in mesh.tris:
		bm.faces.new((new_verts[f[0]], new_verts[f[1]], new_verts[f[2]]))
	bm.faces.ensure_lookup_table()
	index = 0
	for sub in mesh.subs:
		# add material
		mat = bpy.data.materials.new(sub.name)
		mat.use_shadeless = True
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
		img = LoadImage(sub.tex, config)
		tex.image = img
		sub.image = img
		# set material index
		for i in range(sub.first, sub.first+sub.tris):
			bm.faces[i].material_index = index
		index += 1
	bm.to_mesh(mesh_data)
	obj = bpy.data.objects.new(name='Obj', object_data=mesh_data)
	bpy.context.scene.objects.link(obj)
	# uv map
	uv_map = mesh_data.uv_textures.new()
	uv_data = uv_map.data
	uvs = mesh_data.uv_layers[0].data
	for sub in mesh.subs:
		for i in range(sub.first, sub.first+sub.tris):
			uv_data[i].image = sub.image
	for i in range(mesh.head.n_tris):
		f = mesh.tris[i]
		uvs[i*3+0].uv = mesh.verts[f[0]].tex
		uvs[i*3+1].uv = mesh.verts[f[1]].tex
		uvs[i*3+2].uv = mesh.verts[f[2]].tex
	# remove broken tris
	if len(mesh.broken_tris) > 0:
		bm.clear()
		bm.from_mesh(mesh_data)
		to_delete = []
		bm.faces.ensure_lookup_table()
		for i in mesh.broken_tris:
			to_delete.append(bm.faces[i])
		bmesh.ops.delete(bm, geom=to_delete, context=5)  
		bm.to_mesh(mesh_data)
		mesh_data.update()
		print("WARN: Removed %d broken tris." % len(mesh.broken_tris))
	# armature
	if mesh.head.IsAnimated() and not mesh.head.IsStatic():
		armature = bpy.data.armatures.new(name='Armature')
		obj_arm = bpy.data.objects.new(name='Armature', object_data=armature)
		bpy.context.scene.objects.link(obj_arm)
		bpy.context.scene.objects.active = obj_arm
		bpy.ops.object.mode_set(mode='EDIT')
		for bone in mesh.bones:
			if bone.name == "zero":
				continue
			b = armature.edit_bones.new(bone.name)
			b.head = (1.0, 1.0, 0.0)
			b.tail = (1.0, 1.0, 1.0)
		bpy.ops.object.mode_set(mode='OBJECT')
	# add points
	for point in mesh.points:
		empty = bpy.data.objects.new(name=point.name, object_data=None)
		if empty.type == 0:
			empty.empty_draw_type = 'ARROWS'
		elif empty.type == 1:
			empty.empty_draw_type = 'SPHERE'
		else:
			empty.empty_draw_type = 'CUBE'
		empty.matrix_world = point.mat
		empty.scale = point.size
		empty.rotation_mode = 'QUATERNION'
		empty.rotation_euler = point.rot
		bpy.context.scene.objects.link(empty)
	# remove doubled vertices
	bm.clear()
	bm.from_mesh(mesh_data)
	bmesh.ops.remove_doubles(bm, verts=bm.verts, dist=0.00001)
	bm.to_mesh(mesh_data)
	mesh_data.update()
	bm.free()
	# tris to quads
	bpy.context.scene.objects.active = obj
	bpy.ops.object.editmode_toggle()
	bpy.ops.mesh.select_all()
	bpy.ops.mesh.tris_convert_to_quads()
	bpy.ops.mesh.select_all()
	bpy.ops.mesh.tris_convert_to_quads()
	bpy.ops.object.editmode_toggle()
	# switch to textured mode
	area = next(area for area in bpy.context.screen.areas if area.type == 'VIEW_3D')
	space = next(space for space in area.spaces if space.type == 'VIEW_3D')
	space.viewport_shade = 'TEXTURED'
	print("INFO: Import finished.");
	
################################################################################
# Klasa importera
class QmshImporter(bpy.types.Operator, ImportHelper):
	"""Import from Qmsh format (.qmsh)"""
	
	bl_idname = "import.qmsh"
	bl_label = "Import QMSH"
	
	config = Config()
	
	loadImages = BoolProperty(
		name="LoadImages",
		default=config.loadImages)
		
	imagesPath = StringProperty(
		name="Converter path",
		default=config.imagesPath)
	
	filter_glob = StringProperty(default="*.qmsh", options={'HIDDEN'})
	
	def execute(self, context):
		self.config.loadImages = self.loadImages
		self.config.imagesPath = self.imagesPath
		self.config.Save()
		try:
			Import(self.properties.filepath, self.config)
		except ImporterException as error:
			msg = 'Exporter error: '+str(error)
			print("ERROR: "+msg)
			bpy.ops.error.message('INVOKE_DEFAULT', message = msg)
		return {"FINISHED"}

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

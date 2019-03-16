bl_info = {
    "name": "Qmsh",
    "author": "Tomashu",
    "version": (0, 21, 0),
    "blender": (2, 7, 8),
    "location": "File > Import > Qmsh",
    "description": "Import from Qmsh",
    "warning": "",
    "category": "Import-Export"}

import bpy
from bpy.props import StringProperty, BoolProperty
from math import sqrt
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
def QuaternionLookRotation(fro, to, up):
	vector = (fro - to).normalized()
	vector2 = up.normalized().cross(vector).normalized()
	vector3 = vector.cross(vector2)
	m00 = vector2.x
	m01 = vector2.y
	m02 = vector2.z
	m10 = vector3.x
	m11 = vector3.y
	m12 = vector3.z
	m20 = vector.x
	m21 = vector.y
	m22 = vector.z

	num8 = (m00 + m11) + m22
	q = Quaternion()
	if num8 > 0:
		num = sqrt(num8 + 1)
		q.w = num * 0.5
		num = 0.5 / num
		q.x = (m12 - m21) * num
		q.y = (m20 - m02) * num
		q.z = (m01 - m10) * num
	elif m00 >= m11 and m00 >= m22:
		num7 = sqrt(((1 + m00) - m11) - m22)
		num4 = 0.5 / num7
		q.x = 0.5 * num7
		q.y = (m01 + m10) * num4
		q.z = (m02 + m20) * num4
		q.w = (m12 - m21) * num4
	elif m11 > m22:
		num6 = sqrt(((1 + m11) - m00) - m22)
		num3 = 0.5 / num6
		q.x = (m10+ m01) * num3
		q.y = 0.5 * num6
		q.z = (m21 + m12) * num3
		q.w = (m20 - m02) * num3
	else:
		num5 = sqrt(((1 + m22) - m00) - m11)
		num2 = 0.5 / num5
		q.x = (m20 + m02) * num2
		q.y = (m21 + m12) * num2
		q.z = 0.5 * num5
		q.w = (m01 - m10) * num2
	return q

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
	return Vector((v[0], v[2], v[1]))
	
################################################################################
def ConvertMatrix(m):
	o = Matrix()
	o[0][0] = m[0][0]
	o[0][3] = m[0][3]
	o[3][0] = m[3][0]
	o[3][3] = m[3][3]
	o[0][1] = m[0][2]
	o[0][2] = m[0][1]
	o[3][2] = m[3][1]
	o[3][1] = m[3][2]
	o[2][0] = m[1][0]
	o[1][0] = m[2][0]
	o[1][3] = m[2][3]
	o[2][3] = m[1][3]
	o[1][1] = m[2][2]
	o[2][2] = m[1][1]
	o[1][2] = m[2][1]
	o[2][1] = m[1][2]
	return o

################################################################################
class Vertex:
	def Read(self, f, type):
		self.pos = ConvertVec3(f.ReadVec3())
		if type == 'ANI' or type == 'TANG_ANI':
			self.weights = f.ReadFloat()
			indices = f.ReadUint()
			self.indices = (indices & 0xFF, (indices & 0xFF00) >> 8)
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
	def ReadBool(self):
		return struct.unpack('B', self.Read(1))[0] != 0
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
class Qmsh:
	##-------------------------------------------------------------------------
	class Header:
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
			self.cam_pos = ConvertVec3(f.ReadVec3())
			self.cam_target = ConvertVec3(f.ReadVec3())
			self.cam_up = ConvertVec3(f.ReadVec3())
			if self.sign != b"QMSH":
				raise ImporterException("Invalid file signature " + str(self.sign))
			if self.version < 20 or self.version > 21:
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
	##-------------------------------------------------------------------------
	class Submesh:
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
	##-------------------------------------------------------------------------
	class Bone:
		def Read(self, f, bones, version):
			self.childs = []
			if version >= 21:
				self.name = f.ReadString()
				self.parent = f.ReadWord()
				self.mat = f.ReadMatrix33()
				self.raw_mat = f.ReadMatrix().transposed()
				self.head = f.ReadVec3()
				self.head_radius = f.ReadFloat()
				self.tail = f.ReadVec3()
				self.tail_radius = f.ReadFloat()
				self.connected = f.ReadBool()
			else:
				self.parent = f.ReadWord()
				self.mat = f.ReadMatrix33()
				self.raw_mat = self.mat
				self.name = f.ReadString()
				self.head = (1.0, 1.0, 0.0)
				self.head_radius = 0.1
				self.tail = (1.0, 1.0, 1.0)
				self.tail_radius = 0.1
				self.connected = False
			bones[self.parent].childs.append(self)
	##-------------------------------------------------------------------------
	class BoneGroup:
		def Read(self, f):
			self.name = f.ReadString()
			self.parent = f.ReadWord()
			self.bones = []
			count = f.ReadByte()
			for i in range(count):
				self.bones.append(f.ReadByte())
	##-------------------------------------------------------------------------
	class Animation:
		class Keyframe:
			class KeyframeBone:
				pass
		def Read(self, f, head):
			self.name = f.ReadString()
			self.length = f.ReadFloat()
			self.n_frames = f.ReadWord()
			self.frames = []
			for i in range(self.n_frames):
				frame = Qmsh.Animation.Keyframe()
				frame.time = f.ReadFloat()
				frame.bones = []
				for j in range(head.n_bones):
					bone = Qmsh.Animation.Keyframe.KeyframeBone()
					bone.pos = ConvertVec3(f.ReadVec3())
					bone.rot = f.ReadQuaternion()
					bone.scale = f.ReadFloat()
					frame.bones.append(bone)
				self.frames.append(frame)
		def UseScale(self, bone_i):
			for frame in self.frames:
				if frame.bones[bone_i].scale != 1:
					return True
			return False
	##-------------------------------------------------------------------------
	class Point:
		def Read(self, f):
			self.name = f.ReadString()
			self.mat = ConvertMatrix(f.ReadMatrix()).transposed()
			self.bone = f.ReadWord()
			empty_types = {
				0: 'PLAIN_AXES',
				1: 'SPHERE',
				2: 'CUBE',
				3: 'ARROWS',
				4: 'SINGLE_ARROW',
				5: 'CIRCLE',
				6: 'CONE' }
			self.type = empty_types.get(f.ReadWord(), 'PLAIN_AXES')
			self.size = ConvertVec3(f.ReadVec3())
			self.rot = ConvertVec3(f.ReadVec3())
	##-------------------------------------------------------------------------
	def Read(self, f):
		self.ReadHeader(f)
		self.ReadVertices(f)
		self.ReadTriangles(f)
		self.ReadSubmeshes(f)
		if self.head.IsAnimated() and not self.head.IsStatic():
			self.ReadBones(f)
			if self.head.version >= 21:
				self.ReadBoneGroups(f)
			self.ReadAnimations(f)
			self.head.n_bones += 1 # add zero bone
		else:
			self.bones = []
			self.groups = []
		self.ReadPoints(f)
		if self.head.version < 21 and self.head.IsAnimated() and not self.head.IsStatic():
			self.ReadBoneGroups(f)
		f.ReadEOF()
	def ReadHeader(self, f):
		self.head = Qmsh.Header()
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
			if tri[0] == tri[1] or tri[0] == tri[2] or tri[1] == tri[2] or self.CheckTriangleDuplicate(tri):
				self.broken_tris.append(i)
			self.tris.append(tri)
		# try to fix broken faces, will be removed later
		if len(self.broken_tris) > 0:
			index = 2
			for i in self.broken_tris:
				face = self.GetFreeFace(index)
				index = face[2] + 1
				self.tris[i] = face
	def CheckTriangleDuplicate(self, tri):
		tri = sorted(tri)
		for tri2 in self.tris:
			if sorted(tri2) == tri:
				return True
		return False
	def ReadSubmeshes(self, f):
		self.subs = []
		for i in range(self.head.n_subs):
			sub = Qmsh.Submesh()
			sub.Read(f, self.head)
			self.subs.append(sub)
	def ReadBones(self, f):
		self.bones = []
		zero_bone = Qmsh.Bone()
		zero_bone.parent = 0
		zero_bone.name = "zero"
		zero_bone.id = 0
		zero_bone.mat = Matrix.Identity(4)
		zero_bone.childs = []
		self.bones.append(zero_bone)
		for i in range(1, self.head.n_bones+1):
			bone = Qmsh.Bone()
			bone.id = i
			bone.Read(f, self.bones, self.head.version)
			self.bones.append(bone)
	def ReadAnimations(self, f):
		self.anims = []
		for i in range(self.head.n_anims):
			anim = Qmsh.Animation()
			anim.Read(f, self.head)
			self.anims.append(anim)
	def ReadPoints(self, f):
		self.points = []
		for i in range(self.head.n_points):
			point = Qmsh.Point()
			point.Read(f)
			self.points.append(point)
	def ReadBoneGroups(self, f):
		self.groups = []
		for i in range(self.head.n_groups):
			group = Qmsh.BoneGroup()
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
	def GetAnimation(self, name):
		for ani in self.anims:
			if ani.name == name:
				return ani
		return None
	def GetPoint(self, name):
		for point in self.points:
			if point.name == name:
				return point
		return None
			
################################################################################
class Config:
	def __init__(self):
		config_path = bpy.utils.user_resource('CONFIG', path='scripts', create=True)
		self.filepath = os.path.join(config_path, 'io_import_qmsh.cfg')
		self.config = configparser.ConfigParser()
		self.config.read(self.filepath)
		self.Init()
		s = self.config['settings']
		self.loadImages = s.getboolean('loadImages')
		self.imagesPath = s['imagesPath']
		self.setResolution = s.getboolean('setResolution')
	def Init(self):
		dict = {'loadImages': 'True',
				'imagesPath': 'D:\\carpg\\bin\\data\\textures',
				'setResolution': 'True'}
		if not self.config.has_section('settings'):
			self.config['settings'] = dict
			return
		s = self.config['settings']
		for k, v in dict.items():
			if not self.config.has_option('settings', k):
				s[k] = v
	def Save(self):
		s = self.config['settings']
		s['loadImages'] = str(self.loadImages)
		s['imagesPath'] = self.imagesPath
		s['setResolution'] = str(self.setResolution)
		with open(self.filepath, 'w') as configfile:
			self.config.write(configfile)

################################################################################
class Importer:
	def __init__(self):
		self.warnings = 0
		self.last_cam = None
		self.skeleton = None
		self.fps = bpy.context.scene.render.fps
	def Warn(self, msg):
		print('WARN: ' + msg)
		self.warnings += 1
	def Run(self, filepath, config):
		self.config = config
		self.LoadMesh(filepath)
		if config.useMerge:
			self.ParseMergeScript(config.mergeScript)
			self.ApplyMergeScript()
		else:
			self.Import()
		if self.config.setResolution:
			self.SetResolution()
	def LoadMesh(self, filepath):
		print("INFO: Loading file "+filepath+"...")
		with FileReader(filepath) as f:
			self.mesh = Qmsh()
			self.mesh.Read(f)
		print("INFO: Loaded mesh.")
	def FindImage(self, filename):
		for root, dirs, files in os.walk(self.config.imagesPath):
			for file in files:
				if file == filename:
					return os.path.join(root, file)
		return None
	def LoadImage(self, filename):
		if self.config.loadImages:
			filepath = self.FindImage(filename)
			if not filepath is None:
				img = bpy.data.images.load(filepath)
				return img
		img = bpy.data.images.new(sub.tex, 1, 1)
		img.filepath = sub.tex
		img.source = 'FILE'
		return img
	def Import(self):
		print("INFO: Importing mesh...")
		mesh = self.mesh
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
		# create list of materials
		materials = []
		sub_to_mat = []
		index = 0
		for sub in mesh.subs:
			existing_index = -1
			# search for already added material
			for i, mat in enumerate(materials):
				tex = mat[1]
				if tex == sub.tex:
					existing_index = i
					break
			# add new
			if existing_index == -1:
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
				img = self.LoadImage(sub.tex)
				tex.image = img
				sub.image = img
				# setup
				materials.append((mat, sub.tex, img))
				existing_index = index
				index += 1
			else:
				sub.image = materials[existing_index][2]
			sub_to_mat.append(existing_index)
		# set material index
		index = 0
		for sub in mesh.subs:
			mat_index = sub_to_mat[index]
			for i in range(sub.first, sub.first+sub.tris):
				bm.faces[i].material_index = mat_index
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
		# vertex groups
		if len(mesh.bones) > 1 and mesh.head.IsAnimated():
			for bone in mesh.bones:
				obj.vertex_groups.new(name=bone.name)
			i = 0
			for v in mesh.verts:
				obj.vertex_groups[v.indices[0]].add([i], v.weights, 'ADD')
				obj.vertex_groups[v.indices[1]].add([i], 1 - v.weights, 'ADD')
				i += 1
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
			self.Warn("Removed %d broken tris." % len(mesh.broken_tris))
		# armature
		if mesh.head.IsAnimated() and not mesh.head.IsStatic():
			# create skeleton
			armature = bpy.data.armatures.new(name='Armature')
			obj_arm = bpy.data.objects.new(name='Armature', object_data=armature)
			self.skeleton = obj_arm
			bpy.context.scene.objects.link(obj_arm)
			bpy.context.scene.objects.active = obj_arm
			# add bones
			bpy.ops.object.mode_set(mode='EDIT')
			for bone in mesh.bones:
				if bone.name == "zero":
					continue
				b = armature.edit_bones.new(bone.name)
				b.head = bone.head
				b.head_radius = bone.head_radius
				b.tail = bone.tail
				b.tail_radius = bone.tail_radius
				b.matrix = bone.raw_mat
				b.use_connect = bone.connected
				if bone.parent != 0:
					b.parent = armature.edit_bones[bone.parent - 1]
			bpy.ops.object.mode_set(mode='OBJECT')
			# add bone groups
			if len(self.mesh.groups) > 1:
				i = 0
				pose = obj_arm.pose
				for group in self.mesh.groups:
					name = group.name
					if group.parent != i:
						name += '#' + self.mesh.groups[group.parent].name
					bone_group = pose.bone_groups.new(name = name)
					for bone_i in group.bones:
						bone_name = self.mesh.bones[bone_i].name
						for bone in pose.bones:
							if bone.name == bone_name:
								bone.bone_group = bone_group
								break
					i += 1
			# animations
			for anim in mesh.anims:
				self.AddAnimation(anim)
			# add armature modifier
			bpy.context.scene.objects.active = obj
			bpy.ops.object.modifier_add(type='ARMATURE')
			mod = obj.modifiers['Armature']
			mod.object = obj_arm
			mod.use_vertex_groups = True
		# add points
		for point in mesh.points:
			self.AddPoint(point)
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
		# camera
		self.AddCamera()
		print("INFO: Import finished.")
	def ParseMergeScript(self, mergeScript):
		print("INFO: Parsing merge script...")
		script = []
		strs = mergeScript.split()
		i = 0
		count = len(strs)
		while True:
			if i >= count:
				break
			op = strs[i]
			if op != 'add' and op != 'replace':
				raise ImporterException('Invalid operation %s.' % op)
			i += 1
			if i >= count:
				raise ImporterException('Unexpected end of script.')
			type = strs[i]
			if type != 'animation' and type != 'point' and type != 'camera':
				raise ImporterException('Invalid type %s.' % type)
			obj = None
			obj2 = None
			if type != 'camera':
				i += 1
				if i >= count:
					raise ImporterException('Unexpected end of script.')
				name = strs[i]
				if type == 'animation':
					obj = self.mesh.GetAnimation(name)
					if obj is None:
						raise ImporterException('Missing animation %s.' % name)
					if op == 'replace':
						self.FindSkeleton()
						obj2 = self.FindAnimation(name)
						if obj2 is None:
							raise ImporterException('Missing animation to merge %s.' % name)
				else:
					obj = self.mesh.GetPoint(name)
					if obj is None:
						raise ImporterException('Missing point %s.' % name)
					if op == 'replace':
						obj2 = self.FindPoint(name)
						if obj2 is None:
							raise ImporterException('Missing point to merge %s.' % name)
			script.append((op, type, obj, obj2))
			i += 1
		self.script = script
		print("INFO: Finished with %d operations." % len(script))
	def FindSkeleton(self):
		if self.skeleton is not None:
			return
		self.skeleton = self.FindObject('ARMATURE')
		if self.skeleton is None:
			raise ImporterException('Missing skeleton.')
	def FindAnimation(self, name):
		for action in bpy.data.actions:
			if action.name == name:
				return action
		return None
	def FindPoint(self, name):
		return self.FindObject('EMPTY', name)
	def ApplyMergeScript(self):
		if len(self.script) == 0:
			print("INFO: Nothing to do.")
			return
		for cmd in self.script:
			op = cmd[0]
			type = cmd[1]
			obj = cmd[2]
			obj2 = cmd[3]
			if op == 'add':
				if type == 'animation':
					self.AddAnimation(obj)
				elif type == 'point':
					self.AddPoint(obj)
				elif type == 'camera':
					self.AddCamera()
			else:
				if type == 'animation':
					self.MergeAnimation(obj, obj2)
				elif type == 'point':
					self.MergePoint(obj, obj2)
				elif type == 'camera':
					self.MergeCamera()
	def AddAnimation(self, anim):
		class ActionHelper:
			def __init__(self, ani, bone_i, bone_name, use_scale):
				self.curves = {}
				self.ani = ani
				self.use_scale = use_scale
				self.group = ani.groups.new(name = bone_name)
				self.AddCurves('location', 3, bone_name)
				self.AddCurves('rotation_quaternion', 4, bone_name)
				if self.use_scale:
					self.AddCurves('scale', 3, bone_name)
			def AddCurves(self, name, count, bone_name):
				for i in range(count):
					curve = self.ani.fcurves.new(data_path='pose.bones["%s"].%s' % (bone_name, name), index=i)
					curve.group = self.group
					self.curves[name + str(i)] = curve
			def SetFrames(self, count):
				for name, curve in self.curves.items():
					curve.keyframe_points.add(count)
			def SetParams(self, i, time, framebone):
				self.curves['location0'].keyframe_points[i].co = (time, framebone.pos[0])
				self.curves['location1'].keyframe_points[i].co = (time, framebone.pos[1])
				self.curves['location2'].keyframe_points[i].co = (time, framebone.pos[2])
				self.curves['rotation_quaternion0'].keyframe_points[i].co = (time, -framebone.rot[3]) # W negated value
				self.curves['rotation_quaternion1'].keyframe_points[i].co = (time, framebone.rot[0]) # X
				self.curves['rotation_quaternion2'].keyframe_points[i].co = (time, framebone.rot[2]) # Y
				self.curves['rotation_quaternion3'].keyframe_points[i].co = (time, framebone.rot[1]) # Z
				if self.use_scale:
					self.curves['scale0'].keyframe_points[i].co = (time, framebone.scale)
					self.curves['scale1'].keyframe_points[i].co = (time, framebone.scale)
					self.curves['scale2'].keyframe_points[i].co = (time, framebone.scale)
			def Update(self):
				for name, curve in self.curves.items():
					curve.update()
		a = bpy.data.actions.new(anim.name)
		for bone_i in range(1, self.mesh.head.n_bones):
			bone = self.mesh.bones[bone_i]
			h = ActionHelper(a, bone_i, bone.name, anim.UseScale(bone_i - 1))
			h.SetFrames(anim.n_frames)
			for i in range(anim.n_frames):
				frame = anim.frames[i]
				framebone = frame.bones[bone_i - 1]
				h.SetParams(i, round(frame.time * self.fps), framebone)
			h.Update()
		a.use_fake_user = True
	def MergeAnimation(self, anim, existing):
		bpy.data.actions.remove(existing, do_unlink=True)
		self.AddAnimation(anim)
	def AddPoint(self, point):
		empty = bpy.data.objects.new(name=point.name, object_data=None)
		empty.empty_draw_type = point.type
		if point.bone <= self.mesh.head.n_bones:
			bone = self.mesh.bones[point.bone]
			empty.parent = self.skeleton
			empty.parent_type = 'BONE'
			empty.parent_bone = bone.name
		empty.matrix_world = point.mat
		empty.scale = point.size
		bpy.context.scene.objects.link(empty)
	def MergePoint(self, point, empty):
		bpy.ops.object.select_all(action='DESELECT')
		empty.select = True
		bpy.ops.object.delete()
		self.AddPoint(point)
	def AddCamera(self):
		cam = bpy.data.cameras.new(name='Camera')
		obj = bpy.data.objects.new(name='Camera', object_data=cam)
		self.SetCameraParams(obj)
		bpy.context.scene.objects.link(obj)
		self.last_cam = obj
	def MergeCamera(self):
		cam = FindObject('CAMERA')
		if cam is None:
			self.Warn('Missing camera to merge.')
		else:
			self.SetCameraParams(cam)
			self.last_cam = cam
	def SetCameraParams(self, cam):
		cam.location = self.mesh.head.cam_pos
		cam.rotation_mode = 'QUATERNION'
		cam.rotation_quaternion = QuaternionLookRotation(self.mesh.head.cam_pos, self.mesh.head.cam_target, self.mesh.head.cam_up)
	def FindObject(self, type, name = None):
		for obj in bpy.context.selected_objects:
			if obj.type == type and (name is None or obj.name == name):
				return obj
		for obj in bpy.context.scene.objects:
			if obj.type == type and (name is None or obj.name == name):
				return obj
		return None
	def SetResolution(self):
		scene = bpy.context.scene
		render = scene.render
		render.resolution_x = 256
		render.resolution_y = 256
		render.resolution_percentage = 100
		if self.last_cam is None:
			self.last_cam = self.FindObject('CAMERA')
		if self.last_cam is not None:
			scene.camera = self.last_cam
			for area in bpy.context.screen.areas:
				if area.type == 'VIEW_3D':
					area.spaces[0].region_3d.view_perspective = 'CAMERA'
		
################################################################################
# Klasa importera
class QmshImporterOperator(bpy.types.Operator, ImportHelper):
	"""Import from Qmsh format (.qmsh)"""
	
	bl_idname = "import.qmsh"
	bl_label = "Import QMSH"
	filter_glob = StringProperty(default="*.qmsh", options={'HIDDEN'})
	
	config = Config()
	
	loadImages = BoolProperty(name="Load images", default=config.loadImages)
	imagesPath = StringProperty(name="Converter path", default=config.imagesPath)
	setResolution = BoolProperty(name='Set resolution', default=config.setResolution)
	useMerge = BoolProperty(name="Use merge", default=False)
	mergeScript = StringProperty(name="Merge script")
	
	def ShowMessageBox(self, message, title, icon = 'INFO'):
		def draw(self, context):
			self.layout.label(message)
		bpy.context.window_manager.popup_menu(draw, title = title, icon = icon)
	
	def execute(self, context):
		self.config.loadImages = self.loadImages
		self.config.imagesPath = self.imagesPath
		self.config.setResolution = self.setResolution
		self.config.useMerge = self.useMerge
		self.config.mergeScript = self.mergeScript
		self.config.Save()
		importer = Importer()
		try:
			importer.Run(self.properties.filepath, self.config)
			if importer.warnings != 0:
				msg = 'Finished with %d warnings.' % importer.warnings
				print('WARN: ' + msg)
				self.ShowMessageBox(msg, 'Warning')
		except ImporterException as error:
			msg = 'Exporter error: ' +str(error)
			print("ERROR: " + msg)
			self.ShowMessageBox(msg, 'Error', 'ERROR')
		return {"FINISHED"}

################################################################################
# funkcje pluginu
def menu_func(self, context):
	self.layout.operator(QmshImporterOperator.bl_idname, text="Qmsh (.qmsh)")

def register():
	bpy.utils.register_module(__name__)
	bpy.types.INFO_MT_file_import.append(menu_func)

def unregister():
	bpy.utils.unregister_module(__name__)
	bpy.types.INFO_MT_file_import.remove(menu_func)

if __name__ == "__main__":
	register()

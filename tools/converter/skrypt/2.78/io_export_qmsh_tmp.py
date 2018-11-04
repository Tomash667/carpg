bl_info = {
    "name": "Qmsh.tmp",
    "author": "Tomashu",
    "version": (0, 21, 0),
    "blender": (2, 7, 8),
    "location": "File > Export > Qmsh.tmp",
    "description": "Export to Qmsh.tmp",
    "warning": "",
    "category": "Import-Export"}
    
# UWAGI
# - zaznacz tylko to co chcesz wyeksprtowac
# - eksportujac punkty (Empty) ustaw animacje na 'base'
# - eksportujac pancerz zaznacz tez szkielet i odpowiednia opcje
# - obiekty na roznych layerach sie nie eksportuja zazwyczaj bierze tylko z pierwszego
# - opcja Use the scene active camera na prawo od layerow sprawia ze nic nie jest zaznaczone

import bpy
from bpy.props import StringProperty, BoolProperty
from math import pi
import subprocess
import mathutils
import os
from bpy_extras.io_utils import ExportHelper
import configparser

################################################################################
class ExporterException(Exception):
	pass

################################################################################
class ExporterData:
	def __init__(self,path,export_tex,static_ani,apply_mod,force_tan):
		self.path = path
		self.export_tex = export_tex
		self.static_ani = static_ani
		self.apply_mod = apply_mod
		self.force_tan = force_tan
		self.warnings = 0
		self.armatures = 0
		self.actions = 0
		self.bones = 0
		self.faces = 0
		self.vertices = 0
		self.objects = 0
		self.file = 0
	def Warning(self,msg):
		self.warnings = self.warnings + 1
		print("Warning: " + msg)
	def Error(self,msg):
		raise ExporterException(msg)

################################################################################
def QuoteString(s):
	R = ""
	for ch in s:
		if ch == "\r":
			R = R + "\\r"
		elif ch == "\n":
			R = R + "\\n"
		elif (ch == "\"") or (ch == "\'") or (ch == "\\"):
			R = R + "\\" + ch
		elif ch == "\0":
			R = R + "\\0"
		elif ch == "\v":
			R = R + "\\v"
		elif ch == "\b":
			R = R + "\\b"
		elif ch == "\f":
			R = R + "\\f"
		elif ch == "\a":
			R = R + "\\a"
		elif ch == "\t":
			R = R + "\\t"
		else:
			R = R + ch
	return "\"" + R + "\""

################################################################################
def ProcessMeshObject(data,obj):
	mesh = obj.to_mesh(bpy.context.scene, data.apply_mod, 'PREVIEW', True)
	# aktualizuj statystyki
	data.objects = data.objects + 1
	data.vertices = data.vertices + len(mesh.vertices)
	data.faces = data.faces + len(mesh.tessfaces)
	# Poczatek
	data.file.write("\tmesh %s {\n" % QuoteString(obj.name))
	# pozycja, rotacja, skala
	v = obj.location
	data.file.write("\t\tpos %f,%f,%f\n" % (v[0], v[1], v[2]))
	v = obj.rotation_euler
	data.file.write("\t\trot %f,%f,%f\n" % (v[0], v[1], v[2]))
	v = obj.scale
	data.file.write("\t\tscale %f,%f,%f\n" % (v[0], v[1], v[2]))
	# Parent Armature i Parent Bone
	if obj.parent == None:
		sParent = ""
	else:
		if obj.parent.type != "ARMATURE":
			data.Warning("Object \"%s\" has a parent." % obj.name)
		sParent = obj.parent.name
	data.file.write("\t\tparent %s %s\n" % (QuoteString(sParent), QuoteString(obj.parent_bone)))
	
	# Materialy
	mats = obj.material_slots
	data.file.write("\t\tmaterials %i {\n" % len(mats))
	for mat2 in mats:
		mat = mat2.material
		if mat == None:
			data.file.write("\t\t\tmaterial %s { }\n" % QuoteString(""))
		else:
			data.file.write("\t\t\tmaterial %s {\n" % QuoteString(mat.name))
			data.file.write("\t\t\t\tdiffuse %f,%f,%f %f\n" % (mat.diffuse_color.r, mat.diffuse_color.g, mat.diffuse_color.b, mat.diffuse_intensity))
			data.file.write("\t\t\t\tspecular %f,%f,%f %f %d\n" % (mat.specular_color.r, mat.specular_color.g, mat.specular_color.b, mat.specular_intensity, mat.specular_hardness))
			if data.export_tex:
				for ts in mat.texture_slots:
					if ts:
						tex = ts.texture
						if tex.type == 'IMAGE':
							data.file.write("\t\t\t\ttexture %s {\n" % QuoteString(bpy.path.basename(tex.image.filepath)))
							if ts.use_map_color_diffuse:
								data.file.write("\t\t\t\t\tdiffuse %f\n" % ts.diffuse_color_factor)
							if ts.use_map_specular:
								data.file.write("\t\t\t\t\tspecular %f\n" % ts.specular_factor)
							if ts.use_map_color_spec:
								data.file.write("\t\t\t\t\tspecular_color %f\n" % ts.specular_color_factor)
							if ts.use_map_normal:
								data.file.write("\t\t\t\t\tnormal %f\n" % ts.normal_factor)
							data.file.write("\t\t\t\t}\n")
						else:
							data.Warning("Unsuported texture type '%s'!" % tex.type)
			data.file.write("\t\t\t}\n")
	data.file.write("\t\t}\n")
	
	# Wierzcholki
	data.file.write("\t\tvertices %i {\n" % len(mesh.vertices))
	for v in mesh.vertices:
		data.file.write("\t\t\t%f,%f,%f,%f,%f,%f\n" % (v.co[0], v.co[1], v.co[2], v.normal[0], v.normal[1], v.normal[2]))
	data.file.write("\t\t}\n")
	
	# scianki
	data.file.write("\t\tfaces %i {\n" % len(mesh.tessfaces))
	if len(mesh.tessface_uv_textures) == 0:
		for f in mesh.tessfaces:
			if f.use_smooth:
				sSmooth = "1"
			else:
				sSmooth = "0"
			data.file.write("\t\t\t%i %i %s" % (len(f.vertices), f.material_index, sSmooth))
			ii = 0
			for v in f.vertices:
				data.file.write(" %i 0,0" % (v))
			data.file.write(" %f,%f,%f\n" % (f.normal.x,f.normal.y,f.normal.z))
	else:
		uv = mesh.tessface_uv_textures[0].data
		for f in mesh.tessfaces:
			if f.use_smooth:
				sSmooth = "1"
			else:
				sSmooth = "0"
			data.file.write("\t\t\t%i %i %s" % (len(f.vertices), f.material_index, sSmooth))
			tuv = uv[f.index]
			ii = 0
			for v in f.vertices:
				data.file.write(" %i %f,%f" % (v, tuv.uv[ii][0], tuv.uv[ii][1]))
				ii = ii + 1
			data.file.write(" %f,%f,%f\n" % (f.normal.x,f.normal.y,f.normal.z))
	data.file.write("\t\t}\n")
	
	# Grupy wierzcholkow
	data.file.write("\t\tvertex_groups {\n")
	for group in obj.vertex_groups:
		data.file.write("\t\t\t%s {" % QuoteString(group.name))
		for v in mesh.vertices:
			for g in v.groups:
				if g.group == group.index:
					data.file.write(" %i %f" % (v.index, g.weight ))
		data.file.write(" }\n")
	data.file.write("\t\t}\n")
		
	# Koniec
	data.file.write("\t}\n")
	bpy.data.meshes.remove(mesh)
	
################################################################################
def ProcessArmatureObject(data,obj):
	armature = obj.data
	
	# Zakazany Parent
	if obj.parent != None:
		data.Warning("Object \"%s\" has a parent." %obj.name)

	data.armatures = data.armatures + 1
	if data.armatures > 1:
		data.Error("Multiple Armature objects found in scene.")
		
	# Poczatek
	data.file.write("\tarmature %s {\n" % QuoteString(obj.name))
	
	# Pozycja
	Location = obj.location
	data.file.write("\t\t%f,%f,%f\n" % (Location[0], Location[1], Location[2]))
	# Orientacja
	Orientation = obj.rotation_euler
	data.file.write("\t\t%f,%f,%f\n" % (Orientation[0], Orientation[1], Orientation[2]))
	# Skalowanie
	Scaling = obj.scale
	data.file.write("\t\t%f,%f,%f\n" % (Scaling[0], Scaling[1], Scaling[2]))
	
	# Bone groups
	pose = obj.pose
	bone_groups = {}
	for bone_group in obj.pose.bone_groups:
		s = bone_group.name.split('#')
		if len(s) == 2:
			name = s[0]
			parent = s[1]
		else:
			name = s[0]
			parent = ''
		data.file.write("\t\tgroup %s {\n\t\t\tparent %s\n\t\t}\n" % (QuoteString(name), QuoteString(parent)))
		bone_groups[bone_group] = name
	
	# Bones
	for bone in armature.bones:
		data.file.write("\t\tbone %s {\n" % QuoteString(bone.name))
		# Parent
		if bone.parent == None:
			sBoneParent = ""
		else:
			sBoneParent = bone.parent.name
		data.file.write("\t\t\tparent %s\n" % QuoteString(sBoneParent))
		# Group
		foundPoseBone = None
		for poseBone in pose.bones:
			if poseBone.name == bone.name:
				foundPoseBone = poseBone
				break
		if foundPoseBone is None or foundPoseBone.bone_group is None:
			sBoneGroup = ""
		else:
			sBoneGroup = bone_groups[foundPoseBone.bone_group]
		data.file.write("\t\t\tgroup %s\n" % QuoteString(sBoneGroup))
		# Head
		data.file.write("\t\t\thead %f,%f,%f %f\n" % (bone.head_local[0], bone.head_local[1], bone.head_local[2], bone.head_radius))
		# Tail
		data.file.write("\t\t\ttail %f,%f,%f %f\n" % (bone.tail_local[0], bone.tail_local[1], bone.tail_local[2], bone.tail_radius))	
		# Is connected
		data.file.write("\t\t\t%d\n" % int(bone.use_connect))
		# Pusta linia
		data.file.write("\n")
		# Macierz w ARMATURESPACE (jest 4x4)
		m = bone.matrix_local.transposed()
		for iRow in range(4):
			data.file.write("\t\t\t%f, %f, %f, %f\n" % (m[iRow][0], m[iRow][1], m[iRow][2], m[iRow][3]))
		# Koniec
		data.file.write("\t\t}\n")
		# Policz kosci
		data.bones = data.bones + 1	
	# Koniec
	data.file.write("\t}\n")
	
################################################################################
# Zapisuje pusty obiekt (uzywane do oznaczania roznych rzeczy w grze)
def ProcessEmpty(data,empty):
	type = empty.empty_draw_type
	print("Exporting %s.\n" % empty.name)
	if type == 'Image':
		Warning("Empty of Image type not supported, object "+empty.name+" ignored.")
	else:
		data.file.write("\tempty %s {\n" % QuoteString(empty.name))
		if empty.parent:
			if len(empty.parent_bone) > 0:
				bone = empty.parent_bone
			else:
				bone = "NULL"
		else:
			bone = "NULL"
		data.file.write("\t\tbone %s\n" % QuoteString(bone))
		data.file.write("\t\ttype %s\n" % QuoteString(type))
		data.file.write("\t\tsize %f,%f,%f\n" % (empty.scale[0],empty.scale[1],empty.scale[2]))
		data.file.write("\t\tscale %f\n" % empty.empty_draw_size)
		m = empty.matrix_world.transposed()
		data.file.write("\t\tmatrix\n")
		for n in m:
			data.file.write("\t\t\t%f,%f,%f,%f\n" % (n[0],n[1],n[2],n[3]))
		# w wersji 19 dodany obrot w eulerach
		mode = empty.rotation_mode
		empty.rotation_mode = 'QUATERNION'
		data.file.write("\t\trot %f,%f,%f\n" % (empty.rotation_euler.x, empty.rotation_euler.y, empty.rotation_euler.z))
		empty.rotation_mode = mode
		data.file.write("\t}\n")	

################################################################################
# Zapisuje kamere
def ProcessCamera(data,camera):
	data.file.write("\tcamera {\n")
	# Pozycja kamery
	Location = camera.location
	data.file.write("\t\t%f,%f,%f\n" % (Location[0], Location[1], Location[2]))
	# Pozycja celu
	mode = camera.rotation_mode
	camera.rotation_mode = 'QUATERNION'
	pos = Location + camera.rotation_quaternion * mathutils.Vector((0,0,-1))
	data.file.write("\t\t%f,%f,%f\n" % (pos[0], pos[1], pos[2]))
	# Up kamery
	up = camera.rotation_quaternion * mathutils.Vector((0,1,0))
	data.file.write("\t\t%f,%f,%f\n" % (up[0], up[1], up[2]))
	camera.rotation_mode = mode
	# Koniec
	data.file.write("\t}\n")
	
################################################################################
# Sprawdza co to za obiekt i wykonuje odpowiednie czynnosci
def ProcessObject(data,obj):
	type = obj.type
	print("Object %s is %s" % (data, type))
	if type == "MESH":
		ProcessMeshObject(data,obj)
	elif type == "ARMATURE":
		ProcessArmatureObject(data,obj)
	elif type == "EMPTY":
		ProcessEmpty(data,obj)
	elif type == "CAMERA":
		ProcessCamera(data,obj)
	# typy, ktore sa ignorowane i wyswietlaja ostrzezenie
	elif (type == "Curve") or (type == "Lattice") or (type == "MBall") or (type == "Surf"):
		data.Warning("Object \"" + obj.name + "\" of type \"" + type + "\" ignored.")
	# pozostale typy sa ignorowane

################################################################################
# zwraca nazwe krzywej taka jak kiedys PosX,QuatW,ScaleZ
def ConvertFCurve(channel):
	str = 0
	parts = channel.data_path.split('.')
	ile = len(parts)
	str2 = parts[ile-1]
	if str2 == "location":
		str = "Loc"
	elif str2 == "rotation_quaternion":
		str = "Quat"
	else:
		str = "Scale"
	index = channel.array_index
	if str == "Quat":
		if index == 0:
			str += 'W'
		elif index == 1:
			str += 'X'
		elif index == 2:
			str += 'Y'
		else:
			str += 'Z'
	else:
		if index == 0:
			str += 'X'
		elif index == 1:
			str += 'Y'
		else:
			str += 'Z'
	return str		

################################################################################
# Zapisuje akcje do pliku
def ProcessActions(data):
	print("Saving actions")
	data.file.write("actions {\n")
	if not data.static_ani:
		for action in bpy.data.actions:
			# ignoruj animacje z '#' na poczatku nazwy
			if action.name[0] != '#':
				data.file.write("\t%s {\n" % QuoteString(action.name))
				for group in action.groups:
					data.file.write("\t\t%s {\n" % QuoteString(group.name))
					for channel in group.channels:
						if len(channel.keyframe_points) > 0:
							sInterpolation = channel.keyframe_points[0].interpolation
							data.file.write("\t\t\t%s %s {" % (QuoteString(ConvertFCurve(channel)), sInterpolation))
							for pt in channel.keyframe_points:
								data.file.write(" %f,%f" % (pt.co[0], pt.co[1]))
							data.file.write(" }\n")
					data.file.write("\t\t}\n")
				data.file.write("\t}\n")
				data.actions = data.actions + 1
	data.file.write("}\n")

################################################################################
# Zapisuje parametry do pliku
def ProcessParams(data,scene):
	data.file.write("params {\n")
	data.file.write("\tfps %i\n" % scene.render.fps)
	data.file.write("\tstatic %i\n" % data.static_ani)
	data.file.write("\ttangents %i\n" % data.force_tan)
	data.file.write("\tsplit 0\n")
	data.file.write("}\n")

################################################################################
# Eksportuj model
def ExportQmsh(data):
	#Blender.Window.WaitCursor(True)
	print("Exporting to QMSH...")
	data.file = open(data.path, "w")
	try:
		try:
			# Zapis do pliku naglowka
			data.file.write("QMSH TMP 21\n")
			# Przetworz wszystkie obiekty biezacej sceny
			print("Saving objects %d" % len(bpy.context.selected_objects))
			data.file.write("objects {\n")
			for obj in bpy.context.selected_objects:
				ProcessObject(data,obj)
			data.file.write("}\n")
			# Przetworz wszystkie akcje
			if data.armatures == 1:
				ProcessActions(data)
			else:
				data.file.write("actions {\n}\n")
			# Wyeksportuj parametry
			ProcessParams(data,bpy.context.scene)
			# statystyki
			print("Exported: Objects=%i, Vertices=%i, Faces=%i, Bones=%i, Actions=%i" % (data.objects, data.vertices, data.faces, data.bones, data.actions))
			# czy byly jakies ostrzezenia
			if data.warnings == 0:
				sEndMessage = "Export succeeded."
			else:
				sEndMessage = "Succeeded with %i warnings." % data.warnings
			print(sEndMessage)
			return True
		except ExporterException:
			try:
				data.file.close()
				os.remove(data.path)
			except:
				pass
			return False
	finally:
		try:
			data.file.close()
		except:
			pass
	print()
	
################################################################################
def RunExport(filepath, config):
	if config.runConverter and not os.path.isfile(config.converterPath):
		raise ExporterException("Invalid converter path.")
	if filepath.endswith('.qmsh.tmp.qmsh'):
		filepath = filepath[:-4]
	elif filepath.endswith('.qmsh'):
		filepath += '.tmp'
	data = ExporterData(filepath, config.textureNames, config.useExistingArmature, config.applyModifiers, config.forceTangents)
	if not ExportQmsh(data):
		return False
	if config.runConverter:
		cmd = '"' + config.converterPath + '" "' + filepath + '"'
		print("Command: %s" % cmd)
		sub = subprocess.Popen(cmd)
		if sub.wait() != 0:
			return False
		os.remove(data.path)
	return True

################################################################################
class Config:
	def __init__(self):
		config_path = bpy.utils.user_resource('CONFIG', path='scripts', create=True)
		self.filepath = os.path.join(config_path, 'io_export_qmsh_tmp.cfg')
		self.config = configparser.ConfigParser()
		self.config.read(self.filepath)
		self.Init()
		s = self.config['settings']
		self.textureNames = s.getboolean('textureNames')
		self.useExistingArmature = s.getboolean('useExistingArmature')
		self.applyModifiers = s.getboolean('applyModifiers')
		self.forceTangents = s.getboolean('forceTangents')
		self.runConverter = s.getboolean('runConverter')
		self.converterPath = s['converterPath']
	def Init(self):
		dict = {'textureNames': 'True',
				'useExistingArmature': 'False',
				'applyModifiers': 'True',
				'forceTangents': 'False',
				'runConverter': 'True',
				'converterPath': 'D:\\carpg\\other\\mesh\\converter.exe'}
		if not self.config.has_section('settings'):
			self.config['settings'] = dict
			return
		s = self.config['settings']
		for k, v in dict.items():
			if not self.config.has_option('settings', k):
				s[k] = v
	def Save(self):
		s = self.config['settings']
		s['textureNames'] = str(self.textureNames)
		s['useExistingArmature'] = str(self.useExistingArmature)
		s['applyModifiers'] = str(self.applyModifiers)
		s['forceTangents'] = str(self.forceTangents)
		s['runConverter'] = str(self.runConverter)
		s['converterPath'] = self.converterPath
		with open(self.filepath, 'w') as configfile:
			self.config.write(configfile)

################################################################################
# Klasa eksportera
class QmshExporterOperator(bpy.types.Operator, ExportHelper):
	"""Export to the Qmsh temporary format (.qmsh.tmp)"""
	
	bl_idname = "export.qmsh"
	bl_label = "Export QMSH"
	filename_ext = ".qmsh"
	filter_glob = StringProperty(default="*.qmsh;*.qmsh.tmp", options={'HIDDEN'})
	
	config = Config()
	
	TextureNames = BoolProperty(
		name="Export texture names",
		description="File will contain texture names, otherwise only material name",
		default=config.textureNames)
	
	UseExistingArmature = BoolProperty(
		name="Use existing armature",
		description="Don't export armature and animations, only vertex groups",
		default=config.useExistingArmature)
	
	ApplyModifiers = BoolProperty(
		name="Apply modifiers",
		description="Apply mesh modifiers before exporting mesh",
		default=config.applyModifiers)
	
	ForceTangents = BoolProperty(
		name="Force tangents",
		description="Force export tangents, by default exported only if mesh use normal map",
		default=config.forceTangents)
		
	RunConverter = BoolProperty(
		name="Run converter",
		description="Run converter to process qmsh.tmp into qmsh",
		default=config.runConverter)
		
	ConverterPath = StringProperty(
		name="Converter path",
		default=config.converterPath)
		
	def execute(self, context):
		self.config.textureNames = self.TextureNames
		self.config.useExistingArmature = self.UseExistingArmature
		self.config.applyModifiers = self.ApplyModifiers
		self.config.forceTangents = self.ForceTangents
		self.config.runConverter = self.RunConverter
		self.config.converterPath = self.ConverterPath
		self.config.Save()
		try:
			RunExport(self.properties.filepath, self.config)
		except ExporterException as error:
			msg = 'Exporter error: ' +str(error)
			print("ERROR: " + msg)
			bpy.ops.error.message('INVOKE_DEFAULT', message = msg)
		return {"FINISHED"}

################################################################################
# funkcje pluginu
def menu_func(self, context):
    self.layout.operator(QmshExporterOperator.bl_idname, text="Qmsh (.qmsh.tmp)")

def register():
    bpy.utils.register_module(__name__)
    bpy.types.INFO_MT_file_export.append(menu_func)

def unregister():
    bpy.utils.unregister_module(__name__)
    bpy.types.INFO_MT_file_export.remove(menu_func)

if __name__ == "__main__":
	register()

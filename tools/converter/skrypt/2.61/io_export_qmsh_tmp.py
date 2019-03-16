bl_info = {
    "name": "Qmsh.tmp",
    "author": "Tomash",
    "version": (0, 15, 1),
    "blender": (2, 6, 1),
    "api": 37702,
    "location": "File > Export > Qmsh.tmp",
    "description": "Export to Qmsh.tmp",
    "warning": "",
    "category": "Import-Export"}
    
# UWAGI
# - przed eksportem wejdz do trybu obiektu
# - zaznacz tylko to co chcesz wyeksprtowac
# - eksportujac punkty (Empty) ustaw animacje na 'base'
# - eksportujac pancerz zaznacz tez szkielet i odpowiednia opcje
# - obiekty na roznych layerach sie nie eksportuja zazwyczaj bierze tylko z pierwszego
# - opcja Use the scene active camera na prawo od layerow sprawia ze nic nie jest zaznaczone

import bpy
from bpy.props import StringProperty, BoolProperty
from math import pi
import mathutils

################################################################################
class MyException(Exception):
	pass

################################################################################
class ExporterData:
	def __init__(self,path,export_tex,static_ani):
		self.path = path
		self.export_tex = export_tex
		self.static_ani = static_ani
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
		print("Error: " + msg)
		raise MyException

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
# Konstruuje liste materialow
# Niektore moga byc None, a indeksy materialow beda tez mogly wykraczac poza zakres tej listy.
# Mechanizm tego jest pomotany, bo zaleznie od bitow z Object.colbits dany material
# jest uzywany z listy obiektu lub z listy jego danych (siatki)
def MakeMaterialList(data,obj):
	lMaterials = []
	for slot in obj.material_slots:
		lMaterials.append( slot.material )
	if len(lMaterials) > 32:
		data.Error("Object \"" + obj.name + "\" - too many materials.")
	return lMaterials

################################################################################
def ProcessMeshObject(data,obj):
	mesh = obj.data
	data.objects = data.objects + 1
	data.vertices = data.vertices + len(mesh.vertices)
	data.faces = data.faces + len(mesh.faces)
	# Poczatek
	data.file.write("\tmesh %s {\n" % QuoteString(obj.name))
	# pozycja, rotacja, skala
	v = obj.location
	data.file.write("\t\tpos: %f,%f,%f\n" % (v[0], v[1], v[2]))
	v = obj.rotation_euler
	data.file.write("\t\trot: %f,%f,%f\n" % (v[0], v[1], v[2]))
	v = obj.scale
	data.file.write("\t\tscale: %f,%f,%f\n" % (v[0], v[1], v[2]))
	# Parent Armature i Parent Bone
	if obj.parent == None:
		sParent = ""
	else:
		if obj.parent.type != "ARMATURE":
			data.Warning("Object \"%s\" has a parent." % obj.name)
		sParent = obj.parent.name
	data.file.write("\t\tparent: %s %s\n" % (QuoteString(sParent), QuoteString(obj.parent_bone)))
	# AutoSmoothAngle
	# w 2.49 bylo w katach, teraz jest w radianach
	angle = mesh.auto_smooth_angle*180/pi;
	data.file.write("\t\tsmooth: %f\n" % angle)
	
	# Materialy
	mats = MakeMaterialList(data,obj)
	data.file.write("\t\tmaterials %i {\n" % len(mats))
	for mat in mats:
		if mat == None:
			data.file.write("\t\t\t%s, " % QuoteString(""))
		else:
			data.file.write("\t\t\t%s, " % QuoteString(mat.name))
		if data.export_tex:
			ts = mat.texture_slots[0]
			if ts:
				tex = ts.texture
				if tex.type == 'IMAGE':
					data.file.write("image %s\n" % QuoteString(tex.image.name))
				else:
					data.Warning("Unsuported texture type '%s'!" % tex.type)
					data.file.write("none\n")	
			else:
				data.file.write("none\n")	
		else:
			data.file.write("none\n")
	data.file.write("\t\t}\n")
	
	# Wierzcholki
	data.file.write("\t\tvertices %i {\n" % len(mesh.vertices))
	for v in mesh.vertices:
		data.file.write("\t\t\t%f,%f,%f\n" % (v.co[0], v.co[1], v.co[2]))
	data.file.write("\t\t}\n")
	
	# scianki
	data.file.write("\t\tfaces %i {\n" % len(mesh.faces))
	if len(mesh.uv_textures) == 0:
		for f in mesh.faces:
			if f.use_smooth:
				sSmooth = "1"
			else:
				sSmooth = "0"
			data.file.write("\t\t\t%i %i %s" % (len(f.vertices), f.material_index, sSmooth));
			ii = 0
			for v in f.vertices:
				data.file.write(" %i 0,0" % (v))
			data.file.write(" %f,%f,%f\n" % (f.normal.x,f.normal.y,f.normal.z))
	else:
		uv = mesh.uv_textures[0].data
		for f in mesh.faces:
			if f.use_smooth:
				sSmooth = "1"
			else:
				sSmooth = "0"
			data.file.write("\t\t\t%i %i %s" % (len(f.vertices), f.material_index, sSmooth));
			iVertIndex = 0
			tuv = uv[f.index]
			ii = 0
			for v in f.vertices:
				data.file.write(" %i %f,%f" % (v, tuv.uv[ii][0], tuv.uv[ii][1]))
				iVertIndex = iVertIndex + 1
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

	# Parametry calego Armature
	if armature.use_deform_vertex_groups:
		sVertexGroups = "1"
	else:
		sVertexGroups = "0"
	if armature.use_deform_envelopes:
		sEnvelopes = "1"
	else:
		sEnvelopes = "0"
	data.file.write("\t\t%s %s\n" % (sVertexGroups, sEnvelopes))
	
	# Dla kolejnych kosci
	for bone in armature.bones:
		# Sprawdzenie
		#if oBone.subdivisions != 1:
		#	Warning("Bone \"%s\" has %i subdivisions (not supported)." % (sBoneKey, oBone.subdivisions))
		# czyzby chodzilo o use_envelope_multiply albo bbone_segments ?
		# Poczatek
		data.file.write("\t\tbone %s {\n" % QuoteString(bone.name))
		# Parent
		if bone.parent == None:
			sBoneParent = ""
		else:
			sBoneParent = bone.parent.name
		data.file.write("\t\t\t%s\n" % QuoteString(sBoneParent))
		# Head
		data.file.write("\t\t\thead %f,%f,%f %f,%f,%f %f\n" % (bone.head[0], bone.head[1], bone.head[2], bone.head_local[0], bone.head_local[1], bone.head_local[2], bone.head_radius))
		# Tail
		data.file.write("\t\t\ttail %f,%f,%f %f,%f,%f %f\n" % (bone.tail[0], bone.tail[1], bone.tail[2], bone.tail_local[0], bone.tail_local[1], bone.tail_local[2], bone.tail_radius))
		# Roll
		# sprawdzalem w kodzie convertera i ta wartosc jest nieuzywana
		# w blender 2.50+ wogole jej nie ma
		# na razie dla zachowania zgodnosci to zostawiam, potem sie wywali
		data.file.write("\t\t\t%f %f\n" % (1.0000, 1.0000))
		# Length, Weight
		# weight juz nie ma, zawsze bylo zero wiec jest na stale
		data.file.write("\t\t\t%f %f\n" % (bone.length,1.0000))
		# Pusta linia
		data.file.write("\n")
		# Macierz w BONESPACE (jest 3x3)
		for iRow in range(3):
			data.file.write("\t\t\t%f, %f, %f;\n" % (bone.matrix[iRow][0], bone.matrix[iRow][1], bone.matrix[iRow][2]))
		# Pusta linia
		data.file.write("\n")
		# Macierz w ARMATURESPACE (jest 4x4)
		for iRow in range(4):
			data.file.write("\t\t\t%f, %f, %f, %f;\n" % (bone.matrix_local[iRow][0], bone.matrix_local[iRow][1], bone.matrix_local[iRow][2], bone.matrix_local[iRow][3]))
		# Koniec
		data.file.write("\t\t}\n")
		# Policz kosci
		data.bones = data.bones + 1	
	# Koniec
	data.file.write("\t}\n")
	
################################################################################
# Zapisuje pusty obiekt (uzywane do oznaczania roznych rzeczy w grze)
def ProcessEmpty(data,empty):
	empty_type = empty.empty_draw_type
	if empty_type == 'Image':
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
		data.file.write("\t\tbone: %s\n" % QuoteString(bone))
		data.file.write("\t\ttype: %s\n" % QuoteString(empty_type))
		data.file.write("\t\tsize: %f,%f,%f\n" % (empty.scale[0],empty.scale[1],empty.scale[2]))
		data.file.write("\t\tscale: %f\n" % empty.empty_draw_size);
		m = empty.matrix_local
		data.file.write("\t\tmatrix:\n")
		for n in m:
			data.file.write("\t\t\t%f,%f,%f,%f;\n" % (n[0],n[1],n[2],n[3]))
		data.file.write("\t}\n")	

################################################################################
# Zapisuje kamere
def ProcessCamera(data,camera):
	data.file.write("\tcamera {\n");
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
	name = 0
	parts = channel.data_path.split('.')
	ile = len(parts)
	str2 = parts[ile-1]
	if str2 == "location":
		name = "Loc"
	elif str2 == "rotation_quaternion":
		name = "Quat"
	else:
		name = "Scale"
	index = channel.array_index
	if name == "Quat":
		if index == 0:
			name += 'W'
		elif index == 1:
			name += 'X'
		elif index == 2:
			name += 'Y'
		else:
			name += 'Z'
	else:
		if index == 0:
			name += 'X'
		elif index == 1:
			name += 'Y'
		else:
			name += 'Z'
	return name

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
							# tak opcja juz nie istnieje, dla kompatybilnosci wypisuje 'CONST'
							# niby jest channel.extrapolation ale nie wiem czy to to samo
							data.file.write("\t\t\t%s %s CONST {" % (QuoteString(ConvertFCurve(channel)), sInterpolation))
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
	data.file.write("\tfps = %i\n" % scene.render.fps)
	if data.static_ani:
		n = 1
	else:
		n = 0
	data.file.write("\tstatic = %i\n" % n)
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
			data.file.write("QMSH TMP 15\n")
			# Sprawdzenie czy w EditMode
			mode = bpy.context.mode
			if mode == "EDIT_MESH" or mode == "EDIT_ARMATURE":
				data.Warning("You are in EditMode - changed may not be respected.")
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
		except MyException:
			try:
				data.file.close()
				os.remove(g_sFileName)
			except:
				pass
			#Blender.Window.WaitCursor(False)
			#Blender.Draw.PupBlock(g_sPluginTitle, ["ERROR. Check console."])
	finally:
		try:
			data.file.close()
		except:
			pass
	print()

################################################################################
# Klasa eksportera
class QmshExporter(bpy.types.Operator):
    """Export to the Qmsh temporary format (.qmsh.tmp)"""

    bl_idname = "export.qmsh"
    bl_label = "Export QMSH.TMP"

    filepath = StringProperty(subtype='FILE_PATH')
    
    TextureNames = BoolProperty(
    	name="Export texture names",
    	description="File will contain texture names, otherwise only material name",
    	default=True)
    
    UseExistingArmature = BoolProperty(
		name="Use existing armature",
		description="Don't export armature and animations, only vertex groups",
		default=False)

    def execute(self, context):
        FilePath = self.filepath
        data = ExporterData(self.filepath,self.TextureNames,self.UseExistingArmature)
        ExportQmsh(data)
        return {"FINISHED"}

    def invoke(self, context, event):
        if not self.filepath:
            self.filepath = bpy.path.ensure_ext(bpy.data.filepath, ".qmsh.tmp")
        WindowManager = context.window_manager
        WindowManager.fileselect_add(self)
        return {"RUNNING_MODAL"}

################################################################################
# funkcje pluginu
def menu_func(self, context):
    self.layout.operator(QmshExporter.bl_idname, text="Qmsh (.qmsh.tmp)")

def register():
    bpy.utils.register_module(__name__)
    bpy.types.INFO_MT_file_export.append(menu_func)

def unregister():
    bpy.utils.unregister_module(__name__)
    bpy.types.INFO_MT_file_export.remove(menu_func)

if __name__ == "__main__":
    register()

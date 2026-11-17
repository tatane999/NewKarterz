import bpy
bpy.ops.wm.read_factory_settings(use_empty=True)
bpy.ops.import_scene.gltf(filepath=r'C:\Users\Antoine\Documents\NewKarterz_3D\polyhaven_models\fir_tree_01\fir_tree_01.gltf')
log=[]
for o in list(bpy.context.scene.objects):
    if o.type=='MESH' and not o.name.startswith('fir_tree_01_b'): bpy.data.objects.remove(o,do_unlink=True)
o=[x for x in bpy.context.scene.objects if x.type=='MESH'][0]
o.location=(0,0,0)
me=o.data
for a in list(me.color_attributes): me.color_attributes.remove(a)
log.append('mats '+','.join(m.name for m in me.materials))
bpy.context.view_layer.objects.active=o;o.select_set(True)
log.append('polys %d dims %s'%(len(o.data.polygons),tuple(round(v,2) for v in o.dimensions)))
bpy.ops.export_scene.gltf(filepath=r'C:\Users\Antoine\Documents\NewKarterz_3D\ue_import\ph\ph_fir2.glb',export_format='GLB',use_selection=True)
open(r'C:\Users\Antoine\Documents\NewKarterz_3D\join_fir.txt','w').write('\n'.join(log))

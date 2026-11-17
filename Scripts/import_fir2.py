import unreal
AT=unreal.AssetToolsHelpers.get_asset_tools();EAL=unreal.EditorAssetLibrary;MEL=unreal.MaterialEditingLibrary;out=[]
f=r'C:\Users\Antoine\Documents\NewKarterz_3D\ue_import\ph\ph_fir2.glb'
tmp='/Game/NK/PH/src_ph_fir2';final='/Game/NK/PH/ph_fir2'
if EAL.does_asset_exist(final): EAL.delete_asset(final)
if EAL.does_directory_exist(tmp): EAL.delete_directory(tmp)
t=unreal.AssetImportTask();t.filename=f;t.destination_path=tmp;t.automated=True;t.replace_existing=True;t.save=True
AT.import_asset_tasks([t])
sms=[a.split('.')[0] for a in EAL.list_assets(tmp,True,False) if isinstance(unreal.load_asset(a),unreal.StaticMesh)]
out.append('meshes %s'%sms)
sm=sms[0];EAL.rename_asset(sm,final);o=unreal.load_asset(final)
ns=o.get_editor_property('nanite_settings');ns.enabled=True;ns.set_editor_property('shape_preservation',unreal.NaniteShapePreservation.PRESERVE_AREA);o.set_editor_property('nanite_settings',ns)
twig=unreal.load_asset('/Game/NK/PH/src_ph_fir/ph_fir/Materials/fir_tree_01_twig')
mats=o.get_editor_property('static_materials')
for i,sm_ in enumerate(mats):
    out.append('%d %s'%(i,sm_.material_slot_name))
    if 'twig' in str(sm_.material_slot_name).lower(): o.set_material(i,twig);out.append('  twig set')
EAL.save_asset(final)
b=o.get_bounds();out.append('bounds %s'%b.box_extent)
open(r'C:\Users\Antoine\Documents\NewKarterz_3D\fir2.txt','w').write('\n'.join(out))

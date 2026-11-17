import unreal
EAL=unreal.EditorAssetLibrary;MEL=unreal.MaterialEditingLibrary;out=[]
for a in EAL.list_assets('/Game/NK/PH',True,False):
    o=unreal.load_asset(a)
    if not isinstance(o,unreal.MaterialInstanceConstant): continue
    bm=o.get_blend_mode()
    out.append('%s %s'%(o.get_name(),bm))
    if bm!=unreal.BlendMode.BLEND_OPAQUE and bm!=unreal.BlendMode.BLEND_MASKED or any(k in o.get_name().lower() for k in ['twig','leaf','leaves','branch','grass','fern']):
        ov=o.get_editor_property('base_property_overrides');ov.set_editor_property('override_blend_mode',True);ov.set_editor_property('blend_mode',unreal.BlendMode.BLEND_MASKED)
        ov.set_editor_property('override_two_sided',True);ov.set_editor_property('two_sided',True)
        o.set_editor_property('base_property_overrides',ov);MEL.update_material_instance(o);EAL.save_loaded_asset(o);out.append('  -> masked')
# terrain forêt plus verte
mi=unreal.load_asset('/Game/NK/Mat/MI_Terrain_pine')
MEL.set_material_instance_vector_parameter_value(mi,'Tint',unreal.LinearColor(0.62,0.95,0.42,1));EAL.save_loaded_asset(mi)
open(r'C:\Users\Antoine\Documents\NewKarterz_3D\fix_masks.txt','w').write('\n'.join(out))

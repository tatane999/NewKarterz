import unreal
EAL=unreal.EditorAssetLibrary;out=[]
for n in ['ph_fir2','ph_fir','ph_shrub_02','ph_shrub_01','ph_island_tree_01']:
    p='/Game/NK/PH/'+n
    if not EAL.does_asset_exist(p): continue
    m=unreal.load_asset(p);ns=m.get_editor_property('nanite_settings')
    try: ns.set_editor_property('preserve_area',True)
    except Exception as e: out.append('pa err %s'%e)
    try: ns.set_editor_property('shape_preservation',unreal.NaniteShapePreservation.PRESERVE_AREA)
    except Exception as e: out.append('sp err %s'%e)
    m.set_editor_property('nanite_settings',ns);EAL.save_asset(p);out.append('ok '+n)
open(r'C:\Users\Antoine\Documents\NewKarterz_3D\nanite_area.txt','w').write('\n'.join(out))

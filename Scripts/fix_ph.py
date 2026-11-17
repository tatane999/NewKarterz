import unreal
EAL=unreal.EditorAssetLibrary;out=[]
for n in ['ph_fir','ph_rock_moss_set_01','ph_shrub_02','ph_boulder_01','ph_namaqualand_boulder_02','ph_shrub_01','ph_coast_rocks_01','ph_island_tree_01','ph_rock_07']:
    src='/Game/NK/PH/src_%s/%s/StaticMeshes/%s'%(n,n,n)
    if not EAL.does_asset_exist(src): out.append('nosrc '+n);continue
    fin='/Game/NK/PH/'+n
    if EAL.does_asset_exist(fin): EAL.delete_asset(fin)
    EAL.rename_asset(src,fin);o=unreal.load_asset(fin)
    ns=o.get_editor_property('nanite_settings');ns.enabled=True;o.set_editor_property('nanite_settings',ns);EAL.save_asset(fin)
    b=o.get_bounding_box();out.append('%s tris=%d size=%s'%(n,o.get_num_triangles(0),b.max-b.min))
open(r'C:\Users\Antoine\Documents\NewKarterz_3D\fix_ph.txt','w').write('\n'.join(out))

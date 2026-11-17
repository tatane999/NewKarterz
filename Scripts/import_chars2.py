# Import des karts découpés (carrosserie + roues séparées) : ue_import/chars2/cN_body.glb, cN_wK.glb → /Game/NK/Chars2
import unreal, os, glob
AT=unreal.AssetToolsHelpers.get_asset_tools(); EAL=unreal.EditorAssetLibrary; LOG=[]
SRC=r'C:\Users\Antoine\Documents\NewKarterz_3D\ue_import\chars2'
for f in sorted(glob.glob(SRC+r'\c*.glb')):
    n=os.path.basename(f)[:-4]; final='/Game/NK/Chars2/'+n; tmp='/Game/NK/Chars2/src_'+n
    if EAL.does_asset_exist(final): continue
    t=unreal.AssetImportTask(); t.filename=f; t.destination_path=tmp; t.automated=True; t.replace_existing=True; t.save=True
    AT.import_asset_tasks([t])
    sms=[a.split('.')[0] for a in EAL.list_assets(tmp,True,False) if isinstance(unreal.load_asset(a),unreal.StaticMesh)]
    if not sms: LOG.append('ECHEC '+n); continue
    best=max(sms,key=lambda a: unreal.load_asset(a).get_num_triangles(0))
    EAL.rename_asset(best,final); EAL.save_asset(final); LOG.append('ok '+n)
open(r'C:\Users\Antoine\Documents\NewKarterz_3D\import_chars2.txt','w').write('\n'.join(LOG))

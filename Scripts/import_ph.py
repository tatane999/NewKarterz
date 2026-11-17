
import unreal,os,glob
AT=unreal.AssetToolsHelpers.get_asset_tools();EAL=unreal.EditorAssetLibrary
LOG=[]
for f in sorted(glob.glob(r'C:\Users\Antoine\Documents\NewKarterz_3D\ue_import\ph\ph_*.glb')):
    n=os.path.basename(f)[:-4];final='/Game/NK/PH/'+n;tmp='/Game/NK/PH/src_'+n
    if EAL.does_asset_exist(final): continue
    t=unreal.AssetImportTask();t.filename=f;t.destination_path=tmp;t.automated=True;t.replace_existing=True;t.save=True
    AT.import_asset_tasks([t]);sm=None
    for a in EAL.list_assets(tmp,True,False):
        if isinstance(unreal.load_asset(a),unreal.StaticMesh):sm=a.split('.')[0];break
    if not sm: LOG.append('FAIL '+n);continue
    EAL.rename_asset(sm,final);o=unreal.load_asset(final)
    ns=o.get_editor_property('nanite_settings');ns.enabled=True;o.set_editor_property('nanite_settings',ns);EAL.save_asset(final);LOG.append('ok '+n)
open(r'C:\Users\Antoine\Documents\NewKarterz_3D\import_ph_log.txt','w').write('\n'.join(LOG))

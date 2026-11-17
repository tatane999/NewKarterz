import unreal, os, glob
AT=unreal.AssetToolsHelpers.get_asset_tools(); EAL=unreal.EditorAssetLibrary
ROOT=r'C:\Users\Antoine\Documents\NewKarterz_3D\ue_import\joined'
LOG=[]
def imp(f, dest_dir, final):
    if EAL.does_asset_exist(final): EAL.delete_asset(final)
    tmp=dest_dir+'/src_'+os.path.basename(final)
    if EAL.does_directory_exist(tmp): EAL.delete_directory(tmp)
    t=unreal.AssetImportTask(); t.filename=f; t.destination_path=tmp; t.automated=True; t.replace_existing=True; t.save=True
    AT.import_asset_tasks([t])
    sm=None
    for a in EAL.list_assets(tmp,True,False):
        o=unreal.load_asset(a)
        if isinstance(o,unreal.StaticMesh): sm=a.split('.')[0]; break
    if not sm: LOG.append('FAIL '+f); return None
    EAL.rename_asset(sm, final); o=unreal.load_asset(final)
    try:
        ns=o.get_editor_property('nanite_settings'); ns.enabled=True; o.set_editor_property('nanite_settings',ns)
    except Exception as e: LOG.append('nanite '+str(e))
    EAL.save_asset(final); LOG.append('ok '+final); return o
for f in sorted(glob.glob(ROOT+r'\c*.glb')):
    n=os.path.basename(f)[:-4]; imp(f,'/Game/NK/Chars','/Game/NK/Chars/'+n)
for f in sorted(glob.glob(ROOT+r'\t*.glb')):
    n=os.path.basename(f)[:-4]; imp(f,'/Game/NK/Decor','/Game/NK/Decor/'+n)
EAL.save_directory('/Game/NK')
open(r'C:\Users\Antoine\Documents\NewKarterz_3D\import2_log.txt','w').write('\n'.join(LOG))

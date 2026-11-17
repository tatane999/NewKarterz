
import unreal
t=unreal.AssetImportTask();t.filename=r'C:\Users\Antoine\Documents\NewKarterz_3D\ue_import\axtest.glb';t.destination_path='/Game/NK/Test';t.automated=True;t.replace_existing=True;t.save=True
unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([t])
out=[]
for a in unreal.EditorAssetLibrary.list_assets('/Game/NK/Test',True,False):
    o=unreal.load_asset(a)
    if isinstance(o,unreal.StaticMesh):b=o.get_bounding_box();out.append('%s min=%s max=%s'%(a,b.min,b.max))
open(r'C:\Users\Antoine\Documents\NewKarterz_3D\axtest.txt','w').write('\n'.join(out))

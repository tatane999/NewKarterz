
import unreal
EAL=unreal.EditorAssetLibrary;AT=unreal.AssetToolsHelpers.get_asset_tools()
out=[]
if not EAL.does_asset_exist('/Game/Maps/Race'):
    try:
        w=AT.create_asset('Race','/Game/Maps',unreal.World,unreal.WorldFactory());out.append('world %s'%w)
        EAL.save_asset('/Game/Maps/Race')
    except Exception as e: out.append('worlderr %s'%e)
for i in range(13):
    out.append('k%d %s'%(i,EAL.does_asset_exist('/Game/NK/Karts/k%d'%i)))
    if not EAL.does_asset_exist('/Game/NK/Karts/k%d'%i):
        for a in EAL.list_assets('/Game/NK/Karts/src%d'%i,True,False):
            o=unreal.load_asset(a)
            if isinstance(o,unreal.StaticMesh):
                out.append('  sm %s'%a); EAL.rename_asset(a.split('.')[0],'/Game/NK/Karts/k%d'%i); break
for m in ['M_Road','M_Terrain','M_Water','M_Curb','M_Barrier','M_Flame','MI_Terrain_palm','MI_Road_palm','MI_Dirt_palm']:
    out.append('%s %s'%(m,EAL.does_asset_exist('/Game/NK/Mat/'+m)))
sm=unreal.load_asset('/Game/NK/Karts/k0')
if sm: b=sm.get_bounding_box(); out.append('bounds %s %s'%(b.min,b.max))
EAL.save_directory('/Game')
open(r'C:\Users\Antoine\Documents\NewKarterz_3D\ue_fix1.txt','w').write('\n'.join(out))

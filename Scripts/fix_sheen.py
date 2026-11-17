# Les matériaux « Sheen » (fourrure/tissu des personnages, glTF KHR_materials_sheen) s'affichent noirs :
# on les rebranche sur l'instance standard opaque (la couleur BaseColorFactor est conservée).
import unreal
EAL=unreal.EditorAssetLibrary; MEL=unreal.MaterialEditingLibrary
std=unreal.load_asset('/Game/NK/Mat/Gltf/MI_Default_Opaque'); std_ds=unreal.load_asset('/Game/NK/Mat/Gltf/MI_Default_Opaque_DS')
n=0; LOG=[]
for a in EAL.list_assets('/Game/NK', True, False):
    if '/Materials/' not in a: continue
    o=unreal.load_asset(a)
    if not isinstance(o, unreal.MaterialInstanceConstant): continue
    par=o.get_editor_property('parent')
    if par and 'Sheen' in par.get_name():
        MEL.set_material_instance_parent(o, std_ds if par.get_name().endswith('_DS') else std)
        MEL.update_material_instance(o); EAL.save_loaded_asset(o); n+=1
LOG.append('sheen rebranchés %d'%n)
open(r'C:\Users\Antoine\Documents\NewKarterz_3D\fix_sheen.txt','w',encoding='utf-8').write('\n'.join(LOG))

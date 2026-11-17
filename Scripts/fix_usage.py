# Les matériaux importés (Interchange glTF) héritent de matériaux du plugin sans l'usage « Instanced Static Meshes » :
# en jeu, Unreal les remplace par le matériau par défaut (décor gris/noir). On copie les parents dans /Game/NK/Mat/Gltf
# avec les bons usages, puis on rebranche les instances de matériau du projet dessus.
import unreal
EAL=unreal.EditorAssetLibrary; MEL=unreal.MaterialEditingLibrary
LOG=[]; DST='/Game/NK/Mat/Gltf'
def log(*a): LOG.append(' '.join(str(x) for x in a))
def usages(m):
    ch=False
    for p in ('used_with_instanced_static_meshes','used_with_nanite','used_with_static_lighting'):
        try:
            if not m.get_editor_property(p): m.set_editor_property(p, True); ch=True
        except Exception as e: log('usage?',p,e)
    return ch
copies={}
def local_copy(obj):
    """copie dans /Game un parent hors projet (matériau ou instance), récursivement"""
    path=obj.get_path_name().split('.')[0]
    if path.startswith('/Game/'): return obj
    if path in copies: return copies[path]
    name=path.split('/')[-1]; dst=DST+'/'+name
    if not EAL.does_asset_exist(dst): EAL.duplicate_asset(path, dst)
    c=unreal.load_asset(dst)
    if isinstance(c, unreal.MaterialInstanceConstant):
        par=c.get_editor_property('parent')
        if par: MEL.set_material_instance_parent(c, local_copy(par)); MEL.update_material_instance(c)
    elif isinstance(c, unreal.Material):
        usages(c); MEL.recompile_material(c)
    EAL.save_loaded_asset(c); copies[path]=c; log('copie',path,'->',dst)
    return c
n=0
for a in EAL.list_assets('/Game/NK', True, False):
    if a.startswith(DST): continue
    if '/Materials/' not in a and not a.startswith('/Game/NK/Mat/'): continue
    o=unreal.load_asset(a)
    if isinstance(o, unreal.Material):
        if usages(o): MEL.recompile_material(o); EAL.save_loaded_asset(o); log('usages',a)
    elif isinstance(o, unreal.MaterialInstanceConstant):
        par=o.get_editor_property('parent')
        if par and not par.get_path_name().startswith('/Game/'):
            MEL.set_material_instance_parent(o, local_copy(par)); MEL.update_material_instance(o); EAL.save_loaded_asset(o); n+=1
log('instances rebranchées', n)
open(r'C:\Users\Antoine\Documents\NewKarterz_3D\fix_usage.txt','w',encoding='utf-8').write('\n'.join(LOG))

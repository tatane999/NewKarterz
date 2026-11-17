# Import de la végétation Poly Haven (variantes ph_<id>_<k>.glb), feuillage masqué + vent.
import unreal, os, glob
AT=unreal.AssetToolsHelpers.get_asset_tools(); EAL=unreal.EditorAssetLibrary; MEL=unreal.MaterialEditingLibrary
LOG=[]
def log(*a): LOG.append(' '.join(str(x) for x in a))
SRC=r'C:\Users\Antoine\Documents\NewKarterz_3D\ue_import\ph'
IDS=['grass_medium_02','fern_02','flower_gazania','wild_rooibos_bush']
FOL=unreal.load_asset('/Game/NK/Mat/M_Foliage')

# ---------- vent dans M_Foliage et M_DecorInst (paramètre « Wind », en cm) ----------
def E(m, cls, x, y): return MEL.create_material_expression(m, cls, x, y)
def C(a,ao,b,bi): MEL.connect_material_expressions(a,ao,b,bi)
def add_wind(m, default):
    if m is None: return
    w=E(m, unreal.MaterialExpressionScalarParameter, -900, 900); w.set_editor_property('parameter_name','Wind'); w.set_editor_property('default_value',default)
    t=E(m, unreal.MaterialExpressionTime, -1300, 1000)
    tm=E(m, unreal.MaterialExpressionMultiply, -1150, 1000); tm.set_editor_property('const_b',1.7); C(t,'',tm,'A')
    wp=E(m, unreal.MaterialExpressionWorldPosition, -1300, 1150)
    k=E(m, unreal.MaterialExpressionConstant3Vector, -1300, 1250); k.set_editor_property('constant', unreal.LinearColor(0.004,0.003,0,0))
    dp=E(m, unreal.MaterialExpressionDotProduct, -1150, 1150); C(wp,'',dp,'A'); C(k,'',dp,'B')
    ad=E(m, unreal.MaterialExpressionAdd, -1000, 1050); C(tm,'',ad,'A'); C(dp,'',ad,'B')
    sn=E(m, unreal.MaterialExpressionSine, -850, 1050); sn.set_editor_property('period', 6.2831853); C(ad,'',sn,'')
    lp=E(m, unreal.MaterialExpressionLocalPosition, -1150, 1350)
    lz=E(m, unreal.MaterialExpressionComponentMask, -1000, 1350); lz.set_editor_property('r',False); lz.set_editor_property('g',False); lz.set_editor_property('b',True); C(lp,'',lz,'')
    dz=E(m, unreal.MaterialExpressionDivide, -850, 1350); dz.set_editor_property('const_b',150.0); C(lz,'',dz,'A')
    sa=E(m, unreal.MaterialExpressionSaturate, -700, 1350); C(dz,'',sa,'')
    m1=E(m, unreal.MaterialExpressionMultiply, -700, 1050); C(sn,'',m1,'A'); C(w,'',m1,'B')
    m2=E(m, unreal.MaterialExpressionMultiply, -550, 1150); C(m1,'',m2,'A'); C(sa,'',m2,'B')
    m3=E(m, unreal.MaterialExpressionMultiply, -400, 1250); m3.set_editor_property('const_b',0.6); C(m2,'',m3,'A')
    a1=E(m, unreal.MaterialExpressionAppendVector, -250, 1150); C(m2,'',a1,'A'); C(m3,'',a1,'B')
    z0=E(m, unreal.MaterialExpressionConstant, -250, 1300)
    a2=E(m, unreal.MaterialExpressionAppendVector, -100, 1200); C(a1,'',a2,'A'); C(z0,'',a2,'B')
    MEL.connect_material_property(a2,'',unreal.MaterialProperty.MP_WORLD_POSITION_OFFSET)
    MEL.recompile_material(m); EAL.save_loaded_asset(m); log('vent', m.get_name())
for p,dflt in (('/Game/NK/Mat/M_Foliage',6.0),('/Game/NK/Mat/M_DecorInst',0.0)):
    m=unreal.load_asset(p)
    try:
        if m and not MEL.get_material_property_input_node(m, unreal.MaterialProperty.MP_WORLD_POSITION_OFFSET): add_wind(m, dflt)
        else: log('vent déjà présent', p)
    except Exception as e: log('vent ERREUR', p, e)

# ---------- import ----------
for f in sorted(glob.glob(SRC+r'\ph_*.glb')):
    n=os.path.basename(f)[:-4]
    if not any(n.startswith('ph_'+i+'_') for i in IDS): continue
    final='/Game/NK/PH/'+n; tmp='/Game/NK/PH/src_'+n
    if EAL.does_asset_exist(final): log('existe', n); continue
    t=unreal.AssetImportTask(); t.filename=f; t.destination_path=tmp; t.automated=True; t.replace_existing=True; t.save=True
    AT.import_asset_tasks([t])
    sms=[a.split('.')[0] for a in EAL.list_assets(tmp,True,False) if isinstance(unreal.load_asset(a),unreal.StaticMesh)]
    if not sms: log('ECHEC', n); continue
    best=max(sms, key=lambda a: unreal.load_asset(a).get_num_triangles(0))
    EAL.rename_asset(best, final); o=unreal.load_asset(final)
    ns=o.get_editor_property('nanite_settings'); ns.enabled=True
    try: ns.set_editor_property('shape_preservation', unreal.NaniteShapePreservation.PRESERVE_AREA)
    except Exception as e: log('preserve', e)
    o.set_editor_property('nanite_settings', ns)
    # matériaux : feuillage masqué deux faces + vent (M_Foliage), textures conservées
    for i,s in enumerate(o.get_editor_property('static_materials')):
        mi=s.material_interface
        if not isinstance(mi, unreal.MaterialInstanceConstant) or not FOL: continue
        texs={str(q.parameter_info.name): q.parameter_value for q in mi.get_editor_property('texture_parameter_values')}
        MEL.set_material_instance_parent(mi, FOL)
        for k in ('BaseColorTexture','NormalTexture'):
            if texs.get(k): MEL.set_material_instance_texture_parameter_value(mi, k, texs[k])
        MEL.set_material_instance_scalar_parameter_value(mi, 'Wind', 0.0 if 'twigs' in mi.get_name() or ('bush' in mi.get_name() and 'leaves' not in mi.get_name()) else 7.0)
        MEL.update_material_instance(mi); EAL.save_loaded_asset(mi)
    EAL.save_asset(final)
    b=o.get_bounds().box_extent; log('ok', n, 'tris', o.get_num_triangles(0), 'ext', round(b.x), round(b.y), round(b.z))
open(r'C:\Users\Antoine\Documents\NewKarterz_3D\import_ph2.txt','w',encoding='utf-8').write('\n'.join(LOG))

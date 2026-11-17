# Matériaux des effets : M_FxAdd (additif, étincelles/flammes) et M_FxDust (translucide éclairé, poussière/neige).
# Couleur et opacité par instance : PerInstanceCustomData 0..2 (rgb) et 3 (alpha).
import unreal
EAL=unreal.EditorAssetLibrary; MEL=unreal.MaterialEditingLibrary; AT=unreal.AssetToolsHelpers.get_asset_tools(); LOG=[]
def E(m, cls, x, y): return MEL.create_material_expression(m, cls, x, y)
def C(a,ao,b,bi): MEL.connect_material_expressions(a,ao,b,bi)
def new_mat(name):
    p='/Game/NK/Mat/'+name
    if EAL.does_asset_exist(p): EAL.delete_asset(p)
    m=AT.create_asset(name,'/Game/NK/Mat',unreal.Material,unreal.MaterialFactoryNew())
    for u in ('used_with_instanced_static_meshes',): m.set_editor_property(u, True)
    return m
def cd(m, i, x, y):
    e=E(m, unreal.MaterialExpressionPerInstanceCustomData, x, y); e.set_editor_property('data_index', i); e.set_editor_property('const_default_value', 1.0); return e
def rgb(m, x, y):
    r=cd(m,0,x,y); g=cd(m,1,x,y+100); b=cd(m,2,x,y+200)
    a=E(m, unreal.MaterialExpressionAppendVector, x+200, y+50); C(r,'',a,'A'); C(g,'',a,'B')
    c=E(m, unreal.MaterialExpressionAppendVector, x+350, y+100); C(a,'',c,'A'); C(b,'',c,'B'); return c
try:
    m=new_mat('M_FxAdd'); m.set_editor_property('shading_model', unreal.MaterialShadingModel.MSM_UNLIT); m.set_editor_property('blend_mode', unreal.BlendMode.BLEND_ADDITIVE)
    col=rgb(m,-900,0); al=cd(m,3,-900,350)
    k=E(m, unreal.MaterialExpressionScalarParameter, -600, 450); k.set_editor_property('parameter_name','Intensity'); k.set_editor_property('default_value', 30.0)
    m1=E(m, unreal.MaterialExpressionMultiply, -400, 100); C(col,'',m1,'A'); C(al,'',m1,'B')
    m2=E(m, unreal.MaterialExpressionMultiply, -250, 150); C(m1,'',m2,'A'); C(k,'',m2,'B')
    MEL.connect_material_property(m2,'',unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    MEL.recompile_material(m); EAL.save_loaded_asset(m); LOG.append('M_FxAdd ok')
except Exception as e: LOG.append('M_FxAdd ERREUR %s'%e)
try:
    m=new_mat('M_FxDust'); m.set_editor_property('blend_mode', unreal.BlendMode.BLEND_TRANSLUCENT)
    try: m.set_editor_property('translucency_lighting_mode', unreal.TranslucencyLightingMode.TLM_VOLUMETRIC_NON_DIRECTIONAL)
    except Exception as e: LOG.append('tlm %s'%e)
    col=rgb(m,-900,0); al=cd(m,3,-900,350)
    MEL.connect_material_property(col,'',unreal.MaterialProperty.MP_BASE_COLOR)
    fr=E(m, unreal.MaterialExpressionFresnel, -700, 500); fr.set_editor_property('exponent', 1.6)
    om=E(m, unreal.MaterialExpressionOneMinus, -550, 500); C(fr,'',om,'')
    op=E(m, unreal.MaterialExpressionMultiply, -350, 400); C(al,'',op,'A'); C(om,'',op,'B')
    MEL.connect_material_property(op,'',unreal.MaterialProperty.MP_OPACITY)
    ro=E(m, unreal.MaterialExpressionConstant, -350, 250); ro.set_editor_property('r', 1.0); MEL.connect_material_property(ro,'',unreal.MaterialProperty.MP_ROUGHNESS)
    MEL.recompile_material(m); EAL.save_loaded_asset(m); LOG.append('M_FxDust ok')
except Exception as e: LOG.append('M_FxDust ERREUR %s'%e)
open(r'C:\Users\Antoine\Documents\NewKarterz_3D\fx_mats.txt','w',encoding='utf-8').write('\n'.join(LOG))

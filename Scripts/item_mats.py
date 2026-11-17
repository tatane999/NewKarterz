# Matériaux des objets : boîte « verre irisé » (M_ItemBox), cœur lumineux (M_ItemCore), objets vernis (M_ItemGloss).
import unreal
EAL=unreal.EditorAssetLibrary; MEL=unreal.MaterialEditingLibrary; AT=unreal.AssetToolsHelpers.get_asset_tools(); LOG=[]
def E(m, cls, x, y): return MEL.create_material_expression(m, cls, x, y)
def C(a,ao,b,bi): MEL.connect_material_expressions(a,ao,b,bi)
def new_mat(name):
    p='/Game/NK/Mat/'+name
    if EAL.does_asset_exist(p): EAL.delete_asset(p)
    m=AT.create_asset(name,'/Game/NK/Mat',unreal.Material,unreal.MaterialFactoryNew()); m.set_editor_property('used_with_instanced_static_meshes', True); return m
def vp(m,n,c,x,y):
    e=E(m, unreal.MaterialExpressionVectorParameter,x,y); e.set_editor_property('parameter_name',n); e.set_editor_property('default_value',unreal.LinearColor(*c)); return e
DO=['M_ItemGloss']
try:
    if 'M_ItemBox' not in DO: raise RuntimeError('ignoré')
    m=new_mat('M_ItemBox'); m.set_editor_property('blend_mode', unreal.BlendMode.BLEND_TRANSLUCENT); m.set_editor_property('two_sided', True)
    try: m.set_editor_property('translucency_lighting_mode', unreal.TranslucencyLightingMode.TLM_SURFACE_FORWARD_SHADING)
    except Exception as e: LOG.append('tlm %s'%e)
    col=vp(m,'Color',(0.3,0.6,1,1),-900,0)
    fr=E(m, unreal.MaterialExpressionFresnel,-900,250); fr.set_editor_property('exponent',2.5)
    em=E(m, unreal.MaterialExpressionMultiply,-650,150); C(col,'',em,'A'); C(fr,'',em,'B')
    k=E(m, unreal.MaterialExpressionMultiply,-450,150); k.set_editor_property('const_b',6.0); C(em,'',k,'A')
    MEL.connect_material_property(k,'',unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    MEL.connect_material_property(col,'',unreal.MaterialProperty.MP_BASE_COLOR)
    op=E(m, unreal.MaterialExpressionLinearInterpolate,-450,350); op.set_editor_property('const_a',0.28); op.set_editor_property('const_b',0.85); C(fr,'',op,'Alpha')
    MEL.connect_material_property(op,'',unreal.MaterialProperty.MP_OPACITY)
    r=E(m, unreal.MaterialExpressionConstant,-450,500); r.set_editor_property('r',0.05); MEL.connect_material_property(r,'',unreal.MaterialProperty.MP_ROUGHNESS)
    sp=E(m, unreal.MaterialExpressionConstant,-450,600); sp.set_editor_property('r',1.0); MEL.connect_material_property(sp,'',unreal.MaterialProperty.MP_SPECULAR)
    MEL.recompile_material(m); EAL.save_loaded_asset(m); LOG.append('M_ItemBox ok')
except Exception as e: LOG.append('M_ItemBox ERREUR %s'%e)
try:
    if 'M_ItemCore' not in DO: raise RuntimeError('ignoré')
    m=new_mat('M_ItemCore'); m.set_editor_property('shading_model', unreal.MaterialShadingModel.MSM_UNLIT)
    col=vp(m,'Color',(6,6,6,1),-500,0); MEL.connect_material_property(col,'',unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    MEL.recompile_material(m); EAL.save_loaded_asset(m); LOG.append('M_ItemCore ok')
except Exception as e: LOG.append('M_ItemCore ERREUR %s'%e)
try:
    m=new_mat('M_ItemGloss');     col=vp(m,'Color',(1,1,1,1),-600,0); MEL.connect_material_property(col,'',unreal.MaterialProperty.MP_BASE_COLOR)
    r=E(m, unreal.MaterialExpressionConstant,-600,200); r.set_editor_property('r',0.18); MEL.connect_material_property(r,'',unreal.MaterialProperty.MP_ROUGHNESS)
    sp=E(m, unreal.MaterialExpressionConstant,-600,300); sp.set_editor_property('r',0.8); MEL.connect_material_property(sp,'',unreal.MaterialProperty.MP_SPECULAR)
    MEL.recompile_material(m); EAL.save_loaded_asset(m); LOG.append('M_ItemGloss ok')
except Exception as e: LOG.append('M_ItemGloss ERREUR %s'%e)
open(r'C:\Users\Antoine\Documents\NewKarterz_3D\item_mats.txt','w',encoding='utf-8').write('\n'.join(LOG))

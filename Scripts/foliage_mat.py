import unreal
EAL=unreal.EditorAssetLibrary;MEL=unreal.MaterialEditingLibrary;AT=unreal.AssetToolsHelpers.get_asset_tools();out=[]
path='/Game/NK/Mat/M_Foliage'
if EAL.does_asset_exist(path): EAL.delete_asset(path)
m=AT.create_asset('M_Foliage','/Game/NK/Mat',unreal.Material,unreal.MaterialFactoryNew())
m.set_editor_property('blend_mode',unreal.BlendMode.BLEND_MASKED)
m.set_editor_property('two_sided',True)
m.set_editor_property('opacity_mask_clip_value',0.4)
m.set_editor_property('shading_model',unreal.MaterialShadingModel.MSM_TWO_SIDED_FOLIAGE)
bc=MEL.create_material_expression(m,unreal.MaterialExpressionTextureSampleParameter2D,-600,0)
bc.set_editor_property('parameter_name','BaseColorTexture')
nt=MEL.create_material_expression(m,unreal.MaterialExpressionTextureSampleParameter2D,-600,300)
nt.set_editor_property('parameter_name','NormalTexture');nt.set_editor_property('sampler_type',unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL)
tint=MEL.create_material_expression(m,unreal.MaterialExpressionVectorParameter,-600,-200)
tint.set_editor_property('parameter_name','Tint');tint.set_editor_property('default_value',unreal.LinearColor(1,1,1,1))
mul=MEL.create_material_expression(m,unreal.MaterialExpressionMultiply,-300,0)
MEL.connect_material_expressions(bc,'RGB',mul,'A');MEL.connect_material_expressions(tint,'',mul,'B')
MEL.connect_material_property(mul,'',unreal.MaterialProperty.MP_BASE_COLOR)
MEL.connect_material_property(bc,'A',unreal.MaterialProperty.MP_OPACITY_MASK)
MEL.connect_material_property(nt,'RGB',unreal.MaterialProperty.MP_NORMAL)
sub=MEL.create_material_expression(m,unreal.MaterialExpressionMultiply,-300,500)
sub.set_editor_property('const_b',0.35)
MEL.connect_material_expressions(mul,'',sub,'A')
MEL.connect_material_property(sub,'',unreal.MaterialProperty.MP_SUBSURFACE_COLOR)
rc=MEL.create_material_expression(m,unreal.MaterialExpressionConstant,-300,200);rc.set_editor_property('r',0.8)
MEL.connect_material_property(rc,'',unreal.MaterialProperty.MP_ROUGHNESS)
MEL.recompile_material(m);EAL.save_loaded_asset(m)
for a in EAL.list_assets('/Game/NK/PH',True,False):
    o=unreal.load_asset(a)
    if not isinstance(o,unreal.MaterialInstanceConstant): continue
    n=o.get_name().lower()
    if not any(k in n for k in ['twig','leaf','leaves']): continue
    texs={t.parameter_info.name:t.parameter_value for t in o.get_editor_property('texture_parameter_values')}
    ov=o.get_editor_property('base_property_overrides');ov.set_editor_property('override_blend_mode',False);ov.set_editor_property('override_two_sided',False);o.set_editor_property('base_property_overrides',ov)
    o.set_editor_property('parent',m)
    for k in ['BaseColorTexture','NormalTexture']:
        if k in texs and texs[k]: MEL.set_material_instance_texture_parameter_value(o,k,texs[k])
    MEL.update_material_instance(o);EAL.save_loaded_asset(o);out.append('foliage '+o.get_name())
open(r'C:\Users\Antoine\Documents\NewKarterz_3D\foliage.txt','w').write('\n'.join(out))

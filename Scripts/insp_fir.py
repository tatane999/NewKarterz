import unreal
out=[]
m=unreal.load_asset('/Game/NK/PH/ph_fir')
out.append('nanite %s'%m.get_editor_property('nanite_settings').enabled)
for i,sm in enumerate(m.get_editor_property('static_materials')):
    mi=sm.material_interface
    out.append('%d %s %s'%(i,sm.material_slot_name,mi.get_path_name() if mi else None))
    if isinstance(mi,unreal.MaterialInstanceConstant):
        p=mi.get_editor_property('parent');out.append('   parent %s'%p.get_path_name())
        for t in mi.get_editor_property('texture_parameter_values'):
            out.append('   tex %s %s'%(t.parameter_info.name,t.parameter_value.get_name() if t.parameter_value else None))
        for s in mi.get_editor_property('scalar_parameter_values'):
            out.append('   sc %s %s'%(s.parameter_info.name,s.parameter_value))
out.append('lod0 tris %d'%m.get_num_triangles(0))
open(r'C:\Users\Antoine\Documents\NewKarterz_3D\insp_fir.txt','w').write('\n'.join(out))

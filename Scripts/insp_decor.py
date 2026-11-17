import unreal
out=[]
for i in range(25):
    m=unreal.load_asset('/Game/NK/Decor/t1_i%d'%i)
    if not m: out.append('i%d none'%i);continue
    b=m.get_bounds()
    mats=[str(s.material_interface.get_name()) if s.material_interface else 'None' for s in m.get_editor_property('static_materials')]
    out.append('i%d tris=%d ext=(%.0f,%.0f,%.0f) org=(%.0f,%.0f,%.0f) %s'%(i,m.get_num_triangles(0),b.box_extent.x,b.box_extent.y,b.box_extent.z,b.origin.x,b.origin.y,b.origin.z,mats))
open(r'C:\Users\Antoine\Documents\NewKarterz_3D\insp_decor.txt','w').write('\n'.join(out))

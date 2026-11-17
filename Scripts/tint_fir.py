import unreal
mi=unreal.load_asset('/Game/NK/PH/src_ph_fir/ph_fir/Materials/fir_tree_01_twig')
unreal.MaterialEditingLibrary.set_material_instance_vector_parameter_value(mi,'Tint',unreal.LinearColor(1.7,2.0,1.4,1))
unreal.MaterialEditingLibrary.update_material_instance(mi);unreal.EditorAssetLibrary.save_loaded_asset(mi)
open(r'C:\Users\Antoine\Documents\NewKarterz_3D\tint.txt','w').write('ok')

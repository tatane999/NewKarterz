# Script Python Unreal : importe les textures/karts, crée les matériaux et la carte.
import unreal, os, json
AT = unreal.AssetToolsHelpers.get_asset_tools()
MEL = unreal.MaterialEditingLibrary
EAL = unreal.EditorAssetLibrary
ROOT = r'C:\Users\Antoine\Documents\NewKarterz_3D'
LOG = []
def log(*a):
    LOG.append(' '.join(str(x) for x in a)); unreal.log(' '.join(str(x) for x in a))

def imp(files, dest, name=None):
    tasks=[]
    for f in files:
        t=unreal.AssetImportTask(); t.filename=f; t.destination_path=dest; t.automated=True; t.replace_existing=True; t.save=True
        if name: t.destination_name=name
        tasks.append(t)
    AT.import_asset_tasks(tasks)
    out=[]
    for t in tasks: out+=list(t.imported_object_paths or [])
    return out

# ---------- textures
TEX = {}
for f in sorted(os.listdir(ROOT+r'\pbr2k')):
    if not f.endswith(('_d.jpg','_n.jpg','_r.jpg')): continue
    nm='T_'+f[:-4]; p='/Game/NK/Tex/'+nm
    if not EAL.does_asset_exist(p): imp([ROOT+r'\pbr2k\\'+f], '/Game/NK/Tex', nm)
    t=unreal.load_asset(p)
    if t is None: log('tex fail',f); continue
    kind=f[-5]
    if kind=='n': t.set_editor_property('compression_settings', unreal.TextureCompressionSettings.TC_NORMALMAP); t.set_editor_property('srgb', False)
    elif kind=='r': t.set_editor_property('compression_settings', unreal.TextureCompressionSettings.TC_MASKS); t.set_editor_property('srgb', False)
    EAL.save_loaded_asset(t); TEX[f[:-4]]=t
wn='/Game/NK/Tex/T_WaterN'
if not EAL.does_asset_exist(wn): imp([ROOT+r'\ue_import\T_WaterN.png'],'/Game/NK/Tex','T_WaterN')
w=unreal.load_asset(wn); w.set_editor_property('compression_settings', unreal.TextureCompressionSettings.TC_NORMALMAP); w.set_editor_property('srgb', False); EAL.save_loaded_asset(w)
log('textures', len(TEX))

# ---------- karts
for i in range(13):
    p='/Game/NK/Karts/k%d'%i
    if EAL.does_asset_exist(p): continue
    got=imp([ROOT+r'\ue_import\karts\k%d.glb'%i], '/Game/NK/Karts/src%d'%i)
    sm=[g for g in got if isinstance(unreal.load_asset(g), unreal.StaticMesh)]
    log('kart',i,got[:6])
    if sm:
        EAL.duplicate_asset(sm[0].split('.')[0], p) if False else EAL.rename_asset(sm[0].split('.')[0], p)

# ---------- matériaux
def new_mat(name, two_sided=False):
    p='/Game/NK/Mat/'+name
    if EAL.does_asset_exist(p): EAL.delete_asset(p)
    m=AT.create_asset(name,'/Game/NK/Mat',unreal.Material,unreal.MaterialFactoryNew())
    m.set_editor_property('two_sided', two_sided); return m
def E(m, cls, x, y): return MEL.create_material_expression(m, cls, x, y)
def tparam(m, name, tex, x, y, normal=False, mask=False, uv=None):
    e=E(m, unreal.MaterialExpressionTextureSampleParameter2D, x, y); e.set_editor_property('parameter_name', name); e.set_editor_property('texture', tex)
    if normal: e.set_editor_property('sampler_type', unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL)
    elif mask: e.set_editor_property('sampler_type', unreal.MaterialSamplerType.SAMPLERTYPE_MASKS)
    if uv: MEL.connect_material_expressions(uv,'',e,'UVs')
    return e
def sparam(m, name, v, x, y):
    e=E(m, unreal.MaterialExpressionScalarParameter, x, y); e.set_editor_property('parameter_name', name); e.set_editor_property('default_value', v); return e
def vparam(m, name, c, x, y):
    e=E(m, unreal.MaterialExpressionVectorParameter, x, y); e.set_editor_property('parameter_name', name); e.set_editor_property('default_value', unreal.LinearColor(*c)); return e
def T(n): return unreal.load_asset('/Game/NK/Tex/T_'+n)

# M_Road : textures sur UV + teinte
m=new_mat('M_Road', True)
uvs=E(m, unreal.MaterialExpressionTextureCoordinate, -900, 0)
sc=sparam(m,'UVScale',1.0,-900,120); mul=E(m, unreal.MaterialExpressionMultiply,-700,40); MEL.connect_material_expressions(uvs,'',mul,'A'); MEL.connect_material_expressions(sc,'',mul,'B')
d=tparam(m,'D',T('asphalt_02_d'),-450,-200,uv=mul); n=tparam(m,'N',T('asphalt_02_n'),-450,100,normal=True,uv=mul); r=tparam(m,'R',T('asphalt_02_r'),-450,400,mask=True,uv=mul)
tint=vparam(m,'Tint',(1,1,1,1),-450,-380); mt=E(m, unreal.MaterialExpressionMultiply,-200,-250); MEL.connect_material_expressions(d,'RGB',mt,'A'); MEL.connect_material_expressions(tint,'',mt,'B')
rs=sparam(m,'RoughMul',1.0,-450,600); rm=E(m, unreal.MaterialExpressionMultiply,-200,450); MEL.connect_material_expressions(r,'G',rm,'A'); MEL.connect_material_expressions(rs,'',rm,'B')
MEL.connect_material_property(mt,'',unreal.MaterialProperty.MP_BASE_COLOR); MEL.connect_material_property(n,'RGB',unreal.MaterialProperty.MP_NORMAL); MEL.connect_material_property(rm,'',unreal.MaterialProperty.MP_ROUGHNESS)
MEL.recompile_material(m); EAL.save_loaded_asset(m)

# M_Terrain : 3 couches mélangées par la pente et un bruit, UV monde
m=new_mat('M_Terrain', True)
wp=E(m, unreal.MaterialExpressionWorldPosition,-1500,0); msk=E(m, unreal.MaterialExpressionComponentMask,-1300,0); msk.set_editor_property('r',True); msk.set_editor_property('g',True); msk.set_editor_property('b',False); MEL.connect_material_expressions(wp,'',msk,'')
tile=sparam(m,'Tile',700.0,-1300,150); uvA=E(m, unreal.MaterialExpressionDivide,-1100,40); MEL.connect_material_expressions(msk,'',uvA,'A'); MEL.connect_material_expressions(tile,'',uvA,'B')
kB=sparam(m,'ScaleB',0.6,-1100,200); uvB=E(m, unreal.MaterialExpressionMultiply,-900,200); MEL.connect_material_expressions(uvA,'',uvB,'A'); MEL.connect_material_expressions(kB,'',uvB,'B')
kC=sparam(m,'ScaleC',0.35,-1100,320); uvC=E(m, unreal.MaterialExpressionMultiply,-900,320); MEL.connect_material_expressions(uvA,'',uvC,'A'); MEL.connect_material_expressions(kC,'',uvC,'B')
Ad=tparam(m,'A_D',T('coast_sand_01_d'),-650,-600,uv=uvA); An=tparam(m,'A_N',T('coast_sand_01_n'),-650,-350,normal=True,uv=uvA)
Bd=tparam(m,'B_D',T('sparse_grass_d'),-650,-100,uv=uvB); Bn=tparam(m,'B_N',T('sparse_grass_n'),-650,150,normal=True,uv=uvB)
Cd=tparam(m,'C_D',T('rock_boulder_dry_d'),-650,400,uv=uvC); Cn=tparam(m,'C_N',T('rock_boulder_dry_n'),-650,650,normal=True,uv=uvC)
# pente
vn=E(m, unreal.MaterialExpressionVertexNormalWS,-1100,800); vz=E(m, unreal.MaterialExpressionComponentMask,-900,800); vz.set_editor_property('r',False); vz.set_editor_property('g',False); vz.set_editor_property('b',True); MEL.connect_material_expressions(vn,'',vz,'')
om=E(m, unreal.MaterialExpressionOneMinus,-750,800); MEL.connect_material_expressions(vz,'',om,'')
ss=E(m, unreal.MaterialExpressionSmoothStep,-550,800); s0=sparam(m,'SlopeMin',0.2,-750,900); s1=sparam(m,'SlopeMax',0.45,-750,980)
MEL.connect_material_expressions(s0,'',ss,'Min'); MEL.connect_material_expressions(s1,'',ss,'Max'); MEL.connect_material_expressions(om,'',ss,'Value')
# bruit pour la couche B
nz=E(m, unreal.MaterialExpressionNoise,-750,1100); nz.set_editor_property('scale',0.0012); nz.set_editor_property('levels',4); nz.set_editor_property('output_min',0.0); nz.set_editor_property('output_max',1.0)
MEL.connect_material_expressions(wp,'',nz,'Position')
nb=E(m, unreal.MaterialExpressionSmoothStep,-550,1100); b0=sparam(m,'BMin',0.48,-750,1250); b1=sparam(m,'BMax',0.66,-750,1330)
MEL.connect_material_expressions(b0,'',nb,'Min'); MEL.connect_material_expressions(b1,'',nb,'Max'); MEL.connect_material_expressions(nz,'',nb,'Value')
def lerp3(a,b,c,x,y):
    l1=E(m, unreal.MaterialExpressionLinearInterpolate,x,y); MEL.connect_material_expressions(a,'RGB',l1,'A'); MEL.connect_material_expressions(b,'RGB',l1,'B'); MEL.connect_material_expressions(nb,'',l1,'Alpha')
    l2=E(m, unreal.MaterialExpressionLinearInterpolate,x+200,y); MEL.connect_material_expressions(l1,'',l2,'A'); MEL.connect_material_expressions(c,'RGB',l2,'B'); MEL.connect_material_expressions(ss,'',l2,'Alpha'); return l2
col=lerp3(Ad,Bd,Cd,-250,-200); nor=lerp3(An,Bn,Cn,-250,300)
tint=vparam(m,'Tint',(1,1,1,1),-250,-400); ct=E(m, unreal.MaterialExpressionMultiply,100,-250); MEL.connect_material_expressions(col,'',ct,'A'); MEL.connect_material_expressions(tint,'',ct,'B')
# variation macro
nz2=E(m, unreal.MaterialExpressionNoise,-250,-600); nz2.set_editor_property('scale',0.0003); nz2.set_editor_property('output_min',0.82); nz2.set_editor_property('output_max',1.12); MEL.connect_material_expressions(wp,'',nz2,'Position')
cm=E(m, unreal.MaterialExpressionMultiply,300,-300); MEL.connect_material_expressions(ct,'',cm,'A'); MEL.connect_material_expressions(nz2,'',cm,'B')
MEL.connect_material_property(cm,'',unreal.MaterialProperty.MP_BASE_COLOR); MEL.connect_material_property(nor,'',unreal.MaterialProperty.MP_NORMAL)
ro=sparam(m,'Rough',0.9,100,500); MEL.connect_material_property(ro,'',unreal.MaterialProperty.MP_ROUGHNESS)
MEL.recompile_material(m); EAL.save_loaded_asset(m)

# M_Curb / M_Barrier : couleur de sommet
for nm,rv in (('M_Curb',0.55),('M_Barrier',0.6)):
    m=new_mat(nm, True); vc=E(m, unreal.MaterialExpressionVertexColor,-400,0); MEL.connect_material_property(vc,'RGB',unreal.MaterialProperty.MP_BASE_COLOR)
    ro=sparam(m,'Rough',rv,-400,200); MEL.connect_material_property(ro,'',unreal.MaterialProperty.MP_ROUGHNESS); MEL.recompile_material(m); EAL.save_loaded_asset(m)

# M_Flame
m=new_mat('M_Flame'); m.set_editor_property('shading_model', unreal.MaterialShadingModel.MSM_UNLIT); m.set_editor_property('blend_mode', unreal.BlendMode.BLEND_ADDITIVE)
c=vparam(m,'Color',(8.0,2.4,0.4,1),-400,0); MEL.connect_material_property(c,'',unreal.MaterialProperty.MP_EMISSIVE_COLOR); MEL.recompile_material(m); EAL.save_loaded_asset(m)

# M_Water : eau à couche unique (Lumen)
m=new_mat('M_Water'); m.set_editor_property('shading_model', unreal.MaterialShadingModel.MSM_SINGLE_LAYER_WATER)
wp=E(m, unreal.MaterialExpressionWorldPosition,-1200,0); msk=E(m, unreal.MaterialExpressionComponentMask,-1000,0); msk.set_editor_property('b',False); MEL.connect_material_expressions(wp,'',msk,'')
dv=E(m, unreal.MaterialExpressionDivide,-850,0); dv.set_editor_property('const_b',900.0); MEL.connect_material_expressions(msk,'',dv,'A')
p1=E(m, unreal.MaterialExpressionPanner,-650,-100); p1.set_editor_property('speed_x',0.02); p1.set_editor_property('speed_y',0.012); MEL.connect_material_expressions(dv,'',p1,'Coordinate')
dv2=E(m, unreal.MaterialExpressionMultiply,-850,200); dv2.set_editor_property('const_b',0.63); MEL.connect_material_expressions(dv,'',dv2,'A')
p2=E(m, unreal.MaterialExpressionPanner,-650,200); p2.set_editor_property('speed_x',-0.015); p2.set_editor_property('speed_y',0.02); MEL.connect_material_expressions(dv2,'',p2,'Coordinate')
n1=tparam(m,'N1',unreal.load_asset(wn),-400,-100,normal=True,uv=p1); n2=tparam(m,'N2',unreal.load_asset(wn),-400,200,normal=True,uv=p2)
ad=E(m, unreal.MaterialExpressionAdd,-150,50); MEL.connect_material_expressions(n1,'RGB',ad,'A'); MEL.connect_material_expressions(n2,'RGB',ad,'B')
fl=E(m, unreal.MaterialExpressionMultiply,50,50); MEL.connect_material_expressions(ad,'',fl,'A'); fl.set_editor_property('const_b',0.5)
MEL.connect_material_property(fl,'',unreal.MaterialProperty.MP_NORMAL)
bc=vparam(m,'Color',(0.02,0.35,0.42,1),-150,-250); MEL.connect_material_property(bc,'',unreal.MaterialProperty.MP_BASE_COLOR)
ro=sparam(m,'Rough',0.04,-150,350); MEL.connect_material_property(ro,'',unreal.MaterialProperty.MP_ROUGHNESS)
sp=sparam(m,'Spec',0.6,-150,450); MEL.connect_material_property(sp,'',unreal.MaterialProperty.MP_SPECULAR)
MEL.recompile_material(m); EAL.save_loaded_asset(m)

# instances par biome
BIOME = {
 'palm': dict(road=('asphalt_02',(1,1,1)), dirt='dirt', A='coast_sand_01', B='sparse_grass', C='rock_boulder_dry', tint=(1.05,1.0,0.92), b=(0.48,0.66)),
 'pine': dict(road=('asphalt_02',(0.95,0.95,0.95)), dirt='dirt', A='leafy_grass', B='forest_ground_04', C='mossy_rock', tint=(0.92,1.1,0.86), b=(0.55,0.75)),
 'rock': dict(road=('asphalt_02',(0.45,0.42,0.42)), dirt='rocks_ground_05', A='burned_ground_01', B='rocks_ground_05', C='rock_face', tint=(0.9,0.84,0.82), b=(0.48,0.66)),
 'snow': dict(road=('asphalt_snow',(1,1,1)), dirt='snow_03', A='snow_02', B='snow_03', C='rock_face', tint=(1.08,1.1,1.14), b=(0.62,0.85)),
}
def mi(name, parent):
    p='/Game/NK/Mat/'+name
    if EAL.does_asset_exist(p): EAL.delete_asset(p)
    i=AT.create_asset(name,'/Game/NK/Mat',unreal.MaterialInstanceConstant,unreal.MaterialInstanceConstantFactoryNew())
    MEL.set_material_instance_parent(i, unreal.load_asset('/Game/NK/Mat/'+parent)); return i
def st(i,k,t): MEL.set_material_instance_texture_parameter_value(i,k,T(t))
def ss_(i,k,v): MEL.set_material_instance_scalar_parameter_value(i,k,v)
def sv(i,k,c): MEL.set_material_instance_vector_parameter_value(i,k,unreal.LinearColor(c[0],c[1],c[2],1))
for b,c in BIOME.items():
    i=mi('MI_Road_'+b,'M_Road'); st(i,'D',c['road'][0]+'_d'); st(i,'N',c['road'][0]+'_n'); st(i,'R',c['road'][0]+'_r'); sv(i,'Tint',c['road'][1]); ss_(i,'UVScale',1.0); EAL.save_loaded_asset(i)
    i=mi('MI_Dirt_'+b,'M_Road'); st(i,'D',c['dirt']+'_d'); st(i,'N',c['dirt']+'_n'); st(i,'R',c['dirt']+'_r'); ss_(i,'UVScale',1.0); EAL.save_loaded_asset(i)
    i=mi('MI_Terrain_'+b,'M_Terrain')
    for L in 'ABC': st(i,L+'_D',c[L]+'_d'); st(i,L+'_N',c[L]+'_n')
    sv(i,'Tint',c['tint']); ss_(i,'BMin',c['b'][0]); ss_(i,'BMax',c['b'][1]); EAL.save_loaded_asset(i)

# carte vide
if not EAL.does_asset_exist('/Game/Maps/Race'):
    ok=EAL.duplicate_asset('/Engine/Maps/Entry','/Game/Maps/Race'); log('map dup',ok)
EAL.save_directory('/Game/NK')
open(ROOT+r'\ue_setup_log.txt','w',encoding='utf-8').write('\n'.join(LOG))

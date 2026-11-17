# Refonte graphique v2 : matériau de décor à couleur par instance + terrain sans répétition visible.
import unreal
EAL=unreal.EditorAssetLibrary; MEL=unreal.MaterialEditingLibrary; AT=unreal.AssetToolsHelpers.get_asset_tools()
LOG=[]
def log(*a): LOG.append(' '.join(str(x) for x in a))
def new_mat(name, two_sided=False):
    p='/Game/NK/Mat/'+name
    if EAL.does_asset_exist(p): EAL.delete_asset(p)
    m=AT.create_asset(name,'/Game/NK/Mat',unreal.Material,unreal.MaterialFactoryNew())
    m.set_editor_property('two_sided', two_sided); return m
def E(m, cls, x, y): return MEL.create_material_expression(m, cls, x, y)
def C(a,ao,b,bi): MEL.connect_material_expressions(a,ao,b,bi)
def sparam(m, name, v, x, y):
    e=E(m, unreal.MaterialExpressionScalarParameter, x, y); e.set_editor_property('parameter_name', name); e.set_editor_property('default_value', v); return e
def vparam(m, name, c, x, y):
    e=E(m, unreal.MaterialExpressionVectorParameter, x, y); e.set_editor_property('parameter_name', name); e.set_editor_property('default_value', unreal.LinearColor(*c)); return e
def T(n): return unreal.load_asset('/Game/NK/Tex/T_'+n)
def mul(m,a,b,x,y,ao='',bo=''):
    e=E(m, unreal.MaterialExpressionMultiply,x,y); C(a,ao,e,'A'); C(b,bo,e,'B'); return e
def lerp(m,a,b,al,x,y,ao='',bo='',alo=''):
    e=E(m, unreal.MaterialExpressionLinearInterpolate,x,y); C(a,ao,e,'A'); C(b,bo,e,'B'); C(al,alo,e,'Alpha'); return e

DO_DECOR = not EAL.does_asset_exist('/Game/NK/Mat/M_DecorInst')
# ---------- M_DecorInst : couleur = Base × couleur de sommet × couleur d'instance (PerInstanceCustomData 0..2) ----------
try:
    if not DO_DECOR: raise RuntimeError('déjà présent')
    m=new_mat('M_DecorInst', True)
    base=vparam(m,'Base',(1,1,1,1),-900,-200)
    vc=E(m, unreal.MaterialExpressionVertexColor,-900,0)
    try:
        pc=E(m, unreal.MaterialExpressionPerInstanceCustomData3Vector,-900,200); pc.set_editor_property('data_index',0)
        pc.set_editor_property('default_value', unreal.LinearColor(1,1,1,1))
    except Exception as e:
        log('3vector indisponible',e)
        r=E(m, unreal.MaterialExpressionPerInstanceCustomData,-1100,150); r.set_editor_property('data_index',0); r.set_editor_property('const_default_value',1.0)
        g=E(m, unreal.MaterialExpressionPerInstanceCustomData,-1100,250); g.set_editor_property('data_index',1); g.set_editor_property('const_default_value',1.0)
        b=E(m, unreal.MaterialExpressionPerInstanceCustomData,-1100,350); b.set_editor_property('data_index',2); b.set_editor_property('const_default_value',1.0)
        ap=E(m, unreal.MaterialExpressionAppendVector,-950,200); C(r,'',ap,'A'); C(g,'',ap,'B')
        pc=E(m, unreal.MaterialExpressionAppendVector,-850,250); C(ap,'',pc,'A'); C(b,'',pc,'B')
    a1=mul(m,base,vc,-650,-100,'','RGB'); a2=mul(m,a1,pc,-450,0)
    # légère variation aléatoire par instance
    pr=E(m, unreal.MaterialExpressionPerInstanceRandom,-650,250)
    lr=E(m, unreal.MaterialExpressionLinearInterpolate,-450,250); lr.set_editor_property('const_a',0.9); lr.set_editor_property('const_b',1.08); C(pr,'',lr,'Alpha')
    a3=mul(m,a2,lr,-250,50)
    MEL.connect_material_property(a3,'',unreal.MaterialProperty.MP_BASE_COLOR)
    ro=sparam(m,'Rough',0.72,-250,250); MEL.connect_material_property(ro,'',unreal.MaterialProperty.MP_ROUGHNESS)
    MEL.recompile_material(m); EAL.save_loaded_asset(m); log('M_DecorInst ok')
except Exception as e: log('M_DecorInst ERREUR',e)

# ---------- M_Terrain2 : 3 couches, chacune échantillonnée à 2 échelles (anti-répétition), mélanges larges ----------
try:
    m=new_mat('M_Terrain2', True)
    wp=E(m, unreal.MaterialExpressionWorldPosition,-2200,0)
    msk=E(m, unreal.MaterialExpressionComponentMask,-2000,0); msk.set_editor_property('r',True); msk.set_editor_property('g',True); msk.set_editor_property('b',False); C(wp,'',msk,'')
    tile=sparam(m,'Tile',700.0,-2000,150); uv=E(m, unreal.MaterialExpressionDivide,-1800,40); C(msk,'',uv,'A'); C(tile,'',uv,'B')
    # distance à la caméra -> plus de grande échelle au loin
    pd=E(m, unreal.MaterialExpressionPixelDepth,-1800,-300)
    dd=E(m, unreal.MaterialExpressionDivide,-1650,-300); dd.set_editor_property('const_b',7000.0); C(pd,'',dd,'A')
    ds=E(m, unreal.MaterialExpressionSaturate,-1500,-300); C(dd,'',ds,'')
    far=E(m, unreal.MaterialExpressionLinearInterpolate,-1350,-300); far.set_editor_property('const_a',0.3); far.set_editor_property('const_b',0.75); C(ds,'',far,'Alpha')
    def layer(L, scale, x, y):
        k=E(m, unreal.MaterialExpressionMultiply,x-400,y); k.set_editor_property('const_b',scale); C(uv,'',k,'A')
        k2=E(m, unreal.MaterialExpressionMultiply,x-400,y+120); k2.set_editor_property('const_b',scale*0.23); C(uv,'',k2,'A')
        off=E(m, unreal.MaterialExpressionAdd,x-250,y+120); off.set_editor_property('const_b',0.37); C(k2,'',off,'A')
        outs=[]
        for kind,tex,normal in (('D',T('leafy_grass_d'),False),('N',T('leafy_grass_n'),True)):
            s1=E(m, unreal.MaterialExpressionTextureSampleParameter2D,x,y+(0 if kind=='D' else 260)); s1.set_editor_property('parameter_name',L+'_'+kind); s1.set_editor_property('texture',tex)
            s2=E(m, unreal.MaterialExpressionTextureSampleParameter2D,x,y+(130 if kind=='D' else 390)); s2.set_editor_property('parameter_name',L+'_'+kind); s2.set_editor_property('texture',tex)
            if normal:
                s1.set_editor_property('sampler_type',unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL); s2.set_editor_property('sampler_type',unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL)
            C(k,'',s1,'UVs'); C(off,'',s2,'UVs')
            outs.append(lerp(m,s1,s2,far,x+250,y+(0 if kind=='D' else 260),'RGB','RGB',''))
        return outs
    Ad,An=layer('A',1.0,-900,-900); Bd,Bn=layer('B',0.6,-900,-300); Cd,Cn=layer('C',0.35,-900,300)
    # pente
    vn=E(m, unreal.MaterialExpressionVertexNormalWS,-1300,900); vz=E(m, unreal.MaterialExpressionComponentMask,-1150,900); vz.set_editor_property('r',False); vz.set_editor_property('g',False); vz.set_editor_property('b',True); C(vn,'',vz,'')
    om=E(m, unreal.MaterialExpressionOneMinus,-1000,900); C(vz,'',om,'')
    ss=E(m, unreal.MaterialExpressionSmoothStep,-800,900); C(sparam(m,'SlopeMin',0.18,-1000,1000),'',ss,'Min'); C(sparam(m,'SlopeMax',0.5,-1000,1080),'',ss,'Max'); C(om,'',ss,'Value')
    # bruit large (zones), transitions douces
    nz=E(m, unreal.MaterialExpressionNoise,-1000,1200); nz.set_editor_property('scale',0.00045); nz.set_editor_property('levels',5); nz.set_editor_property('output_min',0.0); nz.set_editor_property('output_max',1.0); C(wp,'',nz,'Position')
    nb=E(m, unreal.MaterialExpressionSmoothStep,-800,1200); C(sparam(m,'BMin',0.4,-1000,1350),'',nb,'Min'); C(sparam(m,'BMax',0.8,-1000,1430),'',nb,'Max'); C(nz,'',nb,'Value')
    col=lerp(m,lerp(m,Ad,Bd,nb,-300,-700),Cd,ss,-100,-700)
    nor=lerp(m,lerp(m,An,Bn,nb,-300,300),Cn,ss,-100,300)
    tint=vparam(m,'Tint',(1,1,1,1),-100,-900); ct=mul(m,col,tint,100,-750)
    # variation macro de luminosité et de teinte
    nz2=E(m, unreal.MaterialExpressionNoise,-100,-1100); nz2.set_editor_property('scale',0.00018); nz2.set_editor_property('output_min',0.8); nz2.set_editor_property('output_max',1.15); C(wp,'',nz2,'Position')
    cm=mul(m,ct,nz2,300,-800)
    # adoucit un peu les normales (moins « bruité » au loin)
    up=E(m, unreal.MaterialExpressionConstant3Vector,100,400); up.set_editor_property('constant', unreal.LinearColor(0,0,1,1))
    fl=lerp(m,nor,up,sparam(m,'NormalFlat',0.25,100,500),300,300)
    MEL.connect_material_property(cm,'',unreal.MaterialProperty.MP_BASE_COLOR); MEL.connect_material_property(fl,'',unreal.MaterialProperty.MP_NORMAL)
    MEL.connect_material_property(sparam(m,'Rough',0.92,300,600),'',unreal.MaterialProperty.MP_ROUGHNESS)
    MEL.recompile_material(m); EAL.save_loaded_asset(m); log('M_Terrain2 ok')
    for b in ('palm','pine','rock','snow'):
        mi=unreal.load_asset('/Game/NK/Mat/MI_Terrain_'+b)
        if not mi: continue
        bmin=mi.get_scalar_parameter_value('BMin') if hasattr(mi,'get_scalar_parameter_value') else 0.5
        MEL.set_material_instance_parent(mi, m)
        MEL.set_material_instance_scalar_parameter_value(mi,'BMin',0.38); MEL.set_material_instance_scalar_parameter_value(mi,'BMax',0.78)
        MEL.update_material_instance(mi); EAL.save_loaded_asset(mi); log('reparent',b)
except Exception as e: log('M_Terrain2 ERREUR',e)
open(r'C:\Users\Antoine\Documents\NewKarterz_3D\graphics_v2.txt','w',encoding='utf-8').write('\n'.join(LOG))

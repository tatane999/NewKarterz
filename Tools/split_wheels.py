import bpy,bmesh,mathutils,json,os,sys
from mathutils import Vector
B=r'C:\Users\Antoine\Documents\NewKarterz_3D'
OUT=B+r'\ue_import\chars2'; os.makedirs(OUT,exist_ok=True)
names=sys.argv[sys.argv.index('--')+1].split(',')
LOG=open(B+r'\split_wheels_log.txt','a',encoding='utf-8')
meta={}
mp=B+r'\ue_import\chars2\wheels.json'
if os.path.exists(mp): meta=json.load(open(mp))
for n in names:
  try:
    bpy.ops.wm.read_factory_settings(use_empty=True)
    bpy.ops.import_scene.gltf(filepath=B+r'\ue_import\joined\%s.glb'%n)
    obs=[x for x in bpy.context.scene.objects if x.type=='MESH']
    o=obs[0]
    bpy.context.view_layer.objects.active=o
    for x in bpy.context.scene.objects: x.select_set(x==o)
    bpy.ops.object.transform_apply(location=True,rotation=True,scale=True)
    me=o.data
    mats=[m.name.lower() if m else '' for m in me.materials]
    tire={i for i,m in enumerate(mats) if 'tire' in m}
    polys=me.polygons
    tf=[p for p in polys if p.material_index in tire]
    if not tf: LOG.write('%s pas de pneus\n'%n); continue
    # regroupement spatial des faces de pneu
    kd=mathutils.kdtree.KDTree(len(tf))
    for i,p in enumerate(tf): kd.insert(p.center,i)
    kd.balance()
    lab=[-1]*len(tf); nc=0
    for i in range(len(tf)):
        if lab[i]>=0: continue
        st=[i]; lab[i]=nc
        while st:
            j=st.pop()
            for (co,k,d) in kd.find_range(tf[j].center,0.12):
                if lab[k]<0: lab[k]=nc; st.append(k)
        nc+=1
    cl=[]
    for c in range(nc):
        ps=[tf[i] for i in range(len(tf)) if lab[i]==c]
        if len(ps)<20: continue
        vs=[me.vertices[v].co for p in ps for v in p.vertices]
        mn=Vector([min(v[i] for v in vs) for i in range(3)]); mx=Vector([max(v[i] for v in vs) for i in range(3)])
        cl.append((mn,mx))
    # fusion des anneaux concentriques d'une même roue
    merged=True
    while merged:
        merged=False
        for i in range(len(cl)):
            for j in range(i+1,len(cl)):
                ci=(cl[i][0]+cl[i][1])/2; cj=(cl[j][0]+cl[j][1])/2
                if abs(ci.y-cj.y)<0.12 and abs(ci.z-cj.z)<0.12 and abs(ci.x-cj.x)<0.7:
                    a,b=cl[i],cl[j]; cl[i]=(Vector([min(a[0][k],b[0][k]) for k in range(3)]),Vector([max(a[1][k],b[1][k]) for k in range(3)])); cl.pop(j); merged=True; break
            if merged: break
    ok=[(mn,mx) for mn,mx in cl if (mx-mn).y>0.1 and abs((mx-mn).y-(mx-mn).z)/max((mx-mn).y,(mx-mn).z)<0.3]
    if len(ok)<3 or len(ok)>6:
        LOG.write('%s ignoré : %d groupes, %d ronds %s\n'%(n,len(cl),len(ok),[tuple(round(v,2) for v in (mx-mn)) for mn,mx in cl])); continue
    my=sum((mn.y+mx.y)/2 for mn,mx in ok)/len(ok)
    wheels=[]
    for mn,mx in ok:
        c=(mn+mx)/2; R=max((mx-mn).y,(mx-mn).z)/2
        wheels.append(dict(c=c,R=R,x0=mn.x,x1=mx.x,front=bool(c.y<my)))
    # attribution des faces : pneus + tout ce qui est dans le cylindre de la roue
    assign={}
    for p in polys:
        pc=p.center
        for wi,w in enumerate(wheels):
            if w['x0']-0.09<=pc.x<=w['x1']+0.09 and ((pc.y-w['c'].y)**2+(pc.z-w['c'].z)**2)**0.5<=w['R']*0.99:
                assign[p.index]=wi; break
    # séparation (bmesh)
    def part(keep):
        bm=bmesh.new(); bm.from_mesh(me); bm.faces.ensure_lookup_table()
        dele=[f for f in bm.faces if not keep(f.index)]
        bmesh.ops.delete(bm,geom=dele,context='FACES')
        lv=[v for v in bm.verts if not v.link_faces]; bmesh.ops.delete(bm,geom=lv,context='VERTS')
        return bm
    info=[]; base=o.data
    for wi,w in enumerate(wheels):
        bm=part(lambda i,wi=wi: assign.get(i)==wi)
        for v in bm.verts: v.co-=w['c']
        m2=bpy.data.meshes.new('%s_w%d'%(n,wi)); bm.to_mesh(m2); bm.free()
        for mt in base.materials: m2.materials.append(mt)
        wo=bpy.data.objects.new(m2.name,m2); bpy.context.scene.collection.objects.link(wo)
        for x in bpy.context.scene.objects: x.select_set(x==wo)
        bpy.context.view_layer.objects.active=wo
        bpy.ops.object.material_slot_remove_unused()
        bpy.ops.export_scene.gltf(filepath=OUT+'\\%s_w%d.glb'%(n,wi),export_format='GLB',use_selection=True)
        c=w['c']; info.append(dict(f=w['front'],p=[round(c.x*100,2),round(-c.y*100,2),round(c.z*100,2)],r=round(w['R']*100,2),tris=len(m2.polygons)))
        bpy.data.objects.remove(wo)
    bm=part(lambda i: i not in assign)
    bm.to_mesh(base); bm.free()
    for x in bpy.context.scene.objects: x.select_set(x==o)
    bpy.context.view_layer.objects.active=o
    bpy.ops.object.material_slot_remove_unused()
    bpy.ops.export_scene.gltf(filepath=OUT+'\\%s_body.glb'%n,export_format='GLB',use_selection=True)
    meta[n]=info
    LOG.write('%s ok %d roues %s\n'%(n,len(info),info))
  except Exception as e:
    LOG.write('%s ERREUR %s\n'%(n,e))
  LOG.flush()
  json.dump(meta,open(mp,'w'),indent=1)
LOG.write('FIN\n'); LOG.close()

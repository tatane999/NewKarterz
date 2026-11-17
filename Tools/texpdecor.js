const p=require('puppeteer-core');const fs=require('fs');
(async()=>{const b=await p.launch({executablePath:'/usr/bin/google-chrome',args:['--no-sandbox','--use-gl=swiftshader','--enable-unsafe-swiftshader'],protocolTimeout:1200000});const pg=await b.newPage();const L=[];pg.on('pageerror',e=>L.push(e.message));pg.on('console',m=>{const t=m.text();if(/counts|done|conv0/.test(t))console.log('PAGE',t)});
await pg.goto('http://localhost:8765/index.html');await new Promise(r=>setTimeout(r,6000));
fs.mkdirSync('/home/claude/ue/decor',{recursive:true});
const tr=+process.argv[2];
const r=await pg.evaluate(async(tr)=>{const {GLTFExporter}=await import('three/addons/exporters/GLTFExporter.js');
  selTrack=tr;wxSel='clear';curWx='clear';buildScene(tr);scene.updateMatrixWorld(true);
  const toB64=(buf)=>{const u8=new Uint8Array(buf);let s='';for(let k=0;k<u8.length;k+=0x8000)s+=String.fromCharCode.apply(null,u8.subarray(k,k+0x8000));return btoa(s)};
  const CC=new Map();const conv=(m)=>{if(m&&!Array.isArray(m)&&CC.has(m.uuid))return CC.get(m.uuid);const r=conv0(m);if(m&&!Array.isArray(m))CC.set(m.uuid,r);return r};const conv0=(m)=>{if(Array.isArray(m))return m.map(conv);if(!m)return m;let map=m.map||null;if(map){const im=map.image;if(!im||!(im.width>0&&im.height>0)||map.isCompressedTexture)map=null}const s=new THREE.MeshStandardMaterial({color:(m.color?m.color.clone():new THREE.Color(1,1,1)),map,roughness:m.roughness??0.8,metalness:m.metalness??0,side:m.side});
    if(m.emissive&&m.emissiveIntensity>0){s.emissive=m.emissive.clone();s.emissiveIntensity=m.emissiveIntensity}if(m.isMeshBasicMaterial){s.emissive=m.color.clone();s.emissiveIntensity=0.6}if(m.transparent&&m.opacity<0.6)return null;s.name=m.name||'';if(m.alphaTest>0||(m.transparent&&m.map)){s.alphaTest=Math.max(0.4,m.alphaTest||0);s.transparent=false}if(m.vertexColors)s.vertexColors=true;return s};
  const clean=(g0)=>{const g=g0.clone();for(const k of Object.keys(g.morphAttributes||{}))delete g.morphAttributes[k];for(const a of Object.keys(g.attributes))if(!['position','normal','uv','color'].includes(a))g.deleteAttribute(a);if(!g.attributes.normal)g.computeVertexNormals();return g};
  const big=(o)=>{const g=o.geometry;if(!g.boundingBox)g.computeBoundingBox();const s=new THREE.Vector3();g.boundingBox.getSize(s);s.multiply(o.getWorldScale(new THREE.Vector3()));return Math.max(s.x,s.z)>260};
  const inst={},statics=new THREE.Group();let nSt=0,nIn=0;
  scene.traverse(o=>{if(!o.isMesh||o===skyMesh)return;let q=o,vis=true;while(q){if(!q.visible){vis=false;break}q=q.parent}if(!vis)return;
    if(o.isPoints||o.isLine||o.isSprite)return;if(o.geometry===terrGeo)return;if(o.material&&(o.material.isShaderMaterial||o.material.isPointsMaterial))return;
    if(o.isInstancedMesh){if(!o.count)return;const mat=conv(o.material);if(!mat)return;const key=o.geometry.uuid+'_'+(Array.isArray(o.material)?o.material.map(m=>m.uuid).join(''):o.material.uuid);
      if(!inst[key])inst[key]={geo:clean(o.geometry),mat,M:[]};const mm=new THREE.Matrix4();for(let k=0;k<o.count;k++){o.getMatrixAt(k,mm);const w=new THREE.Matrix4().multiplyMatrices(o.matrixWorld,mm);inst[key].M.push(Array.from(w.elements).map(v=>Math.round(v*1e4)/1e4))}nIn+=o.count;return}
    if(big(o))return;const mat=conv(o.material);if(!mat)return;const g=clean(o.geometry);g.applyMatrix4(o.matrixWorld);statics.add(new THREE.Mesh(g,mat));nSt++});
  console.log('counts',nSt,nIn,Object.keys(inst).length,CC.size);const t0=performance.now();const ex=new GLTFExporter();let sb;try{sb=await ex.parseAsync(statics,{binary:true,maxTextureSize:1024})}catch(e){console.log('conv0 retry no maps '+e);statics.traverse(o=>{if(o.isMesh){const ms=Array.isArray(o.material)?o.material:[o.material];for(const mm of ms)if(mm)mm.map=null}});sb=await ex.parseAsync(statics,{binary:true})}const res={statics:toB64(sb),inst:[],nSt,nIn};console.log('statics done',performance.now()-t0);
  let i=0;for(const [k,v] of Object.entries(inst)){const g=new THREE.Group();g.add(new THREE.Mesh(v.geo,v.mat));let ib;try{ib=await ex.parseAsync(g,{binary:true,maxTextureSize:1024})}catch(e){v.mat.map=null;ib=await ex.parseAsync(g,{binary:true})}res.inst.push({id:'i'+i,b64:toB64(ib),M:v.M});i++}
  return res},tr);
fs.writeFileSync(`/home/claude/ue/decor/t${tr}_static.glb`,Buffer.from(r.statics,'base64'));
const meta=[];for(const it of r.inst){fs.writeFileSync(`/home/claude/ue/decor/t${tr}_${it.id}.glb`,Buffer.from(it.b64,'base64'));meta.push({id:it.id,M:it.M})}
fs.writeFileSync(`/home/claude/ue/decor/t${tr}_inst.json`,JSON.stringify(meta));
console.log(tr,'statics',r.nSt,Math.round(r.statics.length/1024)+'KB','instTypes',r.inst.length,'instances',r.nIn,L.slice(0,3));await b.close()})();

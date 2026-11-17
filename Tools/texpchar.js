const p=require('puppeteer-core');const fs=require('fs');
(async()=>{const b=await p.launch({executablePath:'/usr/bin/google-chrome',args:['--no-sandbox','--use-gl=swiftshader','--enable-unsafe-swiftshader'],protocolTimeout:600000});const pg=await b.newPage();const L=[];pg.on('pageerror',e=>L.push(e.message));
await pg.goto('http://localhost:8765/index.html');await new Promise(r=>setTimeout(r,6000));
const n=await pg.evaluate(()=>CHARS.length);console.log('chars',n);
fs.mkdirSync('/home/claude/ue/chars',{recursive:true});
const names=[];
for(let i=+(process.argv[2]||0);i<n;i++){
 const r=await pg.evaluate(async(i)=>{const {GLTFExporter}=await import('three/addons/exporters/GLTFExporter.js');
  const ch=CHARS[i];const km=makeKartMesh(ch,i%13);const root=km.root;root.position.set(0,0,0);root.rotation.set(0,0,0);root.updateMatrixWorld(true);
  const skip=new Set([...(km.flames||[]),...(km.sparks||[]),km.shield,km.shadow,km.sm].filter(Boolean));
  const out=new THREE.Group();const v=new THREE.Vector3();
  const conv=(m)=>{if(Array.isArray(m))return m.map(conv);if(!m)return m;if(m.isMeshStandardMaterial||m.isMeshBasicMaterial)return m;const s=new THREE.MeshStandardMaterial({color:m.color||0xffffff,map:m.map||null,roughness:m.roughness??0.6,metalness:m.metalness??0});if(m.emissive)s.emissive=m.emissive;return s};
  root.traverse(o=>{if(!o.isMesh)return;let vis=true;let q=o;while(q){if(!q.visible||skip.has(q)){vis=false;break}q=q.parent}if(!vis)return;
    let g=o.geometry.clone();for(const k of Object.keys(g.morphAttributes||{}))delete g.morphAttributes[k];
    if(o.isSkinnedMesh){o.skeleton.update();const pos=g.attributes.position;const np=new Float32Array(pos.count*3);for(let j=0;j<pos.count;j++){v.fromBufferAttribute(pos,j);o.applyBoneTransform(j,v);np[j*3]=v.x;np[j*3+1]=v.y;np[j*3+2]=v.z}
      g=new THREE.BufferGeometry();g.setAttribute('position',new THREE.BufferAttribute(np,3));for(const a of ['uv','normal'])if(o.geometry.attributes[a])g.setAttribute(a,o.geometry.attributes[a].clone());if(o.geometry.index)g.setIndex(o.geometry.index.clone());for(const gr of o.geometry.groups)g.addGroup(gr.start,gr.count,gr.materialIndex);g.applyMatrix4(o.bindMatrix);g.computeVertexNormals()}
    else g.applyMatrix4(o.matrixWorld);
    if(o.isSkinnedMesh)g.applyMatrix4(new THREE.Matrix4().copy(o.matrixWorld).multiply(new THREE.Matrix4().copy(o.bindMatrix).invert()));
    for(const a of Object.keys(g.attributes))if(!['position','normal','uv'].includes(a))g.deleteAttribute(a);
    if(o.isInstancedMesh){for(let k=0;k<o.count;k++){const mm=new THREE.Matrix4();o.getMatrixAt(k,mm);const gg=g.clone();gg.applyMatrix4(mm);out.add(new THREE.Mesh(gg,conv(o.material)))}return}
    out.add(new THREE.Mesh(g,conv(o.material)))});
  const ex=new GLTFExporter();const buf=await ex.parseAsync(out,{binary:true,maxTextureSize:512});const u8=new Uint8Array(buf);let s='';for(let k=0;k<u8.length;k+=0x8000)s+=String.fromCharCode.apply(null,u8.subarray(k,k+0x8000));
  return {name:ch.name,b64:btoa(s),n:out.children.length}},i);
 fs.writeFileSync(`/home/claude/ue/chars/c${i}.glb`,Buffer.from(r.b64,'base64'));names.push(r.name);console.log(i,r.name,r.n,Math.round(r.b64.length/1024)+'KB')}
fs.writeFileSync('/home/claude/ue/chars/names.json',JSON.stringify(names));console.log(L.slice(0,3));await b.close()})();

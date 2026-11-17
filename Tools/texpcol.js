const p=require('puppeteer-core');const fs=require('fs');
(async()=>{const b=await p.launch({executablePath:'/usr/bin/google-chrome',args:['--no-sandbox','--use-gl=swiftshader','--enable-unsafe-swiftshader'],protocolTimeout:600000});
for(const tr of [0,1,2,3]){const pg=await b.newPage();
await pg.goto('http://localhost:8765/index.html');await new Promise(r=>setTimeout(r,6000));
const r=await pg.evaluate((tr)=>{selTrack=tr;wxSel='clear';curWx='clear';buildScene(tr);scene.updateMatrixWorld(true);
  const inst={};const order=[];
  scene.traverse(o=>{if(!o.isMesh||o===skyMesh)return;let q=o,vis=true;while(q){if(!q.visible){vis=false;break}q=q.parent}if(!vis)return;
    if(o.isPoints||o.isLine||o.isSprite)return;if(o.geometry===terrGeo)return;if(o.material&&(o.material.isShaderMaterial||o.material.isPointsMaterial))return;
    if(o.isInstancedMesh){if(!o.count)return;const key=o.geometry.uuid+'_'+(Array.isArray(o.material)?o.material.map(m=>m.uuid).join(''):o.material.uuid);
      if(!inst[key]){inst[key]={C:[],has:false,base:null};order.push(key);const m=Array.isArray(o.material)?o.material[0]:o.material;inst[key].base=m&&m.color?[m.color.r,m.color.g,m.color.b].map(v=>+v.toFixed(3)):[1,1,1];inst[key].vc=!!(m&&m.vertexColors)}
      const c=new THREE.Color();for(let k=0;k<o.count;k++){if(o.instanceColor){o.getColorAt(k,c);inst[key].has=true;inst[key].C.push(+c.r.toFixed(3),+c.g.toFixed(3),+c.b.toFixed(3))}else inst[key].C.push(1,1,1)}}});
  return order.map((k,i)=>({id:'i'+i,base:inst[k].base,vc:inst[k].vc,C:inst[k].has?inst[k].C:null}))},tr);
fs.writeFileSync(`/home/claude/ue/decor/t${tr}_col.json`,JSON.stringify(r));
console.log(tr,r.map(x=>x.id+':'+(x.C?x.C.length/3:'-')+':'+x.base.join(',')).join(' '));await pg.close()}
await b.close()})();

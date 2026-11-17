const p=require('puppeteer-core');const fs=require('fs');
(async()=>{const b=await p.launch({executablePath:'/usr/bin/google-chrome',args:['--no-sandbox','--use-gl=swiftshader','--enable-unsafe-swiftshader']});const pg=await b.newPage();
await pg.goto('http://localhost:8765/index.html');await new Promise(r=>setTimeout(r,5000));
for(const tr of [0,1,2,3]){const d=await pg.evaluate((tr)=>{selTrack=tr;wxSel='clear';curWx='clear';buildScene(tr);const def=TRACKS[tr];
  const r=(v)=>Math.round(v*100)/100;
  const out={name:def.name,deco:def.deco,water:!!def.water,halfW:T.halfW,N:T.N,P:T.P.map(p=>[r(p.x),r(p.y),r(p.z)]),NOR:T.NOR.map(n=>[r(n.x),r(n.z)]),lim:Array.from(T.lim||[]).map(r),openF:Array.from(T.openF||[]).map(r),
    cuts:(T.cuts||[]).map(c=>({type:c.type,a:c.a,b:c.b,C:c.C.map(q=>[r(q.x),r(q.y),r(q.z)]),w:c.w||c.hw||5})),rails:(T.rails||[]).map(x=>({i0:x.i0,len:x.len,lat:x.lat,h:x.h})),
    ramps:(typeof ramps!=='undefined'?ramps:[]),boxes:(typeof boxes!=='undefined'?boxes:[]).map(bx=>bx.mesh?[r(bx.mesh.position.x),r(bx.mesh.position.y),r(bx.mesh.position.z)]:null).filter(Boolean),
    sky:{top:def.skyTop,hor:def.skyHor,fog:def.fog,sun:def.sun,sunDir:def.sunDir,hemi:def.hemi},bb:T.bb};
  // heightmap
  const ext=terrInfo.ext,R=1009;const H=new Array(R*R);let mn=1e9,mx=-1e9;for(let j=0;j<R;j++)for(let i=0;i<R;i++){const x=terrInfo.cx-ext+2*ext*i/(R-1),z=terrInfo.cz-ext+2*ext*j/(R-1);const h=terrainH(x,z);H[j*R+i]=h;if(h<mn)mn=h;if(h>mx)mx=h}
  out.terrain={ext,cx:terrInfo.cx,cz:terrInfo.cz,res:R,min:mn,max:mx};out.H=H.map(r);
  // props instanced: collect instanced mesh transforms by name-ish
  return out},tr);
 fs.writeFileSync(`/home/claude/ue/track${tr}.json`,JSON.stringify(d));console.log(tr,d.name,d.N,d.cuts.length,d.boxes.length,d.terrain)}
await b.close()})();

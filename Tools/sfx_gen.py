import numpy as np, wave
SR=44100
def save(n,x,loop=False):
    x=np.clip(x,-1,1)
    if loop: # fondu circulaire pour boucler sans clic
        f=int(0.03*SR); x[:f]=x[:f]*np.linspace(0,1,f)+x[-f:]*np.linspace(1,0,f); x=x[:-f]
    w=wave.open(n+'.wav','wb'); w.setnchannels(1); w.setsampwidth(2); w.setframerate(SR); w.writeframes((x*32000).astype('<i2').tobytes()); w.close()
def t(d): return np.arange(int(d*SR))/SR
rng=np.random.default_rng(1)
def lp(x,a):  # passe-bas 1 pôle
    y=np.empty_like(x); s=0.
    for i in range(len(x)): s+=a*(x[i]-s); y[i]=s
    return y
def env(n,a,r): e=np.ones(n); A=int(a*SR); R=int(r*SR); e[:A]=np.linspace(0,1,A) if A else 1; e[-R:]*=np.linspace(1,0,R); return e
# moteur : impulsions de combustion à 55 Hz (pitch multiplié en jeu), harmoniques + bruit
tt=t(2.0); f0=55.0
ph=2*np.pi*f0*tt
x=0.55*np.sign(np.sin(ph))*0.3+0.5*np.sin(ph)+0.35*np.sin(2*ph+0.3)+0.25*np.sin(3*ph)+0.15*np.sin(4*ph+1)
x*= (0.7+0.3*np.maximum(0,np.sin(ph/2)))  # irrégularité 2 temps
x+=0.12*lp(rng.standard_normal(len(tt)),0.15)
save('engine',lp(x,0.35)*0.6,loop=True)
# dérapage : crissement (bruit filtré + modulation)
tt=t(2.0); n=rng.standard_normal(len(tt)); hp=n-lp(n,0.05)
x=lp(hp,0.5)*(0.7+0.3*np.sin(2*np.pi*13*tt))
save('drift',x*0.5,loop=True)
# turbo : souffle qui monte
tt=t(1.2); n=rng.standard_normal(len(tt)); e=env(len(tt),0.05,0.7)
cut=np.linspace(0.05,0.5,len(tt)); y=np.empty_like(n); s=0
for i in range(len(n)): s+=cut[i]*(n[i]-s); y[i]=s
x=y*e*1.2+0.3*np.sin(2*np.pi*np.cumsum(np.linspace(200,900,len(tt)))/SR)*e
save('boost',x*0.7)
# boîte à objet : arpège cristallin
x=np.zeros(int(0.6*SR))
for k,f in enumerate([880,1109,1319,1760]):
    s=int(k*0.06*SR); d=int(0.35*SR); tt=np.arange(d)/SR
    x[s:s+d]+=np.sin(2*np.pi*f*tt)*np.exp(-tt*9)*0.4+np.sin(2*np.pi*f*2*tt)*np.exp(-tt*14)*0.15
save('itembox',x)
# roulette : tic
tt=t(0.05); save('tick',np.sin(2*np.pi*1500*tt)*np.exp(-tt*120)*0.5)
# objet obtenu
x=np.zeros(int(0.5*SR))
for k,f in enumerate([660,990]):
    s=int(k*0.08*SR); tt=np.arange(int(0.4*SR))/SR; x[s:s+len(tt)]+=np.sin(2*np.pi*f*tt)*np.exp(-tt*7)*0.45
save('itemget',x[:int(0.5*SR)])
# lancer (carapace) : swoosh
tt=t(0.4); n=rng.standard_normal(len(tt)); save('throw',lp(n,0.25)*np.sin(np.pi*tt/0.4)**2*0.8)
# impact : boum + bruit
tt=t(0.7); x=np.sin(2*np.pi*np.cumsum(np.linspace(160,40,len(tt)))/SR)*np.exp(-tt*6)*0.9+lp(rng.standard_normal(len(tt)),0.4)*np.exp(-tt*10)*0.8
save('hit',x)
# bouclier : vibration lumineuse
tt=t(0.8); x=(np.sin(2*np.pi*520*tt)+0.6*np.sin(2*np.pi*780*tt*(1+0.01*np.sin(2*np.pi*6*tt))))*env(len(tt),0.05,0.4)*0.35; save('shield',x)
# décompte et départ
tt=t(0.35); save('beep',np.sin(2*np.pi*660*tt)*env(len(tt),0.005,0.1)*0.5)
tt=t(0.8); save('go',(np.sin(2*np.pi*1320*tt)+0.3*np.sin(2*np.pi*2640*tt))*env(len(tt),0.005,0.4)*0.45)
# atterrissage
tt=t(0.35); save('land',(np.sin(2*np.pi*90*tt)*0.8+lp(rng.standard_normal(len(tt)),0.3)*0.4)*np.exp(-tt*14))
# tour : ding
tt=t(1.0); save('lap',(np.sin(2*np.pi*1046*tt)+0.5*np.sin(2*np.pi*1568*tt))*np.exp(-tt*4)*0.4)
# menu
tt=t(0.08); save('ui_move',np.sin(2*np.pi*900*tt)*np.exp(-tt*50)*0.35)
tt=t(0.25); save('ui_ok',(np.sin(2*np.pi*700*tt)+np.sin(2*np.pi*1050*tt))*np.exp(-tt*12)*0.3)

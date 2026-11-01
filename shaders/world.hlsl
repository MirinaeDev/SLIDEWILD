struct Params { row_major float4x4 mvp; float4 eye; float4 weather;float4 camera;row_major float4x4 sunMvp;float4 dynamics;float4 raster;float4 rayX;float4 rayY;float4 rayZ;float4 riderState[4];float4 cloth[4]; };
ConstantBuffer<Params> vertexParams : register(b0, space1);
ConstantBuffer<Params> pixelParams : register(b0, space3);
Texture2D<float4> riderArt:register(t0,space2);SamplerState riderSampler:register(s0,space2);
Texture2D<float4> pineArt:register(t1,space2);SamplerState pineSampler:register(s1,space2);
Texture2D<float4> valleyArt:register(t2,space2);SamplerState valleySampler:register(s2,space2);
Texture2D<float4> vegetationArt:register(t3,space2);SamplerState vegetationSampler:register(s3,space2);
Texture2D<float4> asphaltArt:register(t4,space2);SamplerState asphaltSampler:register(s4,space2);
Texture2D<float4> grassArt:register(t5,space2);SamplerState grassSampler:register(s5,space2);
Texture2D<float4> floraArt:register(t6,space2);SamplerState floraSampler:register(s6,space2);
Texture2D<float4> wallArt:register(t7,space2);SamplerState wallSampler:register(s7,space2);
Texture2D<float4> fallArt:register(t8,space2);SamplerState fallSampler:register(s8,space2);
Texture2D<float4> rockArt:register(t9,space2);SamplerState rockSampler:register(s9,space2);
Texture2D<float4> midgroundArt:register(t10,space2);SamplerState midgroundSampler:register(s10,space2);
Texture2D<float> sunDepth:register(t11,space2);SamplerState sunSampler:register(s11,space2);
// The UI is a separate draw phase and rebinds the first six texture slots.
// Keeping each pass below sixteen samplers also supports lower binding tiers.
#define menuArt riderArt
#define menuArtSampler riderSampler
#define logoArt pineArt
#define logoArtSampler pineSampler
#define selectSol valleyArt
#define selectSolSampler valleySampler
#define selectRio vegetationArt
#define selectRioSampler vegetationSampler
#define selectNox asphaltArt
#define selectNoxSampler asphaltSampler
#define fontArt grassArt
#define fontArtSampler grassSampler
Texture2D<float4> activeRider:register(t12,space2);SamplerState activeRiderSampler:register(s12,space2);
Texture2D<float4> activeFall:register(t13,space2);SamplerState activeFallSampler:register(s13,space2);
Texture2D<float4> badgeArt:register(t14,space2);SamplerState badgeSampler:register(s14,space2);
Texture2D<float4> parkingArt:register(t15,space2);SamplerState parkingSampler:register(s15,space2);
struct VSIn { float3 pos:TEXCOORD0;float3 normal:TEXCOORD1;float3 color:TEXCOORD2;float3 uvmat:TEXCOORD3; };
struct VSOut { float4 pos:SV_Position;float3 world:TEXCOORD0;float3 normal:TEXCOORD1;float3 color:TEXCOORD2;float3 uvmat:TEXCOORD3;noperspective float2 spriteUV:TEXCOORD4;nointerpolation float material:TEXCOORD5; };
VSOut vs_main(VSIn i){
 VSOut o;o.pos=mul(vertexParams.mvp,float4(i.pos,1));
 float mat=i.uvmat.z;
 if(mat<0||(mat>=5&&mat<5.5)||(mat>=22&&mat<22.5)||mat==25){
   float2 grid=vertexParams.raster.xy;
   float2 px=(o.pos.xy/o.pos.w*.5+.5)*grid;
   o.pos.xy=((round(px)/grid)*2-1)*o.pos.w;
 }
 o.world=i.pos;o.normal=i.normal;o.color=i.color;o.uvmat=i.uvmat;o.spriteUV=i.uvmat.xy;o.material=mat;return o;
}
float sceneBiome(){return abs(pixelParams.raster.w-5)<.5?0:pixelParams.raster.w;}
float hash(float2 p){return frac(sin(dot(p,float2(127.1,311.7)))*43758.5453);}
float noise(float2 p){float2 k=floor(p),f=frac(p);f=f*f*(3-2*f);return lerp(lerp(hash(k),hash(k+float2(1,0)),f.x),lerp(hash(k+float2(0,1)),hash(k+1),f.x),f.y);}
float3 weather(float3 c,float wet){float l=dot(c,float3(.25,.55,.2));return lerp(c,lerp(c,l.xxx,.4)*float3(.74,.83,.98),wet*.75);}
float sunlight(float3 world){
 float4 p=mul(pixelParams.sunMvp,float4(world,1));float2 uv=p.xy*float2(.5,-.5)+.5;
 if(any(uv<.002)||any(uv>.998)||p.z<0||p.z>1)return 1;
 float lit=0;
 [unroll]for(int y=0;y<2;y++)[unroll]for(int x=0;x<2;x++)
   lit+=step(p.z-.00065,sunDepth.SampleLevel(sunSampler,uv+(float2(x,y)-.5)/1536,0));
 float border=max(abs(uv.x*2-1),abs(uv.y*2-1));
 return lerp(lit*.25,1,smoothstep(.84,.98,border));
}
float repairMask(float s,float d){return saturate((.27-abs(sin(s*.081+.8)))*9)*saturate((3.3-abs(d-.75*sin(s*.043)))*2);}
float waterMask(float s,float d,float wet){return wet*saturate((sin(s*.117+d*.29)*sin(s*.047-d*.83)-.39)*3.2)*saturate((4.2-abs(d))*2);}
// Depth-only caster pass: alpha-tested vegetation, opaque architecture. The
// rider keeps its explicitly grounded contact/cast shadow and receives sunlight.
void shadow_ps(VSOut i){
 float mat=i.material;float2 uv=i.uvmat.xy;
 if(mat<0||(mat>=5&&mat<5.5)||mat==7||mat==14||(mat>=11&&mat<12)||mat==19||(mat>=22&&mat<29))clip(-1);
 if(mat>=29&&mat<29.5)clip(vegetationArt.Sample(vegetationSampler,uv).a-.70);
 if(mat>=32&&mat<32.5)clip(pineArt.Sample(pineSampler,uv).a-.70);
 if(mat>=33&&mat<33.5)clip(fallArt.Sample(fallSampler,uv).a-.70);
 if(mat>=35&&mat<35.5)clip(floraArt.Sample(floraSampler,uv).a-.72);
 if(mat==34||mat==49||(mat>=52&&mat<=55))clip(-1);
 if(mat==58)clip(wallArt.Sample(wallSampler,uv).a-.65);
 if(mat>=13&&mat<13.5)clip(vegetationArt.Sample(vegetationSampler,uv).a-.78);
 if(mat>=20&&mat<20.5)clip(grassArt.Sample(grassSampler,uv).a-.55);
 if(mat>=21&&mat<21.5)clip(floraArt.Sample(floraSampler,uv).a-.84);
 if(mat>=6&&mat<6.5)clip(pineArt.Sample(pineSampler,uv).a-.85);
}
float3 rockPatch(float2 p,float2 key,float2 dx,float2 dy){
 float2 offset=float2(hash(key),hash(key+37.7))*7;
 float2 uv=abs(frac((p+offset)*.5)*2-1);
 return rockArt.SampleGrad(rockSampler,uv,dx,dy).rgb;
}
float3 stonePlane(float2 p){
 // Randomly translated overlapping samples have no repeating tile boundary.
 // Explicit derivatives remain continuous across cell changes and mirror seams.
 float2 cell=floor(p),f=frac(p);f=f*f*(3-2*f);
 float2 dx=ddx(p),dy=ddy(p);
 float3 a=rockPatch(p,cell,dx,dy),b=rockPatch(p,cell+float2(1,0),dx,dy);
 float3 c=rockPatch(p,cell+float2(0,1),dx,dy),d=rockPatch(p,cell+1,dx,dy);
 return lerp(lerp(a,b,f.x),lerp(c,d,f.x),f.y);
}
float3 groundPatch(float2 p,float2 key,float2 dx,float2 dy){
 float index=clamp(round(sceneBiome())-1,0,3);
 float2 tile=float2(fmod(index,2),floor(index/2))*.5;
 float2 offset=float2(hash(key+14),hash(key+91))*5;
 float2 local=abs(frac((p+offset)*.5)*2-1);
 return grassArt.SampleGrad(grassSampler,tile+.002+local*.496,dx*.496,dy*.496).rgb;
}
float3 biomeGround(float2 p){
 float2 cell=floor(p),f=frac(p);f=f*f*(3-2*f);float2 dx=ddx(p),dy=ddy(p);
 return lerp(lerp(groundPatch(p,cell,dx,dy),groundPatch(p,cell+float2(1,0),dx,dy),f.x),
   lerp(groundPatch(p,cell+float2(0,1),dx,dy),groundPatch(p,cell+1,dx,dy),f.x),f.y);
}
float3 surfaceLight(float3 color,float3 n,float wet,float3 world){
 float sun=saturate(dot(n,normalize(float3(.70,.32,.64))));
 sun=floor(sun*12+.5)/12;
 float3 ambient=lerp(float3(.63,.65,.76),float3(.73,.75,.80),wet);
 float cloud=smoothstep(.35,.72,noise(world.xz*.011+float2(pixelParams.weather.y*.014,0)));
 float3 direct=float3(.49,.36,.22)*sun*(1-wet*.85)*(1-cloud*.40)*sunlight(world);
 return color*(ambient+direct);
}
float3 stoneSurface(float3 p,float3 normal,float wet){
 float3 n=normalize(normal),weights=pow(abs(n),6);weights/=max(dot(weights,1.xxx),.001);
 float3 stone=stonePlane(p.zy*.065)*weights.x+stonePlane(p.xz*.065)*weights.y+stonePlane(p.xy*.065)*weights.z;
 float macro=noise(p.xz*.065+p.y*.027);
 stone*=.82+floor(macro*7)*.055;
 // Broad mineral strata and broken dark fissures give the face readable mass
 // before the fine stone texture. They are anchored in world space.
 float shelf=p.y*.28+noise(p.xz*.12)*.85;
 float seam=1-smoothstep(.025,.115,abs(frac(shelf)-.5));
 float fracture=1-smoothstep(.02,.10,abs(noise(p.xz*.19+p.y*.095)-.48));
 stone*=1-seam*.19-fracture*.13;
 stone+=float3(.052,.039,.024)*(1-smoothstep(.04,.17,abs(frac(shelf)-.63)));
 float moss=smoothstep(.61,.79,noise(p.xz*.61+p.y*.22))*saturate(n.y*.6+.12);
 stone=lerp(stone,stone*float3(.65,.74,.39),moss*.55);
 return surfaceLight(stone,n,wet,p);
}
float3 panoramaTexel(float2 uv){
 // Native texels; the previous four-tap reconstruction blurred the backdrop.
 return valleyArt.SampleLevel(valleySampler,uv,0).rgb;
}
float3 panorama(float2 screen){
 float2 q=screen*2-1;
 float3 ray=normalize(pixelParams.rayZ.xyz+pixelParams.rayX.xyz*q.x+pixelParams.rayY.xyz*q.y);
 float yaw=atan2(ray.x,ray.z);
 float u=frac(yaw/6.2831853+.15);
 if(sceneBiome()>.5)u=1-abs(u*2-1);
 float v=clamp(.50-asin(ray.y)/1.57,0,1);
 float seam=(1-smoothstep(0,.015,min(u,1-u)))*.5;
 if(sceneBiome()>.5)seam=0;
 float3 farColor=lerp(panoramaTexel(float2(u,v)),panoramaTexel(float2(1-u,v)),seam);
 if(sceneBiome()>3.5&&sceneBiome()<4.5){
   float3 storm=midgroundArt.SampleLevel(midgroundSampler,float2(u,v),0).rgb;
   return lerp(farColor,storm,smoothstep(.08,.94,pixelParams.weather.x));
 }
 if(sceneBiome()>.5)return farColor;
 // A finite world-space cylindrical valley layer gives translation parallax;
 // the sky/high peaks remain at infinity and retain independent angular motion.
 float3 direction=ray;
 float3 eye=pixelParams.eye.xyz-float3(0,0,500);
 float a=dot(direction.xz,direction.xz),b=dot(eye.xz,direction.xz);
 float t=(-b+sqrt(max(0,b*b+a*(2200*2200-dot(eye.xz,eye.xz)))))/max(a,.001);
 float3 hit=eye+direction*t;
 float angle=atan2(hit.x,hit.z)+1.45;angle=atan2(sin(angle),cos(angle));
 float2 layerUV=float2(angle/4.1+.5,(1050-hit.y)/2400);
 float4 middle=midgroundArt.SampleLevel(midgroundSampler,layerUV,0);
 float edge=smoothstep(0,.06,layerUV.x)*(1-smoothstep(.94,1,layerUV.x));
 float coverage=step(0,layerUV.y)*step(layerUV.y,1)*edge*smoothstep(.35,.8,middle.a);
 return lerp(farColor,middle.rgb,coverage);
}
float4 pilotArt(float2 uv,float pilot){
 float4 state=pixelParams.riderState[clamp(int(pilot),0,3)];
 float2 cell=frac(uv*float2(state.x>=11?4:3,2));
 // Cloth animation uses authored frames. Never shift UVs into a neighbouring
 // pose or distort the body, board or gloves in the fragment shader.
 float4 c=activeRider.SampleLevel(activeRiderSampler,uv,0);
 // Registered drawn folds only. The reference silhouette, head, limbs and
 // board always come from frame zero, including their alpha and shadows.
 float4 cloth=pixelParams.cloth[clamp(int(pilot),0,3)];
 if((state.x==0||state.x==5)&&uv.y>cloth.z&&uv.y<cloth.w&&c.a>.7){
   float4 next=activeRider.SampleLevel(activeRiderSampler,uv+cloth.xy,0);
   bool fabric=pilot<.5?(min(c.r,min(c.g,c.b))>.36&&max(c.r,max(c.g,c.b))-min(c.r,min(c.g,c.b))<.20):
     pilot<1.5?(c.g>c.r*1.25&&c.b>c.r*1.15):pilot<2.5?(c.r>c.g*1.15&&c.b>c.g*1.2):(c.r>c.g*1.4&&c.r>c.b*1.15);
   if(fabric&&next.a>.7)c.rgb=next.rgb;
 }
 float hi=max(c.r,max(c.g,c.b)),lo=min(c.r,min(c.g,c.b));
 bool wheel=cell.y>.72&&hi-lo>.22&&hi>.32&&
   (pilot<.5?(c.r>c.g*1.25&&c.g>c.b*1.3):pilot<1.5?(c.r>c.g*1.4&&c.r>c.b*1.25):pilot<2.5?(c.r>c.b*2&&c.g>c.b*2&&c.g>c.r*.55):(c.b>c.g*1.3&&c.r>c.g*1.15));
 if(wheel)c.rgb*=.96+.10*sin(uv.y*1024*.42-state.z*70);
 return c;
}
float3 futureHaze(float3 c,float3 p,float density=.0009){
 return lerp(c,float3(.40,.54,.65),saturate(1-exp(-length(p-pixelParams.eye.xyz)*density)));
}
float futureLight(float3 n){
 return .68+floor(saturate(dot(normalize(n),normalize(float3(-.6,.7,.4))))*4)*.075;
}
float3 futureSurface(float2 metres,float2 cell){
 // Each painted material covers a physical 6 x 4 m module. Explicit atlas
 // insets prevent adjacent panels bleeding into one another under minification.
 float2 st=metres/float2(6,4),local=frac(st);
 return asphaltArt.SampleGrad(asphaltSampler,cell*.5+.0015+local*.497,ddx(st)*.497,ddy(st)*.497).rgb;
}
float3 futureAggregate(float2 metres){
 // Overlapping stochastic patches, with four independently painted sources.
 // Each patch samples INSIDE a source cell: no frac() seam, no atlas neighbour
 // bleeding and no repeated 3 m slab. Derivatives ignore the random offset.
 float2 p=metres/float2(5.7,8.3),base=floor(p),f=frac(p),blend=f*f*(3-2*f);
 float3 color=0;float weight=0;
 [unroll]for(int y=0;y<2;y++)[unroll]for(int x=0;x<2;x++){
   float2 corner=float2(x,y),id=base+corner;
   float variant=floor(hash(id+17.41)*3.999);
   float2 source=float2(fmod(variant,2),floor(variant/2));
   float2 flip=float2(hash(id+41)>.5?1:-1,hash(id+73)>.5?1:-1);
   float2 offset=float2(hash(id+9),hash(id+53))*.24;
   float2 local=.38+offset+(f-corner)*flip*.24;
   float w=(x?blend.x:1-blend.x)*(y?blend.y:1-blend.y);w*=w;
   float3 c=pineArt.SampleGrad(pineSampler,source*.5+.003+local*.494,ddx(p)*flip*.24*.494,ddy(p)*flip*.24*.494).rgb;
   color+=c*w;weight+=w;
 }
 return color/max(weight,.001);
}
float3 futureConcrete(float2 metres){
 float3 grain=futureAggregate(metres*.55);
 float luminance=dot(grain,float3(.25,.55,.2));
 float broad=floor(noise(metres*float2(.037,.018))*8)/8;
 return (float3(.54,.57,.54)+(luminance-.42)*.25)*(.89+broad*.20);
}
float3 futureDeckPanel(float x,float along,float column,float stretch){
 float2 st=float2(saturate(x/8.05+.5),saturate(1-along));
 return fallArt.SampleGrad(fallSampler,float2(column*.25+.002,.003)+st*float2(.246,.994),
   float2(ddx(x)/8.05*.246,-ddx(along)*.994),float2(ddy(x)/8.05*.246,-ddy(along)*.994)).rgb*stretch;
}
float3 futureRacingDeck(float2 uv){
 // Wide composite plates and inset hardware, designed as a track surface.
 // Joints are staggered by lane; no gravel texture is used as the road base.
 float lane=floor((uv.x+4.05)/2.7),laneX=frac((uv.x+4.05)/2.7);
 float offset=hash(float2(lane,13))*13,group=floor((uv.y+offset)/23);
 float jointAt=group*23-offset+4+hash(float2(group,lane))*14;
 float seam=1-smoothstep(.025,.065,abs(uv.y-jointAt));
 float side=1-smoothstep(.015,.04,min(laneX,1-laneX));
 float footprint=max(length(ddx(uv)),length(ddy(uv)));
 float fine=1-smoothstep(.025,.12,footprint);
 float scratch=floor(noise(uv*float2(7,.08))*6)/6;
 float tone=noise(uv*float2(.18,.028));
 float3 c=lerp(float3(.22,.29,.34),float3(.31,.38,.42),tone);
 c+=(scratch-.5)*.018;
 c+=(hash(floor(uv*70))-.5)*.012*fine;
 c=lerp(c,float3(.09,.14,.18),max(seam,side));
 // Physically authored sections have unequal lengths and source variants.
 // The illustrated inserts occupy a part of each 79 m interval; the quiet
 // composite joins between them prevent an endless chain of the same image.
 float sector=floor(uv.y/79),begin=sector*79+5+hash(float2(sector,81))*13;
 float span=25+hash(float2(sector,43))*14,along=(uv.y-begin)/span;
 float choice=floor(hash(float2(sector,27))*2.999),column=choice==2?3:choice;
 float endBlend=smoothstep(0,.035,along)*(1-smoothstep(.965,1,along));
 float transverse=1-smoothstep(3.99,4.06,abs(uv.x));
 float flip=hash(float2(sector,91))>.5?1:-1;
 c=lerp(c,futureDeckPanel(uv.x*flip,along,column,.88),endBlend*transverse);
 // Six distinct painted timing/sector inlays are integrated flush with the
 // banked collision surface; they never masquerade as raised geometry.
 const float markers[6]={246,876,1396,2196,3336,3676};
 [unroll]for(int j=0;j<6;j++){
   float t=(uv.y-markers[j])/18;
   float mask=smoothstep(0,.025,t)*(1-smoothstep(.975,1,t))*transverse;
   c=lerp(c,futureDeckPanel(uv.x,t,2,.96),mask);
 }
 return c;
}
float4 ps_main(VSOut i):SV_Target{
 float mat=i.material,wet=pixelParams.weather.x;float2 uv=i.uvmat.xy;
 if(mat< -19.5){float4 c=parkingArt.SampleLevel(parkingSampler,uv,0);clip(c.a-.3);return c;}
 if(mat< -18.5){float4 c=badgeArt.SampleLevel(badgeSampler,uv,0);clip(c.a-.3);return c;}
 if(mat< -17.5){float4 c=activeRider.SampleLevel(activeRiderSampler,uv,0);clip(c.a-.6);return c;}
 if(mat< -16.5){float4 c=midgroundArt.SampleLevel(midgroundSampler,uv,0);clip(c.a-.6);return c;}
 if(mat< -15.5){float4 c=rockArt.SampleLevel(rockSampler,uv,0);clip(c.a-.6);return c;}
 if(mat< -14.5)return fallArt.SampleLevel(fallSampler,uv,0);
 if((mat>=29&&mat<29.5)||(mat>=32&&mat<33.5)||(mat>=35&&mat<35.5)){
   clip((mat-(mat>=35?35:mat>=33?33:mat>=32?32:29))/.4-hash(floor(i.pos.xy)));
   float4 c=mat>=35?floraArt.SampleLevel(floraSampler,uv,0):mat>=33?fallArt.SampleLevel(fallSampler,uv,0):mat>=32?pineArt.SampleLevel(pineSampler,uv,0):vegetationArt.SampleLevel(vegetationSampler,uv,0);clip(c.a-.72);
   float distance=length(i.world-pixelParams.eye.xyz);
   float3 haze=sceneBiome()<1.5?float3(.67,.43,.31):float3(.20,.25,.38);
   if(sceneBiome()>2.5)haze=lerp(float3(.47,.66,.68),float3(.32,.38,.45),wet);
   if(sceneBiome()>5.5)haze=sceneBiome()>6.5?float3(.32,.20,.24):float3(.18,.24,.33);
   float lighting=mat>=35?lerp(.78,1,sunlight(i.world)):1;
   if(sceneBiome()>5.5&&sceneBiome()<6.5)lighting=lerp(.70,.92,sunlight(i.world));
   return float4(lerp(weather(c.rgb*i.color*lighting,wet),haze,saturate(1-exp(-distance*.0013))),1);
 }
 if(mat==70||mat==74){
   float3 c=floraArt.SampleGrad(floraSampler,uv,ddx(uv),ddy(uv)).rgb;
   c*=futureLight(i.normal)*lerp(.82,1,sunlight(i.world));
   return float4(futureHaze(c,i.world),1);
 }
 if(mat==71||mat==72||mat==73){
   float3 n=abs(normalize(i.normal));float2 p=n.y>.6?i.world.xz:n.x>n.z?i.world.zy:float2(i.world.x,-i.world.y);
   float3 c=futureConcrete(p)*i.color;
   float panel=1-smoothstep(.025,.065,min(abs(frac(p.x/8)-.5)*8,abs(frac(p.y/5)-.5)*5));
   c*=1-panel*.22;
   if(mat==72){
     float reflection=.60+.30*noise(p*float2(.045,.08));
     float mullion=1-smoothstep(.025,.055,abs(frac(p.x/5)-.5));
     c=i.color*reflection*(1-mullion*.5)+float3(.02,.10,.11);
   }
   if(mat==73){
     // One large, original directional emblem on a solid sign plate.
     float2 q=uv*2-1;
     float chevron=1-smoothstep(.15,.20,abs(q.y-abs(q.x)*.65+.2));
     c=lerp(float3(.11,.19,.23),float3(.96,.67,.12),chevron);
   }
   c*=futureLight(i.normal)*lerp(.84,1,sunlight(i.world));
   return float4(futureHaze(c,i.world),1);
 }
 if(mat==64){
   return float4(futureHaze(i.color*futureLight(i.normal),i.world),1);
 }
 if(mat==65||mat==69){
   float row=mat==69?3:floor(uv.y),v=mat==69?uv.y:frac(uv.y);
   float2 st=float2(mat==69?uv.x:frac(uv.x),v);
   float3 c=rockArt.SampleGrad(rockSampler,float2(.001,.001+row*.25)+st*float2(.998,.248),ddx(uv)*float2(.998,.248),ddy(uv)*float2(.998,.248)).rgb;
   float glow=saturate(c.g-c.r)*saturate(c.b-c.r)*.45;
   c=c*futureLight(i.normal)+float3(.13,.7,.85)*glow;
   return float4(futureHaze(c,i.world),1);
 }
 if(mat>=66&&mat<=68){
   float3 n=abs(normalize(i.normal));float2 p=n.y>.6?i.world.xz:n.x>n.z?i.world.zy:float2(i.world.x,-i.world.y);
   float2 cell=mat==66?float2(0,1):mat==67?float2(1,0):float2(1,1);
   float3 c=futureSurface(p,cell)*futureLight(i.normal);
   c*=lerp(.84,1,sunlight(i.world));
   return float4(futureHaze(c,i.world),1);
 }
 if(mat>=60&&mat<64){
   float3 c=futureConcrete(uv)*i.color;
   float recess=1-smoothstep(.10,.22,abs(frac(uv.x/12)-.5));
   float joint=1-smoothstep(.015,.04,abs(frac(uv.y/24)-.5));
   c*=1-recess*.24-joint*.12;
   c*=futureLight(i.normal);
   return float4(futureHaze(c,i.world),1);
 }
 if(mat==58){
   float4 c=wallArt.Sample(wallSampler,uv);clip(c.a-.65);
   float light=.89+.11*sunlight(i.world);
   return float4(futureHaze(c.rgb*light,i.world),1);
 }
 if(mat==54){
   float4 c=fallArt.SampleLevel(fallSampler,uv,0);clip(c.a-.45);
   return float4(c.rgb*(1.05+pixelParams.eye.w*.10),1);
 }
 if(mat==53)return float4(i.color,1);
 if(mat==52){
   float2 p=i.world.xz*.31+i.world.y*float2(.025,.12);
   p.y+=pixelParams.weather.y*.16;
   float flow=noise(p+float2(sin(p.y+pixelParams.weather.y*.4)*.3,0));
   float crust=smoothstep(.49,.67,noise(p*2.3-float2(0,pixelParams.weather.y*.07)));
   float3 c=lerp(float3(.95,.14,.025),float3(1,.79,.20),flow);
   c=lerp(c,float3(.16,.045,.025),crust*.88);
   return float4(c,1);
 }
 if(mat==50||mat==51||mat==56||mat==57){
   float3 n=abs(normalize(i.normal));float2 p=n.y>.65?i.world.xz:n.x>n.z?i.world.zy:i.world.xy;
   float scale=(mat==50||mat==57)?.075:mat==51?.12:.25;
   float2 local=abs(frac(p*scale*.5)*2-1);
   float2 tile=(mat==50||mat==57)?float2(0,0):mat==51?float2(1./3,0):float2(2./3,.5);
   if(mat==56)local=float2(frac(uv.x),uv.y);
   float3 c=rockArt.SampleGrad(rockSampler,tile+float2(.002,.002)+local*float2(.329,.496),ddx(p*scale)*float2(.329,.496),ddy(p*scale)*float2(.329,.496)).rgb;
   float macro=floor(noise(i.world.xz*.13+i.world.y*.025)*6)/6;
   c*=.82+macro*.36;
   c=surfaceLight(c,normalize(i.normal),wet,i.world);
   float warm=.035*(.5+.5*sin(pixelParams.weather.y*2.3+i.world.x*.07))*pixelParams.eye.w;
   c+=float3(warm,warm*.25,0);
   return float4(lerp(c,float3(.32,.20,.24),saturate(1-exp(-length(i.world-pixelParams.eye.xyz)*.00135))),1);
 }
 if(mat==49){
   // Luminous strips keep their colour in shadow; the small diffuse spill is
   // evaluated in road coordinates, so it remains fixed during camera motion.
   float depth=length(i.world-pixelParams.eye.xyz);
   return float4(lerp(i.color,float3(.23,.21,.37),saturate(1-exp(-depth*.00065))),1);
 }
 if(mat==48){
   float3 n=abs(normalize(i.normal));
   float2 p=n.y>.65?i.world.xz:n.x>n.z?i.world.zy:i.world.xy;
   float grain=hash(floor(p*18));
   float panel=step(.975,frac(p.x*.35))+step(.985,frac(p.y*.25));
   float light=.60+.36*abs(dot(normalize(i.normal),normalize(float3(.6,.4,.8))));
   float3 c=i.color*light*(1-min(panel,1)*.28)+(grain-.5)*.018;
   return float4(lerp(c,float3(.23,.21,.37),saturate(1-exp(-length(i.world-pixelParams.eye.xyz)*.0012))),1);
 }
 if(mat==34){
   float2 p=floor(i.world.xz*2)/2;float wave=sin(p.x*.8+sin(p.y*.11)+pixelParams.weather.y*1.8);
   float glint=step(.85,wave)*step(.58,noise(p*.28+pixelParams.weather.y*.1));
   float3 c=lerp(float3(.035,.52,.57),float3(.12,.24,.34),wet);
   if(sceneBiome()>6.5)c=float3(.025,.16,.22);
   c+=float3(.34,.47,.42)*glint*(.3+wet*.4);
   return float4(c,1);
 }
 if(mat==30||mat==59){
   float3 c=mat==59?grassArt.Sample(grassSampler,uv).rgb:floraArt.Sample(floraSampler,uv).rgb;
   float light=.80+.18*abs(dot(normalize(i.normal),normalize(float3(.7,.3,.4))));
   if(sceneBiome()>5.5&&sceneBiome()<6.5){
     light=futureLight(i.normal);
     float glow=saturate(max(c.b,c.g)-c.r)*.12;
     c=c*i.color*light+float3(.20,.80,1)*glow;
     return float4(futureHaze(c,i.world),1);
   }
   float glow=smoothstep(.48,.78,c.r)*saturate(c.r-c.b)*.35;
   c=c*light+float3(1,.68,.32)*glow;
   return float4(lerp(c,float3(.20,.25,.38),saturate(1-exp(-length(i.world-pixelParams.eye.xyz)*.0012))),1);
 }
 if(mat==31){
   float2 p=i.world.xz*2.2;float joints=step(.94,frac(p.x))+step(.94,frac(p.y));
   float n=floor(noise(floor(p))*5)/5;
   float3 c=biomeGround(i.world.xz*.055)*(.90+n*.08)*(1-joints*.05);
   return float4(c,1);
 }
 if(mat==28){
   float2 st=uv/7,tx=abs(frac(st*.5)*2-1);
   float3 c=asphaltArt.SampleGrad(asphaltSampler,tx,ddx(st),ddy(st)).rgb;
   c*=.86+floor(noise(uv*.27)*7)/35;
   c*=1-smoothstep(.67,.79,noise(uv*.7))* .12;
   return float4(weather(surfaceLight(c,normalize(i.normal),wet,i.world),wet),1);
 }
 if(mat==27){float4 c=parkingArt.SampleLevel(parkingSampler,uv,0);clip(c.a-.65);return float4(c.rgb,1);}
 if(mat==26){float4 c=badgeArt.SampleLevel(badgeSampler,uv,0);clip(c.a-.5);return float4(c.rgb,1);}
 if(mat< -13.5){float4 c=wallArt.SampleLevel(wallSampler,uv,0);clip(c.a-.6);return float4(c.rgb,1);}
 if(mat< -12.5)return float4(i.color,uv.x);
 if(mat< -11.5){float r=dot(uv*2-1,uv*2-1);clip(1-r);return float4(.018,.022,.04,(1-smoothstep(.1,1,r))*.65);}
 if(mat< -10.5){float4 c=fontArt.SampleLevel(fontArtSampler,uv,0);return float4(i.color,c.a);}
 if(mat< -7.5){
   float4 c=mat>-8.5?selectSol.SampleLevel(selectSolSampler,uv,0):(mat>-9.5?selectRio.SampleLevel(selectRioSampler,uv,0):selectNox.SampleLevel(selectNoxSampler,uv,0));
   clip(c.a-.6);return float4(c.rgb*i.color,1);
 }
 if(mat< -6.5){float4 c=logoArt.SampleLevel(logoArtSampler,uv,0);clip(c.a-.2);return c;}
 if(mat< -5.5)return float4(menuArt.SampleLevel(menuArtSampler,uv,0).rgb*i.color,1);
 if(mat< -4.5){
   float4 c=riderArt.SampleLevel(riderSampler,uv,0);clip(c.a-.85);
   float neutral=1-smoothstep(.08,.19,max(c.r,max(c.g,c.b))-min(c.r,min(c.g,c.b)));
   c.rgb=lerp(c.rgb,c.rgb*i.color,neutral*step(.19,max(c.r,max(c.g,c.b))));return float4(c.rgb,1);
 }
 if(mat< -3.5){
   float2 p=(floor(uv*302)+.5)/302*2-1;float rr=dot(p,p);clip(1-rr);
   float3 n=float3(p.x,-p.y,sqrt(saturate(1-rr)));float yaw=i.color.x;
   float3 world=float3(n.x*cos(yaw)+n.z*sin(yaw),n.y,n.z*cos(yaw)-n.x*sin(yaw));
   float longitude=atan2(world.x,world.z),latitude=asin(world.y);
   float2 mapUV=float2(frac(longitude/6.2831853+.27),.5-latitude/3.14159265);
   float3 color=floraArt.SampleLevel(floraSampler,mapUV,0).rgb;
   float light=floor((.40+.60*saturate(dot(n,normalize(float3(-.5,.6,1)))))*14)/14;
   color=color*light+pow(1-n.z,4)*float3(.035,.14,.20);
   return float4(color,1);
 }
 // Mild fixed barrel distortion only outside the central rider area. Sampling
 // logical pixel centres keeps crisp blocks; HUD is rendered after this pass.
 if(mat< -2.5){
   float2 p=uv*2-1;float radius=length(p);
   float speed=pixelParams.dynamics.x,impact=pixelParams.dynamics.y;
   float strength=(.014+speed*speed*.008)*smoothstep(.55,1.25,radius);
   float2 source=(p*(1-strength*radius*radius)+1)*.5;
   float2 grid=pixelParams.raster.xy;
   if(sceneBiome()>6.5){
     float heat=sin(uv.y*100+pixelParams.weather.y*3)*pixelParams.eye.w*smoothstep(.36,.85,length(p));
     source.x+=round(heat*1.6)/grid.x;
   }
   source=(floor(source*grid)+.5)/grid;
   // Sparse droplets stay on the lens perimeter, leaving the driving line clear.
   float2 cells=float2(12,7),cell=floor(uv*cells),local=frac(uv*cells);
   float seed=hash(cell+41),phase=frac(pixelParams.weather.y*.07+seed);
   float2 center=float2(.2+hash(cell+19)*.6,.18+phase*.64);
   float2 droplet=(local-center)*float2(30,18/(1+speed*.8));
   float dropRadius=length(droplet);
   float edge=step(.72,max(abs(p.x),abs(p.y)))*step(.70,seed);
   float drop=pixelParams.dynamics.w*edge*smoothstep(0,.12,phase)*(1-smoothstep(.86,1,phase));
   source+=round(droplet*saturate(1-dropRadius)*drop*1.5)/grid;
   float3 color=riderArt.SampleLevel(riderSampler,source,0).rgb;
   // Radial speed streaks only outside the driving line. Integer-texel taps
   // preserve the internal pixel grid; no temporal trails or HUD blur.
   float rush=smoothstep(.39,.89,speed);
   float2 flow=uv-float2(.5,.43);
   float periphery=smoothstep(.26,.62,length(flow*float2(1,.82)));
   float tunnel=rush*periphery;
   if(tunnel>.005){
     float3 streak=color*2;float weight=2;
     [unroll]for(int j=1;j<=6;j++){
       float2 sampleUV=source-flow*(j/6.)*.115*tunnel;
       sampleUV=(floor(sampleUV*grid)+.5)/grid;
       float w=1-j*.09;
       streak+=riderArt.SampleLevel(riderSampler,sampleUV,0).rgb*w;weight+=w;
     }
     color=lerp(color,streak/weight,tunnel*.93);
   }
   // A single logical pixel of channel separation only during an impact.
   if(impact>.05){
     float2 shift=round(p*impact)/grid;
     color.r=riderArt.SampleLevel(riderSampler,source+shift,0).r;
     color.b=riderArt.SampleLevel(riderSampler,source-shift,0).b;
   }
   float3 glow=0;
   const float2 taps[8]={float2(3,0),float2(-3,0),float2(0,3),float2(0,-3),float2(7,7),float2(-7,7),float2(7,-7),float2(-7,-7)};
   [unroll]for(int tap=0;tap<8;tap++){
     float3 light=riderArt.SampleLevel(riderSampler,source+taps[tap]/grid,0).rgb;
     glow+=light*smoothstep(.73,.95,dot(light,float3(.25,.55,.2)));
   }
   color+=glow*.024*(1-wet*.75);
   // The painted sun's position is measured in the panorama. Bright-source
   // occlusion gates the halo so it disappears behind an opaque tree or cliff.
   float3 sunDirection=float3(sin(.830)*cos(.325),sin(.325),cos(.830)*cos(.325));
   float depth=dot(sunDirection,pixelParams.rayZ.xyz);
   float2 q=float2(dot(sunDirection,pixelParams.rayX.xyz)/dot(pixelParams.rayX.xyz,pixelParams.rayX.xyz),dot(sunDirection,pixelParams.rayY.xyz)/dot(pixelParams.rayY.xyz,pixelParams.rayY.xyz))/max(depth,.01);
   float2 sunUV=(q+1)*.5;
   if(depth>.1&&all(sunUV>0)&&all(sunUV<1)){
     float3 sunColor=riderArt.SampleLevel(riderSampler,sunUV,0).rgb;
     float visible=smoothstep(.68,.88,dot(sunColor,float3(.25,.55,.2)))*(1-wet);
     float distance=length((uv-sunUV)*float2(pixelParams.camera.w,1));
     color+=float3(1,.58,.20)*exp(-distance*13)*visible*.06;
     float shaft=0;
     [unroll]for(int ray=1;ray<=6;ray++){
       float2 sampleUV=lerp(uv,sunUV,ray*.10);
       float3 light=riderArt.SampleLevel(riderSampler,sampleUV,0).rgb;
       shaft+=smoothstep(.75,.96,dot(light,float3(.25,.55,.2)));
     }
     color+=float3(1,.69,.36)*shaft*visible*.006;
   }
   color+=drop*(1-smoothstep(.07,.24,abs(dropRadius-.85)))*.075;
   color*=pixelParams.dynamics.z;
   float luma=dot(color,float3(.25,.55,.2));
   color=lerp(luma.xxx,color,.90);
   color=(color-.45)*1.045+.45;
   color*=lerp(float3(1.025,.995,.965),float3(.98,1,1.02),pixelParams.weather.x);
   color*=1-(.075+rush*.075)*smoothstep(.6,1.4,radius);
   if(sceneBiome()>3.5&&wet>.8){
     float phase=fmod(pixelParams.weather.y,23.7);
     float flash=exp(-abs(phase-17.2)*17)*.15+exp(-abs(phase-17.5)*24)*.09;
     color+=float3(.75,.83,1)*flash;
   }
   return float4(saturate(color),1);
 }
 if(mat< -1.5)return float4(weather(panorama(i.pos.xy/pixelParams.raster.xy),wet),1);
 if(mat< -.5)return float4(i.color,1);
 if(mat==25){
   float4 c=riderArt.SampleLevel(riderSampler,i.spriteUV,0);clip(c.a-.70);
   float chroma=max(c.r,max(c.g,c.b))-min(c.r,min(c.g,c.b));
   float rubber=smoothstep(.45,.7,chroma)*step(c.b*2,c.g)*step(c.g*1.2,c.r);
   c.rgb=lerp(c.rgb,i.color*(c.r*.9+.10),rubber);
   return float4(weather(c.rgb*(.8+.2*sunlight(i.world)),wet),1);
 }
 if(mat>8.5&&mat<9.5)return float4(i.color,1);
 if(mat==24)return float4(weather(i.color*(.7+.3*sunlight(i.world)),wet),1);
 if(mat>22.5){
   clip(-1);return 0;
 }
 if(mat>21.5){
   float4 t=activeFall.SampleLevel(activeFallSampler,i.spriteUV,0);clip(t.a-.70);
   return float4(weather(t.rgb,wet),1);
 }
 if(mat>20.5){
   float4 t=floraArt.Sample(floraSampler,uv);clip(t.a-.84);
   clip((mat-21)/.4-hash(floor(uv*float2(1536,1024))));
   float distance=length(i.world-pixelParams.eye.xyz);
   return float4(lerp(weather(t.rgb*i.color,wet),float3(.39,.39,.49),saturate(1-exp(-distance*.0018))),1);
 }
 if(mat>19.5){
   float4 t=grassArt.Sample(grassSampler,uv);clip(t.a-.55);clip((mat-20)/.4-hash(floor(uv*1254)));
   float distance=length(i.world-pixelParams.eye.xyz);
   return float4(lerp(weather(t.rgb*i.color,wet),float3(.39,.39,.49),saturate(1-exp(-distance*.0011))),1);
 }
 if(mat>18.5){float distance=length(i.world-pixelParams.eye.xyz);return float4(lerp(i.color,float3(.37,.43,.48),saturate(distance/80)),1);}
 if(mat>17.5){
   float3 stone=stoneSurface(i.world,i.normal,wet);
   float distance=length(i.world-pixelParams.eye.xyz);
   return float4(lerp(weather(stone*i.color,wet),float3(.39,.39,.49),saturate(1-exp(-distance*.0011))),1);
 }
 if(mat>15.5){
   float2 tile=abs(frac(uv*.5)*2-1);
   float3 stone=wallArt.SampleGrad(wallSampler,tile,ddx(uv),ddy(uv)).rgb;
   return float4(weather(stone*i.color,wet),1);
 }
 if(mat>14.5){
   // Broad, stable brush marks along the rail; muted steel instead of glitter.
   float2 brush=floor(uv*float2(7,95))/float2(7,95);
   float wear=floor(noise(brush*float2(1.4,16))*5)/5;
   float scar=step(.77,noise(brush*float2(4,35)));
   float3 col=surfaceLight(i.color*(.87+wear*.24)-scar*.035,normalize(i.normal),wet,i.world);
   float distance=length(i.world-pixelParams.eye.xyz);
   return float4(lerp(weather(col,wet),float3(.39,.39,.49),saturate(1-exp(-distance*.0011))),1);
 }
 if(mat>13.5){float4 t=pilotArt(uv,round((mat-14)*10));clip(t.a-.70);return float4(.035,.045,.065,.46*(1-wet*.60));}
 if(mat>12.5){float4 t=vegetationArt.Sample(vegetationSampler,uv);clip(t.a-.78);clip((mat-13)/.4-hash(floor(uv*1254)));float distance=length(i.world-pixelParams.eye.xyz);return float4(lerp(weather(t.rgb,wet),float3(.39,.39,.49),saturate(1-exp(-distance*.0011))),1);}
 if(mat>=11){
   float age=saturate((mat-11)/.9),radius=length(uv*2-1);
   float cloud=noise(floor(uv*24)/4+float2(age*.6,0));
   float coverage=(1-smoothstep(.20,1,radius+cloud*.16))*(1-age)*smoothstep(0,.12,age)*(i.color.r>.7?.48:.24);
   clip(coverage-.005);
   return float4(weather(i.color,wet),coverage);
 }
 if(mat>9.5){
   float straw=hash(floor(float2(i.world.x*12+i.world.z*2,i.world.y*36)));
   float seam=step(.97,frac(i.world.y*9));
   float light=.75+.4*saturate(dot(normalize(i.normal),normalize(float3(.7,.5,.3))));
   return float4(weather(i.color*(light+(floor(straw*4)-1.5)*.08-seam*.12),wet),1);
 }
 if(mat>6.5&&mat<7.5){float2 p=uv*2-1;float r=dot(p,p);clip(1-r);return float4(.025,.028,.035,(1-smoothstep(.3,1,r))*.42);}
 float distance=length(i.world-pixelParams.eye.xyz);float3 col=i.color;
 if(mat>4.5){
   float4 tex;
   if(mat<5.5){
     tex=pilotArt(i.spriteUV,round((mat-5)*10));
   }else tex=pineArt.Sample(pineSampler,uv);
   clip(tex.a-.85);col=tex.rgb;
   if(mat>=5.5)col*=abs(i.color);
   if(mat<5.5)col*=lerp(float3(.65,.72,.83),float3(1.02,.99,.95),lerp(sunlight(i.world),1,wet*.65));
   if(mat>7.5){float light=.77+.27*abs(dot(normalize(i.normal),normalize(float3(.7,.35,.25))));col*=light;}
   if(mat>5.5&&mat<6.5){
     // The target foliage is matte olive / blue-black, not glossy golden needles.
     float light=dot(col,float3(.25,.55,.2));
     float band=floor(saturate(light*1.8)*12)/12;
     col=lerp(float3(.022,.041,.037),float3(.40,.42,.19),band)*.78+col*float3(.20,.23,.20);
   }
 }else if(mat>.5&&mat<1.5){
   int course=int(pixelParams.raster.w+.5);
   if(course==7){
     float2 st=abs(frac(uv*float2(.14,.045)*.5)*2-1);
     float3 c=asphaltArt.SampleGrad(asphaltSampler,float2(2./3+.002,.002)+st*float2(.329,.496),ddx(uv*float2(.14,.045))*float2(.329,.496),ddy(uv*float2(.14,.045))*float2(.329,.496)).rgb;
     c=lerp(float3(.16,.145,.15),c,.43);
     c*=.86+.28*floor(noise(uv*float2(5,.08))*6)/6;
     float grit=hash(floor(uv*float2(42,35))),macro=noise(uv*float2(.8,.07));
     c*=.86+macro*.28;
     float edgeAsh=smoothstep(3.2,4.7,abs(uv.x)+noise(uv*.35)*.55);
     c=lerp(c,float3(.31,.28,.27),edgeAsh*.52);
     float crack=1-smoothstep(.006,.022,abs(noise(uv*float2(1.3,.65))-.49));
     c*=1-crack*.22;c+=(grit-.5)*.022;
     float paint=(1-smoothstep(.04,.075,abs(abs(uv.x)-4.32)));
     float dash=(1-smoothstep(.045,.075,abs(uv.x)))*step(frac(uv.y/11),.39);
     c=lerp(c,float3(.80,.71,.54),max(paint,dash)*(.8+.2*grit));
     c=surfaceLight(c,normalize(i.normal)*sign(i.normal.y),wet,i.world);
     float vent=round((uv.y-180)/126),at=180+vent*126,phase=fmod(pixelParams.weather.y+vent*.93,8);
     float burst=saturate((phase-3.4)/.35)*saturate((5.35-phase)/.65);
     float spill=exp(-pow((uv.y-at)/26,2))*(.35+.65*burst)*smoothstep(-1,4.5,uv.x);
     c+=float3(.16,.047,.007)*spill;
     c*=1-wet*.25;
     return float4(lerp(c,float3(.32,.20,.24),saturate(1-exp(-length(i.world-pixelParams.eye.xyz)*.00125))),1);
   }
   if(course==6){
     float3 c=futureRacingDeck(uv);
     float shoulder=smoothstep(4.03,4.09,abs(uv.x));
     float stripe=step(.50,frac(uv.x*.52+uv.y*.39));
     c=lerp(c,lerp(float3(.13,.18,.19),float3(.86,.65,.18),stripe),shoulder*.93);
     float edge=(1-smoothstep(.018,.045,abs(abs(uv.x)-3.89)));
     c=lerp(c,float3(.70,.75,.71),edge*.85);
     c=surfaceLight(c,normalize(i.normal)*sign(i.normal.y),wet,i.world);
     return float4(futureHaze(c,i.world),1);
   }
   // Continuous longitudinal road-space strokes, no baked photograph patches.
   float grain=hash(floor(float2(uv.x*42,uv.y*24)));
   float footprint=max(length(ddx(i.world.xz)),length(ddy(i.world.xz)));
   float detail=1-smoothstep(.10,.65,footprint);
   float2 st=float2(uv.x/6+.07*sin(uv.y*.03),uv.y/11);
   float2 mirrored=abs(frac(st*.5)*2-1);
   col=asphaltArt.SampleGrad(asphaltSampler,mirrored,ddx(st),ddy(st)).rgb*.94;
   float strokes=floor(noise(float2(uv.x*7,uv.y*.075))*7)/7;
   col*=.84+strokes*.31;
   col+=(floor(grain*4)-1.5)*.013*detail;
   float2 secondUV=abs(frac((st+float2(.37,.63))*.5)*2-1);
   float patchBlend=smoothstep(.35,.70,noise(float2(uv.x*.6,uv.y*.08)))*.42;
   col=lerp(col,asphaltArt.SampleGrad(asphaltSampler,secondUV,ddx(st),ddy(st)).rgb*.86,patchBlend);
   // Repairs and curved rubber marks are fixed in road coordinates.
   float repaired=repairMask(uv.y,uv.x);col*=1-repaired*.24;
   col+=(floor(grain*5)-2)*.017*repaired*detail;
   float mark=abs(uv.x-(sin(uv.y*.055)*1.5+.6));
   col*=1-(1-smoothstep(.025,.07,mark))*.17*step(.65,noise(float2(0,uv.y*.04)));
   // Distinct road materials, in stable road coordinates. Their aggregate,
   // repairs, deposits and markings differ, not just their colour tint.
   float shoulder=smoothstep(2.9,4.65,abs(uv.x)+noise(uv*float2(2,.12))*.55);
   if(course==1){
     col=lerp(col,float3(.39,.31,.25),.44);
     float fissure=1-smoothstep(.005,.018,abs(noise(uv*float2(2.3,.7))-.49));
     col*=1-fissure*.20*detail;
     col=lerp(col,float3(.63,.40,.22),shoulder*(.48+.2*noise(uv)));
   }else if(course==2){
     col=lerp(col,float3(.105,.12,.145),.57);
     float service=step(abs(uv.x-1.5),1.05)*step(frac((uv.y+5)/83),.043);
     col=lerp(col,float3(.22,.235,.24),service*.65);
     float2 cover=float2(uv.x+1.55,(frac((uv.y+11)/61)-.5)*61);
     float disk=1-smoothstep(.36,.4,length(cover));
     col=lerp(col,float3(.19,.20,.21)*( .65+.35*step(.5,frac(cover.x*12))),disk);
   }else if(course==3){
     float aggregate=hash(floor(uv*float2(24,19)));
     col=lerp(col,float3(.20,.225,.24),.45)+(floor(aggregate*4)-1.5)*.026*detail;
     float oldPatch=smoothstep(.40,.49,noise(uv*float2(.7,.18)));
     col*=1-oldPatch*.24;
     col=lerp(col,float3(.23,.27,.16),shoulder*noise(uv*2)*.48);
   }else if(course==4){
     col=lerp(col,float3(.42,.40,.35),.55);
     col+=(floor(hash(floor(uv*float2(31,27)))*5)-2)*.020*detail;
     col=lerp(col,float3(.69,.61,.43),shoulder*.58);
     float seam=1-smoothstep(.01,.035,abs(frac(uv.y/36)-.5));
     col*=1-seam*.17;
   }else if(course==5){
     col=lerp(col,float3(.13,.16,.19),.80);
     col+=(grain-.5)*.015*detail;
     float rubber=exp(-pow((abs(uv.x)-2.5-.25*sin(uv.y*.07))*5,2));
     col*=1-rubber*.24;
   }
   float edge=1-smoothstep(.055,.09,abs(abs(uv.x)-4.34));
   float dash=(1-smoothstep(.065,.10,abs(uv.x)))*step(frac(uv.y/10),.50);
   float chipped=1-step(.80,grain)*detail*.36;
   float3 paint=float3(.78,.74,.68);
   if(course==1){dash=(1-smoothstep(.035,.065,abs(abs(uv.x)-.13)));paint=float3(.89,.64,.22);}
   if(course==3)chipped*=.70+.30*step(.4,grain);
   if(course==4){chipped*=.76;paint=float3(.91,.86,.70);}
   if(course==5){
     dash=0;edge=1-smoothstep(.025,.045,abs(abs(uv.x)-4.05));paint=float3(.32,.88,.78);
     float curb=smoothstep(4.26,4.30,abs(uv.x));
     float3 stripe=frac(uv.y/6)<.5?float3(.88,.80,.66):float3(.71,.16,.12);
     col=lerp(col,stripe*(.88+.12*grain),curb);
   }
   col=lerp(col,paint,max(edge,dash)*chipped);
   col=surfaceLight(col,normalize(i.normal)*sign(i.normal.y),wet,i.world);
   if(sceneBiome()>1.5&&sceneBiome()<2.5){
     col*=float3(.78,.84,1.04);
     float lamp=exp(-pow((frac((uv.y+16)/32)-.5)*32/5,2))*exp(-pow((abs(uv.x)-4)/2.6,2));
     col+=float3(.12,.075,.022)*lamp;
   }
   col*=1-wet*.30;
   if(wet>.01){
     float puddle=waterMask(uv.y,uv.x,wet);
     float3 eyeRay=normalize(i.world-pixelParams.eye.xyz),normal=normalize(i.normal)*sign(i.normal.y);
     float3 reflected=reflect(eyeRay,normal);
     float ripples=sin(uv.y*4+pixelParams.weather.y*5)*sin(uv.x*13-pixelParams.weather.y*3)*.002;
     float2 reflectionUV=float2(frac(atan2(reflected.x,reflected.z)/6.2831853+.15+ripples),saturate(.50-asin(reflected.y)/1.57));
     float3 reflection=panoramaTexel(reflectionUV);
     float fresnel=saturate(distance/45);
     col=lerp(col,reflection*float3(.74,.82,.97),puddle*(.28+fresnel*.40));

   }
 }else if(mat>1.5&&mat<2.5){
   float3 n=abs(normalize(i.normal));float2 rock=n.y>.65?i.world.xz:(n.x>n.z?i.world.zy:i.world.xy);
   float fleck=noise(floor(rock*12)/12),seam=step(.89,noise(rock*1.3));
   col=i.color*(.79+floor(fleck*5)*.095-seam*.3);
 }else if(mat>2.5&&mat<3.5){
   if(sceneBiome()>.5){
     float macro=noise(i.world.xz*.075),fleck=noise(floor(i.world.xz*5)/5);
     float3 earth=lerp(float3(.43,.24,.13),float3(.73,.48,.24),floor(macro*7)/7);
     earth*=.86+fleck*.23;
     earth=lerp(earth,stonePlane(i.world.xz*.065)*float3(1.10,.95,.72),.32);
     if(sceneBiome()>1.5)earth=lerp(float3(.20,.22,.26),float3(.31,.32,.34),macro);
     if(sceneBiome()>2.5&&sceneBiome()<3.5){
       earth=lerp(float3(.15,.21,.12),float3(.33,.35,.18),floor(macro*7)/7);
       earth+=float3(.08,.015,.07)*smoothstep(.60,.78,noise(i.world.xz*.21));
       earth*=.84+floor(fleck*5)*.07;
     }
     if(sceneBiome()>3.5){
       earth=lerp(float3(.68,.57,.36),float3(.90,.82,.61),floor(macro*6)/6);
       float grass=smoothstep(.43,.62,noise(i.world.xz*.055));
       earth=lerp(earth,float3(.24,.37,.13)*(.85+fleck*.4),grass*.85);
     }
     earth=lerp(biomeGround(i.world.xz*.049),earth,.18);
     float steep=saturate((.62-abs(normalize(i.normal).y))/.30);
     col=lerp(surfaceLight(earth,normalize(i.normal)*sign(i.normal.y),wet,i.world),stoneSurface(i.world,i.normal,wet),steep);
     float3 haze=sceneBiome()<1.5?float3(.67,.43,.31):float3(.20,.25,.38);
     if(sceneBiome()>2.5)haze=lerp(float3(.47,.66,.68),float3(.32,.38,.45),wet);
     return float4(lerp(weather(col,wet),haze,saturate(1-exp(-distance*.0012))),1);
   }
   float n=hash(floor(i.world.xz*8)),patch=noise(i.world.xz*.32);
   col=i.color*(.68+floor(patch*6)*.085);
   col+=(floor(n*3)-1)*.015;
   // Persistent fine meadow cover underneath the silhouette LODs. Footprint
   // attenuation prevents noisy subpixel blades on distant slopes.
   float footprint=max(length(ddx(i.world.xz)),length(ddy(i.world.xz)));
   float detail=1-smoothstep(.12,.75,footprint);
   float2 tuft=floor(i.world.xz*float2(12,19));
   float blade=hash(tuft),meadow=noise(i.world.xz*.09);
   col=lerp(col,col*float3(1.20,1.12,.77),smoothstep(.5,.8,meadow)*.45);
   col+=(step(.77,blade)*float3(.045,.048,.022)-step(blade,.22)*float3(.024,.028,.013))*detail;
   // Exposed steep cuts are rock, not impossible vertical grass sheets.
   float steep=saturate((.65-abs(normalize(i.normal).y))/.25);
   float3 rock=stoneSurface(i.world,i.normal,wet);
   col=surfaceLight(col,normalize(i.normal)*sign(i.normal.y),wet,i.world);
   col=lerp(col,rock,steep);
 }
 col=weather(col,wet);
 float lowMist=exp(-max(0,i.world.y-pixelParams.eye.y)*.055);
 float haze=saturate(1-exp(-distance*(.0010+lowMist*.00045+wet*.00035)));
 col=lerp(col,float3(.40,.43,.52),haze);
 return float4(saturate(col),1);
}


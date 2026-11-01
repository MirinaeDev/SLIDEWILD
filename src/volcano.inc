namespace {
struct VolcanoProp {V3 p;float h;int kind;float seed;};
struct VolcanoVent {V3 p;float height,phase;};
std::vector<VolcanoProp> volcanoProps;
std::vector<VolcanoVent> volcanoVents;
const float volcanoBounds[12][4]={{41,63,299,442},{340,177,629,442},{640,87,940,449},{940,90,1221,444},
 {32,552,328,824},{363,597,630,830},{679,483,932,819},{1028,493,1199,822},
 {42,927,365,1167},{393,969,630,1180},{669,948,974,1191},{1023,922,1202,1175}};
// Exact nearest-segment queries without scanning all 2,080 segments for every
// terrain vertex. Expand until the unvisited cells cannot beat the best hit.
struct VolcanoLocator {
 const Road& road;std::unordered_map<long long,std::vector<int>> cells;
 explicit VolcanoLocator(const Road& r):road(r){
   for(int j=0;j+1<int(r.points.size());j++){
     auto a=r.points[j].p,b=r.points[j+1].p;
     for(int x=int(std::floor(std::min(a.x,b.x)/64));x<=int(std::floor(std::max(a.x,b.x)/64));x++)
       for(int z=int(std::floor(std::min(a.z,b.z)/64));z<=int(std::floor(std::max(a.z,b.z)/64));z++)cells[cellKey(x,z)].push_back(j);
   }
 }
 RoadLocation operator()(V3 p)const{
   int cx=int(std::floor(p.x/64)),cz=int(std::floor(p.z/64));float best=1e20f,along=0;
   for(int ring=0;ring<64;ring++){
     for(int x=cx-ring;x<=cx+ring;x++)for(int z=cz-ring;z<=cz+ring;z++){
       if(ring&&x!=cx-ring&&x!=cx+ring&&z!=cz-ring&&z!=cz+ring)continue;
       auto found=cells.find(cellKey(x,z));if(found==cells.end())continue;
       for(int j:found->second){
         V3 a=road.points[j].p,v=road.points[j+1].p-a,q=p-a;v.y=q.y=0;
         float t=clamp(dot(q,v)/dot(v,v),0,1),ds=dot(q-v*t,q-v*t);
         if(ds<best){best=ds;along=(j+t)*Road::step;}
       }
     }
     float boundary=std::min({p.x-(cx-ring)*64,(cx+ring+1)*64-p.x,p.z-(cz-ring)*64,(cz+ring+1)*64-p.z});
     if(boundary*boundary>best)break;
   }
   auto r=road.at(along);return {along,dot(p-r.p,V3{std::cos(r.heading),0,-std::sin(r.heading)}),std::sqrt(best)};
 }
};
// Every mass has shared rings, an underground floor and a closed crown.
void basaltMass(Mesh& m,V3 root,float radius,float height,float seed){
 constexpr int sides=9,levels=5;V3 rings[levels][sides];
 const float profile[]={1,.96f,.81f,.73f,.48f};
 for(int l=0;l<levels;l++)for(int k=0;k<sides;k++){
   float angle=k*2*pi/sides+seed*.41f,r=radius*profile[l]*(.80f+hash(seed+k*3)*.20f);
   rings[l][k]=root+V3{std::cos(angle)*r,height*l/(levels-1)+(l==0?0:hash(seed+k*7)*height*.08f),std::sin(angle)*r};
 }
 for(int l=0;l<levels-1;l++)for(int k=0;k<sides;k++)quad(m,rings[l][k],rings[l][(k+1)%sides],rings[l+1][(k+1)%sides],rings[l+1][k],{1,1,1},50);
 for(int k=0;k<sides;k++){
   triangle(m,root,rings[0][(k+1)%sides],rings[0][k],{1,1,1},50);
   triangle(m,root+V3{0,height*1.1f,0},rings[levels-1][k],rings[levels-1][(k+1)%sides],{1,1,1},50);
 }
}
void volcanicBuilding(Mesh& m,V3 root,float width,float depth,float height,float yaw,bool research){
 V3 x{std::cos(yaw)*width*.5f,0,-std::sin(yaw)*width*.5f},z{std::sin(yaw)*depth*.5f,0,std::cos(yaw)*depth*.5f};
 V3 c[]={root-x-z,root+x-z,root+x+z,root-x+z};
 float u=research?1.f/3:0;
 for(int j=0;j<4;j++)artQuad(m,c[j]+V3{0,height,0},c[(j+1)%4]+V3{0,height,0},c[(j+1)%4],c[j],30,u+.002f,.502f,u+.331f,.998f);
 V3 topA=root-z+V3{0,height+2.2f,0},topB=root+z+V3{0,height+2.2f,0};
 quad(m,c[0]+V3{0,height,0},topA,topB,c[3]+V3{0,height,0},{.32f,.21f,.19f},48);
 quad(m,topA,c[1]+V3{0,height,0},c[2]+V3{0,height,0},topB,{.30f,.20f,.18f},48);
 triangle(m,c[0]+V3{0,height,0},c[1]+V3{0,height,0},topA,{.33f,.27f,.25f},48);
 triangle(m,c[2]+V3{0,height,0},c[3]+V3{0,height,0},topB,{.33f,.27f,.25f},48);
}
StaticWorld buildVolcanoWorld(const Road& road){
 StaticWorld result;Mesh& m=result.vertices;volcanoProps.clear();volcanoVents.clear();m.reserve(1600000);
 VolcanoLocator locate(road);
 float minX=1e6f,maxX=-1e6f,minZ=1e6f,maxZ=-1e6f;
 for(auto p:road.points){minX=std::min(minX,p.p.x);maxX=std::max(maxX,p.p.x);minZ=std::min(minZ,p.p.z);maxZ=std::max(maxZ,p.p.z);}
 const float cell=3;minX=std::floor((minX-100)/cell)*cell;minZ=std::floor((minZ-100)/cell)*cell;
 int nx=int((maxX+100-minX)/cell)+1,nz=int((maxZ+100-minZ)/cell)+1;
 terrainField={minX,minZ,cell,nx,nz,{}};terrainField.heights.resize(size_t(nx)*nz);
 struct Land {V3 p;float distance;};std::vector<Land> land(size_t(nx)*nz);
 for(int z=0;z<nz;z++)for(int x=0;x<nx;x++){
   V3 p{minX+x*cell,0,minZ+z*cell};auto loc=locate(p);double height=0,weights=0;
   float support=60+loc.distance*loc.distance*.48f;
   for(size_t j=0;j<road.points.size();j+=5){
     V3 delta=p-road.points[j].p;delta.y=0;float ds=dot(delta,delta),relative=std::max(0.f,(ds-loc.distance*loc.distance)/support);
     if(relative>14)continue;
     double weight=std::exp(-double(relative));
     height+=(road.points[j].p.y-std::min(std::sqrt(ds)*.10f,18.f))*weight;weights+=weight;
   }
   float relief=clamp((loc.distance-12)/35,0,1);
   float folds=6*std::sin(p.x*.047f+p.z*.018f)+3*std::sin(p.z*.071f-p.x*.031f);
   p.y=float(height/std::max(weights,1e-30))-.15f+relief*(folds+(loc.d<0?5.f:-5.f));
   terrainField.heights[z*nx+x]=p.y;land[z*nx+x]={p,loc.distance};
 }
 for(int z=0;z<nz-1;z++)for(int x=0;x<nx-1;x++){
   auto a=land[z*nx+x],b=land[(z+1)*nx+x],c=land[(z+1)*nx+x+1],d=land[z*nx+x+1];
   if(std::min({a.distance,b.distance,c.distance,d.distance})<10.5f)continue;
   quad(m,a.p,b.p,c.p,d.p,{1,1,1},51);
 }
 float sea=road.at(Road::finishDistance).p.y-7;
 quad(m,{minX-500,sea,minZ-500},{minX-500,sea,maxZ+800},{maxX+800,sea,maxZ+800},{maxX+800,sea,minZ-500},{1,1,1},34);
 auto point=[&](float s,float d,float y=0){return road.surface(s,d)+V3{0,y,0};};
 // Road and shoulders overlap the cut heightfield generously. Their outer
 // vertices use that exact field, preventing transparent cracks at the join.
 result.segmentOffsets.push_back(Uint32(m.size()));
 for(float s=0;s<road.size();s+=2){
   size_t begin=m.size();quad(m,point(s,-4.7f),point(s+2,-4.7f),point(s+2,4.7f),point(s,4.7f),{1,1,1},1);
   float uv[6][2]={{-4.7f,s},{-4.7f,s+2},{4.7f,s+2},{-4.7f,s},{4.7f,s+2},{4.7f,s}};
   for(int k=0;k<6;k++){m[begin+k].u=uv[k][0];m[begin+k].v=uv[k][1];}
   for(int side:{-1,1}){
     V3 a=point(s,side*4.7f),b=point(s+2,side*4.7f);
     for(float off=5.5f;off<18;off+=1.5f){
       V3 c=point(s+2,side*off),d=point(s,side*off);float blend=clamp((off-6)/9,0,1);
       c.y=mix(c.y,terrainField.height(c)+.025f,blend);d.y=mix(d.y,terrainField.height(d)+.025f,blend);
       auto loc=locate((a+b+c+d)*.25f);
       if(std::abs(loc.s-s)<16||loc.distance>6.5f)quad(m,a,b,c,d,{1,1,1},51);
       a=d;b=c;
     }
     // Charcoal retaining wall uphill; continuous metal rail above the coast.
     if(s>3990)continue;
     if(side<0&&std::fmod(s,220.f)<124){
       artQuad(m,point(s,-5.15f,.68f),point(s+2,-5.15f,.68f),point(s+2,-5.15f,-.05f),point(s,-5.15f,-.05f),56,s/8,0,(s+2)/8,1);
       quad(m,point(s,-5.15f,.68f),point(s,-5.6f,.68f),point(s+2,-5.6f,.68f),point(s+2,-5.15f,.68f),{.28f,.27f,.29f},48);
       quad(m,point(s,-5.6f,-.2f),point(s+2,-5.6f,-.2f),point(s+2,-5.6f,.68f),point(s,-5.6f,.68f),{.18f,.18f,.21f},48);
     }else{
       for(float y:{.52f,.70f})quad(m,point(s,side*5.15f,y),point(s+2,side*5.15f,y),point(s+2,side*5.15f,y+.12f),point(s,side*5.15f,y+.12f),{.48f,.45f,.43f},48);
       if(int(s)%4==0)futureBeam(m,point(s,side*5.22f,-.20f),point(s,side*5.22f,.85f),.065f,{.30f,.28f,.29f});
     }
     if(int(s)%16==0){
       box(m,point(s,side*5.15f,.70f),{.13f,.18f,.09f},{1,.48f,.17f},road.at(s).heading,49);
     }
   }
 }
 // Basalt buttresses, smaller scree and irregular boulders, all closed solids.
 for(int j=0;j<510;j++){
   float s=20+j*7.75f,seed=float(j+137);int side=j%3==0?1:-1;
   float radius=j%5==0?7+hash(seed)*6:2+hash(seed)*3.7f;
   float offset=side*(radius+9+hash(seed+7)*19);V3 base=point(s,offset);
   if(locate(base).distance<radius+7)continue;
   base.y=terrainField.height(base)-1.8f;
   for(int k=0;k<8;k++)base.y=std::min(base.y,terrainField.height(base+V3{std::cos(k*pi/4)*radius,0,std::sin(k*pi/4)*radius})-1);
   float h=radius*(side<0?2.1f:1.1f)*(1+hash(seed+9)*.7f);
   basaltMass(m,base,radius,h,seed);
 }
 indexRocks(m);indexVerges(m,0);
 // Rooted multilevel vegetation: low cover at the verge, dense fern groves
 // away from the rail, and irregular props between the closed rock masses.
 for(int j=0;j<7900;j++){
   float seed=float(j+9001),s=10+hash(seed)*3970;int side=j%2?1:-1;
   int kind=j%11==0?3:j%5==0?2:j%3==0?4:5;
   float h=kind==3?5+hash(seed+3)*5:kind==2?2.8f+hash(seed+3)*3.2f:kind==4?1+hash(seed+3)*1.2f:.38f+hash(seed+3)*.65f;
   float offset=side*(kind==5?5.95f+hash(seed+4)*12:9+h*.5f+hash(seed+4)*36);
   V3 root=point(s,offset);float radius=h*.58f;auto loc=locate(root);
   if(loc.distance<5.85f+radius||!sceneryPlantClear(root,radius))continue;
   root.y=sceneryHeight(root)-.04f;volcanoProps.push_back({root,h,kind,seed});
 }
 // Dense low cover follows the verge, with a second irregular band behind it.
 for(int j=0;j<2800;j++)for(int side:{-1,1})for(int row=0;row<2;row++){
   float seed=float(j*17+row*11+side*37),s=12+j*1.42f+hash(seed)*.8f;
   if(s>3980)continue;
   int kind=j%4==0?4:5;float height=.55f+hash(seed+8)*.55f;
   float offset=side*(6.5f+row*3.1f+hash(seed+4)*(row?3.5f:1.2f));
   V3 root=point(s,offset);root.y=sceneryHeight(root)-.045f;
   volcanoProps.push_back({root,height,kind,seed});
 }
 for(int j=0;j<245;j++){
   float s=30+j*16.1f,seed=float(j+67);int kind=j%4==0?6:j%4==1?7:j%4==2?0:11;
   float h=kind==6?2.2f:kind==7?2.4f:kind==11?1.0f:1.5f;
   V3 root=point(s,(j%2?1:-1)*(7.2f+hash(seed)*2.5f));
   if(!sceneryPlantClear(root,h*.52f)||locate(root).distance<6.6f)continue;
   root.y=sceneryHeight(root);volcanoProps.push_back({root,h,kind,seed});
 }
 // Off-road geysers and lava channels remain scenery. Their timing uses the
 // same fixed-step run clock as the smaller, signalled vents on the road.
 for(int j=0;j<29;j++){
   float s=180+j*126.f;V3 p{};bool placed=false;
   // Keep a clear sightline from the verge to the vent. A valid ground point
   // alone could bury an eruption inside a buttress or behind a larger rock.
   for(float offset:{16.f,21.f,28.f,36.f}){
     for(int attempt=0;attempt<2&&!placed;attempt++){
       int side=(j%4==0?-1:1)*(attempt?-1:1);
       V3 candidate=point(s,side*offset);
       if(locate(candidate).distance<12||!sceneryPlantClear(candidate,4))continue;
       bool visible=true;
       for(float d=7;d<offset;d+=3)if(!sceneryPlantClear(point(s,side*d),2)){visible=false;break;}
       if(visible){p=candidate;placed=true;}
     }
     if(placed)break;
   }
   if(!placed)continue;
   p.y=sceneryHeight(p)+.10f;
   volcanoVents.push_back({p,10+hash(float(j+45))*16,j*.93f});
   basaltMass(m,p-V3{0,2,0},3.1f,2.6f,float(j+810));
   for(int k=0;k<12;k++){
     float a=k*2*pi/12,b=(k+1)*2*pi/12;
     triangle(m,p+V3{0,.65f,0},p+V3{std::cos(a)*1.9f,.2f,std::sin(a)*1.9f},p+V3{std::cos(b)*1.9f,.2f,std::sin(b)*1.9f},{1,.45f,.1f},52);
   }
 }
 for(float begin:{710.f,1740.f,2360.f,3010.f}){
   for(int j=0;j<34;j++){
     float s=begin+j*2.5f,off=24+std::sin(s*.043f)*5;
     V3 a=point(s,off-2.2f),b=point(s+2.5f,off-2.2f),c=point(s+2.5f,off+2.2f),d=point(s,off+2.2f);
     if(locate((a+b+c+d)*.25f).distance<12)continue;
     for(V3* p:{&a,&b,&c,&d})p->y=sceneryHeight(*p)+.10f;
     artQuad(m,a,b,c,d,52,0,s*.14f,1,(s+2.5f)*.14f);
   }
 }
 // A lavafall beside the exit of the first tunnel gives the course a landmark.
 {
   V3 base=point(1560,32);base.y=sceneryHeight(base);basaltMass(m,base-V3{0,4,0},12,45,872);
   auto r=road.at(1560);V3 across{std::sin(r.heading)*3,0,std::cos(r.heading)*3};
   V3 away{std::cos(r.heading),0,-std::sin(r.heading)},front=base-away*11;
   for(int j=0;j<10;j++){
     float t=j/10.f,n=(j+1)/10.f;
     V3 a=base-away*(6.4f+t*4.6f)+V3{0,37*(1-t),0},b=base-away*(6.4f+n*4.6f)+V3{0,37*(1-n),0};
     artQuad(m,a-across,a+across,b+across,b-across,52,0,t*6,1,n*6);
   }
   volcanoVents.push_back({front+V3{0,.2f,0},8,2});
 }
 // Short basalt galleries. The rock roof is elevated above the banked road;
 // portal lights give a visual entry/exit reference in the dark sections.
 for(float begin:{1370.f,2450.f}){
   auto ringPoint=[&](float s,int j,bool outer){
     float angle=j*pi/12,radius=outer?9.1f:6.2f;
     return point(s,std::cos(angle)*radius,std::sin(angle)*(outer?9.2f:6.8f));
   };
   for(float s=begin;s<begin+42;s+=2){
     for(int j=0;j<12;j++){
       quad(m,ringPoint(s,j,false),ringPoint(s+2,j,false),ringPoint(s+2,j+1,false),ringPoint(s,j+1,false),{1,1,1},57);
       quad(m,ringPoint(s,j+1,true),ringPoint(s+2,j+1,true),ringPoint(s+2,j,true),ringPoint(s,j,true),{1,1,1},57);
     }
     for(int j:{0,12})quad(m,ringPoint(s,j,false),ringPoint(s,j,true),ringPoint(s+2,j,true),ringPoint(s+2,j,false),{1,1,1},57);
     if(int(s-begin)%8==0)for(int side:{-1,1})box(m,point(s,side*5.7f,2.2f),{.15f,.4f,.4f},{1,.45f,.14f},0,49);
   }
   for(float s:{begin,begin+42})for(int j=0;j<12;j++)
     quad(m,ringPoint(s,j,false),ringPoint(s,j+1,false),ringPoint(s,j+1,true),ringPoint(s,j,true),{1,1,1},57);
 }
 for(const auto& hazard:caldeiraHazards){
   V3 p=point(hazard.s,hazard.d);
   if(hazard.kind==HazardKind::Basalt)basaltMass(m,p-V3{0,.09f,0},hazard.radius,.75f,hazard.s);
   else for(int k=0;k<12;k++){
     float a=k*2*pi/12,b=(k+1)*2*pi/12;
     V3 q=point(hazard.s+std::sin(a)*hazard.radius,hazard.d+std::cos(a)*hazard.radius,.027f);
     V3 r=point(hazard.s+std::sin(b)*hazard.radius,hazard.d+std::cos(b)*hazard.radius,.027f);
     triangle(m,p+V3{0,.027f,0},q,r,{.27f,.10f,.06f},48);
   }
   // Reflective warning triangles, before the obstacle and confined to its lane.
   for(int k=0;k<3;k++){
     float s=hazard.s-10-k*5;
     triangle(m,point(s+1,hazard.d,.03f),point(s-1,hazard.d-.6f,.03f),point(s-1,hazard.d+.6f,.03f),{1,.56f,.17f},49);
   }
 }
 // Observatory at the summit and a built-up fishing port at the finish.
 {
   V3 p=point(80,-26);p.y=sceneryHeight(p);volcanicBuilding(m,p,18,14,8,road.at(80).heading,true);
   basaltMass(m,p+V3{0,8,0},5.2f,6,15);
 }
 for(float s=4008;s<4158;s+=2){
   size_t first=m.size();quad(m,point(s,-17,-.01f),point(s+2,-17,-.01f),point(s+2,17,-.01f),point(s,17,-.01f),{1,1,1},1);
   float uv[6][2]={{-17,s},{-17,s+2},{17,s+2},{-17,s},{17,s+2},{17,s}};
   for(int k=0;k<6;k++){m[first+k].u=uv[k][0];m[first+k].v=uv[k][1];}
 }
 auto end=road.at(road.size());V3 f{std::sin(end.heading),0,std::cos(end.heading)},q{f.z,0,-f.x};
 for(int side:{-1,1})for(int j=0;j<5;j++){
   float s=4018+j*29.f;V3 root=point(s,side*25.f,-1);
   volcanicBuilding(m,root,14,23,7+(j%3)*2,road.at(s).heading,false);
   for(int k=0;k<4;k++)volcanoProps.push_back({point(s+k*3,side*12.f),1.3f+(k%2)*.5f,k%2?9:11,float(j*4+k)});
 }
 for(int j=-3;j<=3;j++){
   V3 root=end.p+f*23+q*(j*15.f)-V3{0,1,0};volcanicBuilding(m,root,15.1f,18,12+(j%2)*3,end.heading,false);
 }
 for(int side:{-1,1}){
   futureBeam(m,point(4000,side*5.5f),point(4000,side*5.5f,6.8f),.22f,{.18f,.19f,.23f});
 }
 box(m,point(4000,0,5.6f),{11.7f,2.5f,.3f},{.10f,.07f,.08f},road.at(4000).heading,48);
 artQuad(m,point(3999.8f,-5.6f,6.8f),point(3999.8f,5.6f,6.8f),point(3999.8f,5.6f,4.4f),point(3999.8f,-5.6f,4.4f),26,0,2.f/3,1,1);
 for(int row=0;row<3;row++)for(int k=0;k<16;k++){
   float s=4000+row*.59f,d=-4.7f+k*.5875f;
   quad(m,point(s,d,.025f),point(s+.59f,d,.025f),point(s+.59f,d+.5875f,.025f),point(s,d+.5875f,.025f),(row+k)%2?V3{.08f,.07f,.09f}:V3{.95f,.84f,.65f},49);
 }
 // Index only terrain/closed masses for subsequent sprite and particle queries.
 indexRocks(m);indexVerges(m,0);
 for(auto& prop:volcanoProps)if(locate(prop.p).s>4000)prop.p.y=sceneryHeight(prop.p)-.015f;
 std::erase_if(volcanoProps,[&](const VolcanoProp& prop){
   const auto& b=volcanoBounds[prop.kind];SpriteRect rect{b[0]/1254,b[1]/1254,b[2]/1254,b[3]/1254};
   auto contact=spriteContact("caldeira-props-v24",rect);float radius=prop.h*contact.aspect*std::max(contact.u,1-contact.u);
   return !sceneryPlantClear(prop.p,radius)||locate(prop.p).distance<5.65f+radius;
 });
 result.segmentOffsets.push_back(Uint32(m.size()));return result;
}
void volcanoSprite(Mesh& m,V3 p,V3 camera,float width,float height,int kind,float fade=1){
 (void)width;const auto& b=volcanoBounds[kind];
 SpriteRect rect{b[0]/1254,b[1]/1254,b[2]/1254,b[3]/1254};
 auto contact=spriteContact("caldeira-props-v24",rect);
 V3 across=norm(cross({0,1,0},norm(camera-p)));
 groundedArt(m,p,height,rect,contact,across,{0,1,0},29+fade*.4f);
}
void volcanoJet(Mesh& m,V3 p,V3 camera,float height,float time,float phase){
 float cycle=lavaPhase(time,phase),burst=lavaBurst(time,phase);
 int frame=cycle<2?0:cycle<3.4f?1:std::clamp(2+int((cycle-3.4f)/.38f),2,7);
 float u=(frame%4)*.25f,v=(frame/4)*.5f;
 V3 half=norm(cross({0,1,0},norm(V3{camera.x-p.x,0,camera.z-p.z})))*(height*.375f);
 // Same full-size billboard for every cel: frame padding carries the changing
 // eruption height, so the base never jumps and the pixels are never squashed.
 V3 base=p-V3{0,height*(28.f/512),0};
 artQuad(m,base-half+V3{0,height,0},base+half+V3{0,height,0},base+half,base-half,54,u,v,u+.25f,v+.5f);
 if(burst>.01f)for(int j=0;j<18;j++){
   float seed=float(j+23)+phase*31,age=std::fmod(time*1.1f+hash(seed)*1.4f,1.4f);
   float angle=hash(seed+8)*2*pi;
   V3 drop=p+V3{std::cos(angle)*age*height*.18f,height*(.3f+age*.65f-age*age*.48f),std::sin(angle)*age*height*.18f};
   V3 a=norm(cross({0,1,0},norm(camera-drop)))*(.04f+height*.009f);
   quad(m,drop-a+V3{0,height*.014f,0},drop+a+V3{0,height*.014f,0},drop+a,drop-a,{1,.37f+hash(seed)*.30f,.045f},53);
 }
}
void appendVolcano(Mesh& m,const Road& road,const Rider& r,V3 camera){
 for(const auto& prop:volcanoProps){
   float distance=length(prop.p-camera),far=prop.h<1.5f?330.f:650.f;
   float fade=clamp((far-distance)/(far*.28f),0,1);if(fade<=0)continue;
   volcanoSprite(m,prop.p,camera,prop.h*.75f,prop.h,prop.kind,fade);
 }
 for(const auto& vent:volcanoVents)if(length(vent.p-camera)<850)volcanoJet(m,vent.p,camera,vent.height,r.runTime,vent.phase);
 for(const auto& hazard:caldeiraHazards){
   if(hazard.kind!=HazardKind::LavaVent||std::abs(hazard.s-r.s)>500)continue;
   V3 p=road.surface(hazard.s,hazard.d)+V3{0,.04f,0};
   float phase=lavaPhase(r.runTime,hazard.phase),warn=phase>2&&phase<3.4f?(.4f+.6f*std::abs(std::sin(r.runTime*11))):0;
   float glow=std::max(lavaBurst(r.runTime,hazard.phase),warn);
   for(int k=0;k<12;k++){
     float a=k*2*pi/12,b=(k+1)*2*pi/12;
     V3 x=road.surface(hazard.s+std::sin(a)*hazard.radius,hazard.d+std::cos(a)*hazard.radius)+V3{0,.035f,0};
     V3 y=road.surface(hazard.s+std::sin(b)*hazard.radius,hazard.d+std::cos(b)*hazard.radius)+V3{0,.035f,0};
     triangle(m,p,x,y,{.30f+glow*.7f,.07f+glow*.34f,.025f},53);
   }
   if(phase>=3.4f&&phase<6.1f)volcanoJet(m,p,camera,3.4f,r.runTime,hazard.phase);
 }
 // Sparse airborne embers move through world space and disappear behind walls.
 for(int j=0;j<64;j++){
   float seed=float(j+71),at=r.s-10+hash(seed)*130;
   V3 p=road.surface(at,(hash(seed+3)-.5f)*26)+V3{0,1+std::fmod(r.runTime*(.4f+hash(seed)) + hash(seed+7)*15,15.f),0};
   V3 half=norm(cross({0,1,0},norm(camera-p)))*.025f;
   quad(m,p-half+V3{0,.04f,0},p+half+V3{0,.04f,0},p+half,p-half,{.9f,.28f+hash(seed)*.3f,.04f},53);
 }
}
}

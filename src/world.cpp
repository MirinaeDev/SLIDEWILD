#include "runtime_paths.hpp"
#include "ui.hpp"
#include "rig_data.hpp"
#include "world.hpp"
#include "biome_data.hpp"
#include "detail_data.hpp"
#include "sprite_contact.hpp"
#include <cstdio>
#include <unordered_map>
#include <array>
#include <fstream>
#include <filesystem>
#include <stdexcept>
namespace dh {
namespace {
struct TerrainField {
 float x=0,z=0,cell=2.5f;int nx=0,nz=0;std::vector<float> heights;
 float height(V3 p)const{
   if(heights.empty())return p.y;
   float u=clamp((p.x-x)/cell,0,float(nx-1)-.001f),v=clamp((p.z-z)/cell,0,float(nz-1)-.001f);
   int ix=int(u),iz=int(v);u-=ix;v-=iz;
   float a=heights[iz*nx+ix],b=heights[(iz+1)*nx+ix],c=heights[(iz+1)*nx+ix+1],d=heights[iz*nx+ix+1];
   return u<v?a+(b-a)*v+(c-b)*u:a+(d-a)*u+(c-d)*v;
 }
} terrainField;
// Conservative footprints built from the actual closed rock mesh, including
// its buried floor. A rotating billboard must fit with its entire half-width.
struct RockBounds {float x0,z0,x1,z1;};
std::unordered_map<long long,std::vector<RockBounds>> rockCells;
struct PlantKey {long long position;int radius;bool operator==(const PlantKey&)const=default;};
struct PlantHash {size_t operator()(const PlantKey& k)const{return std::hash<long long>{}(k.position)^(std::hash<int>{}(k.radius)*0x9e3779b9u);}};
std::unordered_map<PlantKey,bool,PlantHash> plantClearCache;
std::unordered_map<long long,float> plantHeightCache;
std::unordered_map<unsigned long long,RoadLocation> locations;
struct GrassInstance {float at,offset,width,seed,far;int variant;V3 root,tint;bool flip;};
struct ShrubInstance {V3 root;float height,width;bool flip;};
std::vector<GrassInstance> grassInstances;
std::vector<ShrubInstance> shrubInstances;
struct GroundFace {V3 a,e,f;float inverse;};
std::unordered_map<long long,std::vector<GroundFace>> vergeCells;
long long cellKey(int x,int z){return static_cast<long long>((static_cast<unsigned long long>(static_cast<unsigned>(x))<<32)|static_cast<unsigned>(z));}
void indexRocks(const Mesh& mesh){
 rockCells.clear();
 plantClearCache.clear();plantHeightCache.clear();grassInstances.clear();shrubInstances.clear();locations.clear();
 for(size_t i=0;i<mesh.size();i+=3){
   if(mesh[i].material!=18&&mesh[i].material!=50&&mesh[i].material!=57)continue;
   V3 a=mesh[i].p,b=mesh[i+1].p,c=mesh[i+2].p;
   RockBounds box{std::min({a.x,b.x,c.x}),std::min({a.z,b.z,c.z}),std::max({a.x,b.x,c.x}),std::max({a.z,b.z,c.z})};
   for(int x=int(std::floor(box.x0/12));x<=int(std::floor(box.x1/12));x++)
     for(int z=int(std::floor(box.z0/12));z<=int(std::floor(box.z1/12));z++)rockCells[cellKey(x,z)].push_back(box);
 }
}
void indexVerges(const Mesh& mesh,size_t first){
 vergeCells.clear();
 for(size_t i=first;i<mesh.size();i+=3){
   if(mesh[i].material!=1&&mesh[i].material!=3&&mesh[i].material!=51&&mesh[i].material!=28&&mesh[i].material!=31)continue;
   V3 a=mesh[i].p,b=mesh[i+1].p,c=mesh[i+2].p,e=b-a,f=c-a;
   float determinant=e.x*f.z-e.z*f.x;if(std::abs(determinant)<1e-6f)continue;
   int x0=int(std::floor(std::min({a.x,b.x,c.x})/4)),x1=int(std::floor(std::max({a.x,b.x,c.x})/4));
   int z0=int(std::floor(std::min({a.z,b.z,c.z})/4)),z1=int(std::floor(std::max({a.z,b.z,c.z})/4));
   for(int x=x0;x<=x1;x++)for(int z=z0;z<=z1;z++)vergeCells[cellKey(x,z)].push_back({a,e,f,1/determinant});
 }
}
}
float sceneryHeight(V3 p){
 float height=terrainField.height(p);
 auto cell=vergeCells.find(cellKey(int(std::floor(p.x/4)),int(std::floor(p.z/4))));
 if(cell!=vergeCells.end())for(const auto& face:cell->second){
   V3 q=p-face.a;
   float u=(q.x*face.f.z-q.z*face.f.x)*face.inverse,v=(face.e.x*q.z-face.e.z*q.x)*face.inverse;
   if(u>=-.0001f&&v>=-.0001f&&u+v<=1.0001f)height=std::max(height,face.a.y+face.e.y*u+face.f.y*v);
 }
 return height;
}
bool sceneryPlantClear(V3 p,float radius){
 radius+=.18f;
 for(int x=int(std::floor((p.x-radius)/12));x<=int(std::floor((p.x+radius)/12));x++)
   for(int z=int(std::floor((p.z-radius)/12));z<=int(std::floor((p.z+radius)/12));z++){
     auto it=rockCells.find(cellKey(x,z));if(it==rockCells.end())continue;
     for(const auto& b:it->second){float dx=p.x-clamp(p.x,b.x0,b.x1),dz=p.z-clamp(p.z,b.z0,b.z1);if(dx*dx+dz*dz<=radius*radius)return false;}
   }
 return true;
}

// Textured quads use source coordinates directly; source art is never pixelated again.
static void artQuad(Mesh& m,V3 a,V3 b,V3 c,V3 d,float material,float u0=0,float v0=0,float u1=1,float v1=1,V3 tint={1,1,1}){
 size_t base=m.size();quad(m,a,b,c,d,tint,material);
 const float uv[6][2]={{u0,v0},{u1,v0},{u1,v1},{u0,v0},{u1,v1},{u0,v1}};
 for(int k=0;k<6;k++){m[base+k].u=uv[k][0];m[base+k].v=uv[k][1];}
}
static void groundedArt(Mesh& m,V3 root,float height,SpriteRect rect,SpriteContact contact,V3 right,V3 up,float material,V3 tint={1,1,1},bool flip=false){
 float width=height*contact.aspect,u=flip?1-contact.u:contact.u;
 V3 bottom=root-up*(height*(1-contact.v)),left=bottom-right*(width*u),end=bottom+right*(width*(1-u));
 artQuad(m,left+up*height,end+up*height,end,left,material,flip?rect.x1:rect.x0,rect.y0,flip?rect.x0:rect.x1,rect.y1,tint);
}
static void sceneryContactShadow(Mesh& m,V3 root,V3 right,float width,float depth){
 V3 along=cross(right,{0,1,0});
 auto point=[&](float x,float z){V3 p=root+right*x+along*z;p.y=sceneryHeight(p)+.012f;return p;};
 artQuad(m,point(-width,-depth),point(width,-depth),point(width,depth),point(-width,depth),7);
}
static V3 woodlandGround(const Road&road,float along,float across){
 auto end=road.at(road.size());V3 f{std::sin(end.heading),0,std::cos(end.heading)},q{f.z,0,-f.x};
 V3 p=end.p+f*along+q*across;
 float rise=clamp((along-6)/10,0,1);
 p.y=end.p.y-.04f+clamp((along-6)*.15f,0,10)+rise*(.65f*std::sin(across*.09f+along*.06f)+.3f*std::sin(across*.23f));
 return p;
}
#include "biomes.inc"
#include "future.inc"
#include "volcano.inc"
StaticWorld buildWorld(const Road&road){if(road.course==6)return buildFutureWorld(road);if(road.course==7)return buildVolcanoWorld(road);StaticWorld result;Mesh& m=result.vertices;m.reserve(60000);
 // A world-space heightfield cannot fold through a hairpin as extruded ribbons
 // do. Keep a corridor for the precisely banked road and its short verge mesh.
 float minX=1e6f,maxX=-1e6f,minZ=1e6f,maxZ=-1e6f;
 for(const auto& p:road.points){minX=std::min(minX,p.p.x);maxX=std::max(maxX,p.p.x);minZ=std::min(minZ,p.p.z);maxZ=std::max(maxZ,p.p.z);}
 const float cell=road.biome()?3.5f:2.5f;minX=std::floor((minX-80)/cell)*cell;minZ=std::floor((minZ-80)/cell)*cell;
 int nx=int((maxX+80-minX)/cell)+1,nz=int((maxZ+80-minZ)/cell)+1;
 struct LandVertex {V3 p;float distance;};std::vector<LandVertex> land(size_t(nx)*nz);
 for(int z=0;z<nz;z++)for(int x=0;x<nx;x++){
   V3 p{minX+x*cell,0,minZ+z*cell};auto loc=road.locate(p);
   double sum=0,weights=0;
   // A hard radius discarded whole road samples at once and made distant
   // hills jump tens of metres. Relative Gaussian weights remain well scaled
   // even far from the road; their support tapers continuously to zero.
   float nearestSquared=loc.distance*loc.distance;
   float support=48.f+nearestSquared*.48f;
   for(size_t j=0;j<road.points.size();j+=4){
     const auto& anchor=road.points[j];V3 delta=p-anchor.p;delta.y=0;float ds=dot(delta,delta);
     float relative=std::max(0.f,(ds-nearestSquared)/support);
     if(relative>16)continue;
     double weight=std::exp(-double(relative));
     float fall=road.biome()==1?.035f:road.biome()==3?.065f:road.biome()==4?.045f:.28f;
     float h=anchor.p.y-std::min(std::sqrt(ds)*fall,road.biome()==1?5.f:road.biome()>=3?10.f:28.f);
     sum+=h*weight;weights+=weight;
   }
   float relief=clamp((loc.distance-14)/40,0,1);
   float folds=2.8f*std::sin(p.x*.043f+p.z*.017f)+1.6f*std::sin(p.z*.071f-p.x*.021f);
   p.y=float(sum/std::max(weights,1e-30))-.12f+relief*folds;land[z*nx+x]={p,loc.distance};
 }
 terrainField={minX,minZ,cell,nx,nz,{}};terrainField.heights.reserve(land.size());
 for(const auto& v:land)terrainField.heights.push_back(v.p.y);
 for(int z=0;z+1<nz;z++)for(int x=0;x+1<nx;x++){
   auto a=land[z*nx+x],b=land[(z+1)*nx+x],c=land[(z+1)*nx+x+1],d=land[z*nx+x+1];
   if(std::min({a.distance,b.distance,c.distance,d.distance})<6.8f)continue;
   quad(m,a.p,b.p,c.p,d.p,{.27f,.31f,.16f},3);
 }
 if(road.biome()==4){
   float sea=road.at(Road::finishDistance).p.y-5;
   quad(m,{minX-250,sea,minZ-250},{minX-250,sea,maxZ+1000},{maxX+1000,sea,maxZ+1000},{maxX+1000,sea,minZ-250},{1,1,1},34);
 }
 // Continuous solid escarpments. Every longitudinal ring is shared by its
 // neighbours; both ends and the underground bottom are closed. No alpha-cut
 // illustrated planes or independent corner caps can expose the empty interior.
 auto rockNoise=[](float x){float k=std::floor(x),f=x-k;f=f*f*(3-2*f);return mix(hash(k+127),hash(k+128),f);};
 if(road.biome()==0)for(auto interval:{std::pair{330.f,590.f},std::pair{890.f,1130.f},std::pair{1620.f,1850.f},std::pair{2440.f,2660.f}}){
   constexpr int count=13;std::vector<std::array<V3,count>> rings;
   for(float s=interval.first;s<=interval.second+.1f;s+=2){
     std::array<V3,count> ring;
     float weight=cliffWeight(s),base=road.at(s).p.y;
     float ridge=(5.5f+rockNoise(s*.065f)*5.7f+rockNoise(s*.23f)*1.8f)*weight;
     float frontage=7.8f+rockNoise(s*.18f)*1.1f;
     for(int j=0;j<=8;j++){
       float v=j/8.f;
       // Coherent shelves and vertical fracture columns, rather than a smooth
       // sloping curtain. Shared rings still keep every edge watertight.
       float ledge=rockNoise(s*.14f+j/2*9.1f)*.92f+(j%2?.46f:-.16f)
                  +std::sin(v*13+s*.033f)*.17f;
       ring[j]=road.surface(s,-frontage-v*1.8f-ledge);
       ring[j].y=base-2+v*(ridge+2);
     }
     ring[9]=road.surface(s,-12.2f-rockNoise(s*.17f));ring[9].y=base+ridge*.80f;
     ring[10]=road.surface(s,-15.0f);ring[10].y=std::min(base+ridge*.35f,terrainField.height(ring[10])+.25f);
     ring[11]=ring[10];ring[11].y=std::min(base-4,terrainField.height(ring[11])-2);
     ring[12]=ring[0];ring[12].y=ring[11].y;
     rings.push_back(ring);
   }
   for(size_t k=0;k+1<rings.size();k++)for(int j=0;j<count;j++){
     int next=(j+1)%count;
     quad(m,rings[k][j],rings[k][next],rings[k+1][next],rings[k+1][j],{1,1,1},18);
   }
   for(int end:{0,int(rings.size()-1)}){
     const auto& ring=rings[end];V3 center{};for(auto p:ring)center=center+p;center=center*(1.f/count);
     for(int j=0;j<count;j++){
       int next=(j+1)%count;
       if(end==0)triangle(m,center,ring[next],ring[j],{1,1,1},18);
       else triangle(m,center,ring[j],ring[next],{1,1,1},18);
     }
   }
 }



 // Independent closed buttresses interrupt the long cliff silhouette. They
 // share the opaque world-space material but have different sizes and fractures.
 if(road.biome()==0)for(int rock=0;rock<int(Road::finishDistance/16.7f);rock++){
   float at=rock*16.7f+hash(float(rock+901))*5;if(cliffWeight(at)<.5f)continue;
   auto rr=road.at(at);V3 f{std::sin(rr.heading),0,std::cos(rr.heading)},q{f.z,0,-f.x};
   V3 root=road.surface(at,-9.6f);root.y-=1.5f;
   float height=4.5f+hash(float(rock+78))*6,wide=2.7f+hash(float(rock+22))*2.5f;
   constexpr int sides=9,levels=5;std::array<V3,sides*levels> ring;
   for(int y=0;y<levels;y++)for(int x=0;x<sides;x++){
     float v=(y+1.f)/(levels+1),angle=x*2*pi/sides;
     float radius=std::sin(v*pi)*(.82f+hash(float(rock*173+x*7+y))*.25f);
     ring[y*sides+x]=root+f*(std::cos(angle)*wide*radius)+q*(std::sin(angle)*1.95f*radius)+V3{0,v*height,0};
   }
   for(int y=0;y+1<levels;y++)for(int x=0;x<sides;x++){
     int next=(x+1)%sides;
     quad(m,ring[y*sides+x],ring[y*sides+next],ring[(y+1)*sides+next],ring[(y+1)*sides+x],{1,1,1},18);
   }
   for(int x=0;x<sides;x++){
     int next=(x+1)%sides;
     triangle(m,root,ring[next],ring[x],{1,1,1},18);
     triangle(m,root+V3{0,height,0},ring[(levels-1)*sides+x],ring[(levels-1)*sides+next],{1,1,1},18);
   }
 }

 if(road.biome())buildBiomeArchitecture(m,road);
 for(int i=0;i<int(road.points.size())-1;i++){
   result.segmentOffsets.push_back(Uint32(m.size()));
   float s=i*Road::step,t=s+Road::step;
   V3 a=road.surface(s,-Road::halfWidth),b=road.surface(t,-Road::halfWidth),c=road.surface(t,Road::halfWidth),d=road.surface(s,Road::halfWidth);
   size_t begin=m.size();quad(m,a,b,c,d,{1,1,1},1);
   float uv[6][2]={{-Road::halfWidth,s},{-Road::halfWidth,t},{Road::halfWidth,t},{-Road::halfWidth,s},{Road::halfWidth,t},{Road::halfWidth,s}};
   for(int k=0;k<6;k++){m[begin+k].u=uv[k][0];m[begin+k].v=uv[k][1];}
   if(road.biome()==2&&i%180==90&&std::abs(wrap(road.at(t).heading-road.at(s).heading))<.012f){
     for(int stripe=0;stripe<9;stripe++){
       float x=-4.1f+stripe*.95f;V3 up{0,.02f,0};
       quad(m,road.surface(s,x)+up,road.surface(t,x)+up,road.surface(t,x+.46f)+up,road.surface(s,x+.46f)+up,{.70f,.69f,.62f},0);
     }
   }
   for(int side:{-1,1}){
     if(road.biome()==2&&s<Road::finishDistance){
       auto pavement=[&](float at,float d){return road.surface(at,side*d)+V3{0,.16f,0};};
       quad(m,pavement(s,4.85f),pavement(t,4.85f),pavement(t,cityTerraceWidth(t)),pavement(s,cityTerraceWidth(s)),{1,1,1},31);
       V3 outerA=pavement(s,cityTerraceWidth(s)),outerB=pavement(t,cityTerraceWidth(t));
       V3 lowA=outerA,lowB=outerB;lowA.y=terrainField.height(lowA)-.2f;lowB.y=terrainField.height(lowB)-.2f;
       artQuad(m,outerA,outerB,lowB,lowA,16,s*.12f,0,t*.12f,1,{.60f,.61f,.65f});
       quad(m,road.surface(s,side*4.85f),road.surface(t,side*4.85f),pavement(t,4.85f),pavement(s,4.85f),{.60f,.57f,.53f},0);
     }
     V3 e=road.surface(s,side*5.3f),f=road.surface(t,side*5.3f);
     quad(m,road.surface(s,side*4.7f),road.surface(t,side*4.7f),f,e,{.45f,.41f,.28f},3);
     V3 previousA=e,previousB=f;
     constexpr float limit=12;
     for(float off=6;off<=limit;off+=1){
       V3 nextA=terrainPoint(road,s,side*off),nextB=terrainPoint(road,t,side*off);
       float blend=clamp((off-7)/4,0,1);
       if(road.biome()){
         nextA=road.surface(s,side*off);nextB=road.surface(t,side*off);
         // Urban pavement is a terrace. An alpine uphill verge would rise
         // through it; the desert also needs a low shoulder, not a grass bank.
         if(road.biome()==2)blend=clamp(off-11,0,1);
       }
       nextA.y=mix(nextA.y,terrainField.height(nextA)+.02f,blend);nextB.y=mix(nextB.y,terrainField.height(nextB)+.02f,blend);
       quad(m,previousA,previousB,nextB,nextA,{.27f,.31f,.16f},3);
       previousA=nextA;previousB=nextB;
     }

   }
   // One broad, recessed steel ribbon. Unequal folds reproduce the painted
   // reference silhouette instead of six equally bright parallel stripes.
   bool desertRail=road.biome()!=1||(s>1120&&s<1350)||(s>2400&&s<2580);
   bool coastalRail=road.biome()<3||std::fmod(s,road.biome()==3?560.f:730.f)<180;
   if(s<Road::finishDistance+16&&(road.biome()!=2||s>2800)&&desertRail&&coastalRail){
   V3 p=road.surface(s,5.0f),q=road.surface(t,5.0f);
   auto ra=road.at(s),rb=road.at(t);V3 oa{std::cos(ra.heading),0,-std::sin(ra.heading)},ob{std::cos(rb.heading),0,-std::sin(rb.heading)};
   const float ys[]={.48f,.52f,.62f,.68f,.80f,.89f,.94f};
   const float xs[]={.035f,-.09f,-.12f,.025f,.025f,-.14f,-.10f};
   const V3 steel[]={{.235f,.235f,.265f},{.40f,.40f,.44f},{.31f,.31f,.36f},
                     {.16f,.16f,.20f},{.32f,.32f,.36f},{.51f,.49f,.48f}};
   for(int j=0;j<6;j++){
     artQuad(m,p+oa*xs[j]+V3{0,ys[j],0},q+ob*xs[j]+V3{0,ys[j],0},q+ob*xs[j+1]+V3{0,ys[j+1],0},p+oa*xs[j+1]+V3{0,ys[j+1],0},15,s,ys[j],t,ys[j+1],steel[j]);
   }
   artQuad(m,p-oa*.10f+V3{0,.94f,0},q-ob*.10f+V3{0,.94f,0},q+ob*.13f+V3{0,.92f,0},p+oa*.13f+V3{0,.92f,0},15,s,.94f,t,1.13f,{.55f,.52f,.48f});
   artQuad(m,p+oa*.13f+V3{0,.92f,0},q+ob*.13f+V3{0,.92f,0},q+ob*.13f+V3{0,.48f,0},p+oa*.13f+V3{0,.48f,0},15,s,0,t,.44f,{.235f,.24f,.27f});
   if(i%2==0){
     V3 post=p+oa*.03f;
     box(m,post+V3{0,.43f,0},{.19f,.86f,.14f},{.22f,.225f,.25f},ra.heading,15);
     box(m,post-oa*.10f+V3{0,.27f,0},{.035f,.54f,.19f},{.35f,.34f,.35f},ra.heading,15);
     box(m,p-oa*.025f+V3{0,.74f,0},{.075f,.045f,.045f},{.46f,.44f,.43f},ra.heading,15);
   }
   if(i%8==0)box(m,p-oa*.15f+V3{0,.865f,0},{.026f,.07f,.15f},{.74f,.66f,.53f},ra.heading,15);
   }
   if(stoneWallAt(s)&&(road.biome()==0||road.biome()==3)){
     auto wall=[&](float at,float d,float h){return road.surface(at,d)+V3{0,h,0};};
     float ha=.91f+hash(float(i))*.08f,hb=.91f+hash(float(i+1))*.08f;
     artQuad(m,wall(s,-5.25f,0),wall(t,-5.25f,0),wall(t,-5.25f,hb),wall(s,-5.25f,ha),16,s/4,1,t/4,0,{.86f,.83f,.79f});
     artQuad(m,wall(s,-5.95f,0),wall(t,-5.95f,0),wall(t,-5.95f,hb),wall(s,-5.95f,ha),16,s/4,1,t/4,0,{.66f,.65f,.61f});
     artQuad(m,wall(s,-5.95f,ha),wall(t,-5.95f,hb),wall(t,-5.25f,hb),wall(s,-5.25f,ha),16,s/4,.1f,t/4,.8f,{1.f,.94f,.84f});
     if(!stoneWallAt(s-.1f)||!stoneWallAt(t))for(float end:{s,t})
       artQuad(m,wall(end,-5.95f,0),wall(end,-5.25f,0),wall(end,-5.25f,ha),wall(end,-5.95f,ha),16,0,1,.17f,0,{.73f,.72f,.68f});
   }
   // Protected entry to the hairpin, as in the reference: straw bales on the inside.
   if(road.biome()==0&&i%2==0&&hayBalesAt(s)){auto r=road.at(s);box(m,road.surface(s,-5.02f)+V3{0,.48f,0},{.66f,.96f,3.65f},{.52f,.36f,.17f},r.heading,10);}
   if(i%3==0){auto r=road.at(s);for(int side:{-1,1}){float offset=side*(4.85f+hash(float(i+14))*.36f);box(m,road.surface(s,offset)+V3{0,.025f,0},{.14f,.06f,.19f},{.42f,.39f,.31f},r.heading+float(i),0);}}
   if(s<Road::finishDistance&&(i%40==0||(i%8==0&&std::abs(wrap(road.at(s+2).heading-road.at(s).heading))>.03f))){auto r=road.at(s);V3 pos=road.surface(s,5.5f);box(m,pos+V3{0,1,0},{.07f,2,.07f},{.28f,.28f,.29f});box(m,pos+V3{0,1.6f,0},{1.0f,.85f,.10f},{.09f,.09f,.12f},r.heading);V3 right{std::cos(r.heading),0,-std::sin(r.heading)},front{std::sin(r.heading),0,std::cos(r.heading)};V3 o=pos+V3{0,1.6f,0}-front*.065f;if(wrap(road.at(s+12).heading-r.heading)>0)right=right*-1;
     quad(m,o+right*.23f+V3{0,.30f,0},o-right*.02f+V3{0,.30f,0},o-right*.28f,o-right*.03f,{.80f,.78f,.75f});quad(m,o-right*.28f,o-right*.03f,o+right*.23f-V3{0,.30f,0},o-right*.02f-V3{0,.30f,0},{.80f,.78f,.75f});}
 }
 // Finish gantry and an actual broad, level parking apron beyond the line.
 if(road.course==5){
   // Track furniture follows the bank; upright posts stand beyond the rideable
   // edge and each transverse beam clears the highest side of the surface.
   for(float at:{120.f,1000.f,2000.f,3000.f,3890.f}){
     auto rr=road.at(at);float top=rr.p.y+std::abs(rr.bank)*5.7f+4.1f;
     for(int side:{-1,1}){
       auto foot=road.surface(at,side*5.8f);float height=top-foot.y;
       box(m,foot+V3{0,height*.5f,0},{.22f,height,.22f},{.17f,.24f,.27f},rr.heading,0);
       box(m,foot+V3{0,1.1f,0},{.32f,.75f,.32f},{.92f,.30f,.15f},rr.heading,0);
     }
     V3 center=rr.p;center.y=top;box(m,center,{12.1f,.35f,.28f},{.22f,.79f,.69f},rr.heading,0);
     for(int j=-5;j<=5;j++){
       V3 p=road.surface(at,float(j));p.y=top-.48f;
       V3 q{std::cos(rr.heading)*.30f,0,-std::sin(rr.heading)*.30f};
       triangle(m,p-q,p+q,p+V3{0,-.65f,0},j%2?V3{.94f,.76f,.41f}:V3{.9f,.27f,.14f},0);
     }
   }
 }
 const float finish=Road::finishDistance;auto finishRoad=road.at(finish);
 auto fp=[&](float s,float d,float h=0.f){return road.surface(s,d)+V3{0,h,0};};
 for(int side:{-1,1}){
   box(m,fp(finish,side*5.25f,3.2f),{.50f,6.4f,.5f},{.07f,.10f,.17f},finishRoad.heading,15);
   for(int stripe=0;stripe<8;stripe++)box(m,fp(finish-.01f,side*5.25f,.3f+stripe*.6f),{.54f,.27f,.54f},stripe%2?V3{.95f,.71f,.30f}:V3{.90f,.89f,.80f},finishRoad.heading,0);
 }
 box(m,fp(finish,0,5.2f),{11.3f,2.5f,.3f},{.045f,.06f,.105f},finishRoad.heading,0);
 artQuad(m,fp(finish-.18f,-5.5f,6.42f),fp(finish-.18f,5.5f,6.42f),fp(finish-.18f,5.5f,3.98f),fp(finish-.18f,-5.5f,3.98f),26,0,2.f/3,1,1);
 for(int row=0;row<3;row++)for(int tile=0;tile<16;tile++){
   float x=-4.7f+tile*.5875f,z=finish+row*.59f;
   quad(m,fp(z,x,.024f),fp(z+.59f,x,.024f),fp(z+.59f,x+.5875f,.024f),fp(z,x+.5875f,.024f),(row+tile)%2?V3{.08f,.09f,.12f}:V3{.95f,.91f,.78f},0);
 }
 for(float at=finish+18;at<finish+156;at+=2){
   size_t first=m.size();quad(m,fp(at,-15,-.015f),fp(at+2,-15,-.015f),fp(at+2,21,-.015f),fp(at,21,-.015f),{1,1,1},28);
   const float uv[6][2]={{-15,at},{-15,at+2},{21,at+2},{-15,at},{21,at+2},{21,at}};
   for(int k=0;k<6;k++){m[first+k].u=uv[k][0];m[first+k].v=uv[k][1];}
 }
 // Angled painted bays, wheel stops and warm-lit bollards on the perimeter.
 for(int bay=0;bay<13;bay++){
   float at=finish+25+bay*8;
   for(int side:{-1,1}){
     float inner=side*7.3f,outer=side*13.5f;
     quad(m,fp(at,inner,.02f),fp(at+.13f,inner,.02f),fp(at+2.2f,outer,.02f),fp(at+2.07f,outer,.02f),{.78f,.75f,.63f},0);
     box(m,fp(at+3.3f,side*13.6f,.09f),{.32f,.18f,1.9f},{.54f,.48f,.34f},finishRoad.heading,0);
     if(bay%3==0){box(m,fp(at,side*16,.55f),{.20f,1.1f,.20f},{.14f,.16f,.20f},finishRoad.heading,15);box(m,fp(at,side*16,1.08f),{.25f,.13f,.25f},{1,.73f,.35f},finishRoad.heading,9);}
   }
 }
 // Close the road's far end with a continuous planted bank. Its first ring
 // meets the last asphalt edge; the raised rings start beyond the road corridor.
 for(float z=0;z<140;z+=4)for(float x=-130;x<130;x+=4)
   quad(m,woodlandGround(road,z,x),woodlandGround(road,z+4,x),woodlandGround(road,z+4,x+4),woodlandGround(road,z,x+4),{.23f,.29f,.13f},3);
 for(int post=0;post<=14;post++){
   float x=-21+post*3;V3 p=fp(finish+149,x);
   box(m,p+V3{0,.55f,0},{.17f,1.1f,.17f},{.38f,.28f,.17f},finishRoad.heading,2);
   if(post<14)for(float h:{.35f,.82f})box(m,fp(finish+149,x+1.5f,h),{3.1f,.14f,.14f},{.47f,.34f,.20f},finishRoad.heading,2);
 }
 result.segmentOffsets.push_back(Uint32(m.size()));indexRocks(m);indexVerges(m,result.segmentOffsets.front());return result;
}
void appendTrees(Mesh&m,const Road&road,const Rider&r,V3 camera){
 if(road.course==7){appendVolcano(m,road,r,camera);return;}
 if(road.course==6){appendFutureProps(m,camera);return;}
 if(road.biome()){appendBiomeProps(m,road,camera);return;}
 // A deep, irregular forest closes the parking sightline. Every tree has its
 // own grounded billboard; gaps expose a second/third row and a solid bank.
 auto canopy=[&](V3 p,float height,int kind){
   if(length(p-camera)>540)return;
   const float bounds[4][4]={{80,24,622,706},{658,150,1220,708},{24,802,630,1224},{683,730,1228,1224}};
   const auto& b=bounds[kind];SpriteRect rect{b[0]/1254,b[1]/1254,b[2]/1254,b[3]/1254};
   auto contact=spriteContact("vegetation-v2",rect);p.y=sceneryHeight(p)-.025f;
   V3 right=norm(cross({0,1,0},norm(camera-p)));
   groundedArt(m,p,height,rect,contact,right,{0,1,0},13.4f);
 };
 if(length(road.at(road.size()).p-camera)<520){
   for(int side:{-1,1})for(int j=0;j<57;j++)for(int band=0;band<2;band++){
     float at=Road::finishDistance+18+j*2.4f+hash(float(j*7+band));
     V3 p=road.surface(at,side*(5.7f+band*.9f));p.y=sceneryHeight(p)-.02f;
     float h=.4f+hash(float(j+side*31+band))*.28f;
     SpriteRect rect{37.f/1254,785.f/1254,609.f/1254,1191.f/1254};
     V3 right=norm(cross({0,1,0},norm(camera-p)));
     groundedArt(m,p,h,rect,spriteContact("grass-v3",rect),right,{0,1,0},20.4f);
   }
   for(int band=3;band>=0;band--)for(int j=0;j<27;j++){
     float seed=float(j+band*47),x=-117+j*9+hash(seed+41)*5,z=9+band*20+hash(seed+52)*8;
     canopy(woodlandGround(road,z,x),16+hash(seed+27)*16, j%5==0?1:(j%7==0?3:0));
   }
   for(int band=0;band<3;band++)for(int j=0;j<50;j++){
     float x=-113+j*4.6f+hash(float(j+band*29))*2,z=3+band*6+hash(float(j+91))*3;
     canopy(woodlandGround(road,z,x),1.7f+hash(float(j*3+band))*1.9f,2);
   }
   for(int side:{-1,1})for(int j=0;j<14;j++){
     float at=Road::finishDistance+65+j*6;V3 p=road.surface(at,side*(33+hash(float(j*7+side))*11));p.y=sceneryHeight(p);
     canopy(p,12+hash(float(j+11))*9,j%3==0?1:0);
     V3 hedge=road.surface(at,side*24);hedge.y=sceneryHeight(hedge);canopy(hedge,2.3f+hash(float(j+2)),2);
   }
 }
 if(r.s>Road::finishDistance-450){
   // Painted finish village: vehicles, refreshments, spectators and picnic bays.
   const float bounds[4][4]={{45,64,790,536},{825,29,1465,520},{65,541,750,980},{813,580,1500,982}};
   for(int j=0;j<18;j++){
     int kind=j%4;float at=Road::finishDistance+26+(j/2)*12.f;
     float side=j%2?1.f:-1.f;float offset=side*(kind==2?8.5f:12.f);
     V3 root=road.surface(at,offset);root.y=sceneryHeight(root);
     const auto& b=bounds[kind];SpriteRect rect{b[0]/1536,b[1]/1024,b[2]/1536,b[3]/1024};
     auto contact=spriteContact("parking-props-v16",rect);
     const float heights[]={2.8f,3.0f,1.85f,2.05f};float h=heights[kind],w=h*contact.aspect;
     V3 toward=norm(camera-root),across=norm(cross({0,1,0},toward)),up=norm(cross(toward,across));
     sceneryContactShadow(m,root,across,w*.40f,kind==2?.25f:.65f);
     groundedArt(m,root,h,rect,contact,across,up,27);
   }
 }
 // Cached global footprint checks also reject trees from the opposite side of
 // a switchback whose wide billboard would otherwise intrude into this road.
 auto location=[&](V3 p){
   auto x=static_cast<unsigned>(int(std::round(p.x*100))),z=static_cast<unsigned>(int(std::round(p.z*100)));
   unsigned long long key=(static_cast<unsigned long long>(x)<<32)|z;
   auto it=locations.find(key);if(it!=locations.end())return it->second;
   auto value=road.locate(p);locations.emplace(key,value);return value;
 };
 auto allowed=[&](V3 p,float radius){
   PlantKey key{cellKey(int(std::round(p.x*100)),int(std::round(p.z*100))),int(std::ceil(radius*100))};
   auto cached=plantClearCache.find(key);if(cached!=plantClearCache.end())return cached->second;
   auto loc=location(p);float safe=std::abs(clearPlantOffset(loc.s,loc.d<0?-1:1,radius,0));
   bool clear=!(loc.s>Road::finishDistance&&loc.distance<24+radius)&&loc.distance>=safe-.005f&&sceneryPlantClear(p,radius+.01f);
   plantClearCache.emplace(key,clear);return clear;
 };
 auto grounded=[&](V3 p){
   auto key=cellKey(int(std::round(p.x*100)),int(std::round(p.z*100)));
   auto cached=plantHeightCache.find(key);if(cached!=plantHeightCache.end()){p.y=cached->second;return p;}
   p.y=sceneryHeight(p)-.025f;
   plantHeightCache.emplace(key,p.y);return p;
 };
 auto groundNear=[&](V3 foot){foot.y=sceneryHeight(foot)-.025f;return foot;};
 auto visibility=[&](V3 p,float far){
   V3 f=norm(V3{r.p.x-camera.x,0,r.p.z-camera.z});
   V3 delta=p-camera;float radius=20;
   if(dot(delta,f)<-radius)return 0.f;
   float distance=length(delta);return clamp((far-distance)/(far*.28f),0,1);
 };
 auto vegetation=[&](V3 p,float h,float w,int variant,bool flip=false){
   if(visibility(p,550)<=0)return;
   if(!allowed(p,w*.5f+h*.026f))return;
   p=grounded(p);
   float fade=visibility(p,550);if(fade<=0)return;
   const float bounds[4][4]={{80,24,622,706},{658,150,1220,708},{24,802,630,1224},{683,730,1228,1224}};
   const float* b=bounds[variant];
   V3 toward=norm(V3{camera.x-p.x,0,camera.z-p.z});
   V3 right=norm(cross({0,1,0},toward)),half=right*(w*.5f);
   float sway=std::sin(r.runTime*1.6f+p.x*.17f+p.z*.12f)*.025f*h;
   V3 wind=right*sway;
   artQuad(m,p-half+V3{0,h,0}+wind,p+half+V3{0,h,0}+wind,p+half,p-half,13+fade*.4f,
     b[flip?2:0]/1254,b[1]/1254,b[flip?0:2]/1254,b[3]/1254);
 };
 // Break broad meadow slabs into uneven copses with low bushes underneath.
 // Open intervals remain between the groups, so the lake is still visible.
 for(int grove=0;grove<int(Road::finishDistance/170);grove++){
   float s=70+grove*170+hash(float(grove+37))*63;
   if(cliffWeight(s)>.5f)continue;
   int side=grove%3==0?-1:1;float d=side*(33+hash(float(grove+28))*34);
   V3 center=terrainPoint(road,s,d);if(visibility(center,480)<=0)continue;
   for(int j=0;j<11;j++){
     float at=s+(hash(float(grove*43+j))-.5f)*35,off=d+(hash(float(grove*13+j+77))-.5f)*21;
     V3 root=terrainPoint(road,at,off);float height=8+hash(float(j+grove*21))*14;
     vegetation(root,height,height*(j%3==0?.81f:.64f),j%3==0?1:(j%4==0?3:0),j%2);
     vegetation(terrainPoint(road,at+2,off+2),1.2f+hash(float(j+44)),3.8f,2,j%2);
   }
 }
 // Painted grass fans retain a readable pixel silhouette at 960x540.
 // Bottom corners follow the actual hillside; only the tips move in the wind.
 auto drawGrass=[&](const GrassInstance& instance){
   float at=instance.at,offset=instance.offset,width=instance.width,far=instance.far;
   int variant=instance.variant;
   const float bounds[4][4]={{33,304,618,625},{657,143,1223,635},
                            {37,785,609,1191},{679,734,1216,1191}};
   const float* b=bounds[variant];V3 root=instance.root;
   V3 flat=root-camera;flat.y=0;
   float limit=std::abs(offset)<5.3f?100.f:far;
   if(dot(flat,flat)>limit*limit)return;
   float fade=visibility(root,std::abs(offset)<5.3f?100.f:far);if(fade<=0)return;
   V3 toward=norm(V3{camera.x-root.x,0,camera.z-root.z});
   V3 across=norm(cross({0,1,0},toward));
   float height=width*(b[3]-b[1])/(b[2]-b[0]);
   float sway=std::sin(r.runTime*1.8f+at*.31f+offset*.6f)*.055f;
   V3 left=root-across*(width*.5f),right=root+across*(width*.5f);
   // Both feet follow the rendered bank, including its transverse rise. The
   // old road-only slope buried half the grass cards inside steep verges.
   left=groundNear(left);right=groundNear(right);
   bool flip=instance.flip;V3 tint=instance.tint;
   for(int strip=0;strip<2;strip++){
     float low=strip*.5f,high=low+.5f;
     auto point=[&](V3 foot,float h){return foot+V3{0,height*h,0}+across*(sway*h*h);};
     artQuad(m,point(left,high),point(right,high),point(right,low),point(left,low),20+fade*.4f,
       b[flip?2:0]/1254,mix(b[3],b[1],high)/1254,
       b[flip?0:2]/1254,mix(b[3],b[1],low)/1254,tint);
   }
 };
 // Six additional silhouettes, distributed in patches rather than a repeated hedge.
 const float floraBounds[6][4]={{27,33,510,484},{545,167,1005,483},{1026,195,1516,486},
                              {62,506,455,998},{539,537,986,990},{1049,577,1504,994}};
 for(int i=0;i<int(Road::finishDistance/3.4f);i++)for(int side:{-1,1}){
   float at=i*3.4f+hash(float(i+side+30))*2.3f;
   if(hash(float(i*3+side+9))<.30f)continue;
   int kind=(i+int(hash(float(i+13))*4))%6;
   float width=(kind==3?2.4f:kind==4?1.1f:1.0f)+hash(float(i+72))*.8f;
   float offset=clearPlantOffset(at,side,width*.5f+.09f,side*(6+hash(float(i*2+side+13))*12));
   V3 p=terrainPoint(road,at,offset);if(!allowed(p,width*.5f+.09f))continue;
   p=grounded(p);float fade=visibility(p,400);if(fade<=0)continue;
   const float* b=floraBounds[kind];float height=width*(b[3]-b[1])/(b[2]-b[0]);
   V3 toward=norm(V3{camera.x-p.x,0,camera.z-p.z}),across=norm(cross({0,1,0},toward));
   V3 half=across*(width*.5f),wind=across*(kind==4?0.f:std::sin(r.runTime*1.6f+at)*.04f);
   bool flip=i%2;V3 tint=mix(V3{.73f,.77f,.72f},V3{.94f,.94f,.88f},hash(float(i+26)));
   V3 footLeft=groundNear(p-half),footRight=groundNear(p+half);
   artQuad(m,footLeft+V3{0,height,0}+wind,footRight+V3{0,height,0}+wind,footRight,footLeft,21+fade*.4f,
     b[flip?2:0]/1536,b[1]/1024,b[flip?0:2]/1536,b[3]/1024,tint);
 }
 for(int i=0;i<int(Road::finishDistance/4.2f);i++){
   float s=i*4.2f;
   // Forest clusters alternate with clear windows onto the lake.
   for(int side:{-1,1}){
     if(side>0&&(hash(float(i*7))>.45f||i%17>10))continue;
     if(side<0&&cliffWeight(s)<.2f&&i%4!=0)continue;
     float off=side<0?-9.f-hash(float(i+31))*7:10+hash(float(i+7))*12;
     float h=side<0?8+hash(float(i))*8:12+hash(float(i+8))*12;
     off=clearPlantOffset(s,side,h*.46f,off);
     V3 p=terrainPoint(road,s,off);p.y-=.2f;if(side<0)p.y+=3.8f*cliffWeight(s);
     vegetation(p,h,h*(i%4==1?.84f:.63f),i%4==1?1:(i%4==2?3:0),i%2);
   }
   if(i%2==0){
     for(int side:{-1,1}){V3 p=terrainPoint(road,s,clearPlantOffset(s,side,1.05f,side*5.7f));p.y-=.15f;
       vegetation(p,.7f+hash(float(i))*1.15f,2.1f,2,i%2);}
   }
 }

 // Descending woodland at three distances: actual world parallax and occlusion.
 for(int band=0;band<3;band++)for(int i=0;i<int(Road::finishDistance/6.1f);i++){
   float s=i*6.1f+hash(float(i+band*310))*5;

   if(hash(float(i*13+band))<.35f||i%12>5)continue;
   float off=14+band*18+hash(float(i+band*107))*14;
   V3 p=terrainPoint(road,s,off);
   float h=14+hash(float(i*3+band))*15;
   vegetation(p,h,h*(i%3==1?.9f:.65f),i%3==1?1:(i%3==2?3:0),i%2);
 }
 // Placement and appearance are immutable. Build this population once, then
 // only orient visible cards each frame; no per-frame procedural hash generation.
 if(grassInstances.empty()){
 auto grass=[&](float at,float offset,float width,int variant,float seed,float far=230.f){
   V3 root=grounded(terrainPoint(road,at,offset));
   if(std::abs(offset)>5.3f&&!allowed(root,width*.5f+.12f))return;
   V3 tint=mix(V3{.73f,.79f,.70f},V3{.97f,.96f,.87f},hash(seed+39));
   grassInstances.push_back({at,offset,width,seed,far,variant,root,tint,hash(seed+33)>.5f});
 };
 // A continuous but irregular roadside plant layer; never a bare green strip.
 for(int i=0;i<int(Road::finishDistance/1.05f);i++){
   float s=i*1.05f+hash(float(i))*1.1f;
   for(int side:{-1,1}){
     float plantWidth=1.0f+hash(float(i+90))*1.4f;
     float off=clearPlantOffset(s,side,plantWidth*.5f+.08f,side*(5.25f+hash(float(i*3+side+11))*2.2f));
     V3 p=terrainPoint(road,s,off);
     shrubInstances.push_back({p,.35f+hash(float(i+side+12))*.8f,plantWidth,bool(i%2)});
     for(int cover=0;cover<2;cover++){
       float seed=float(i*59+side*3+cover),width=.45f+hash(seed+12)*.22f;
       float offset=clearPlantOffset(s,side,width*.5f+.14f,side*(5.6f+hash(seed+27)*1.4f));
       grass(s+cover*.4f,offset,width,(i+cover)%4,seed);
     }
     for(int tuft=0;tuft<2;tuft++){
       float key=float(i*23+tuft),bs=s+hash(key)*.85f;
       // Small verge tufts stop before the rail/post footprint and masonry.
       float bd=side*(4.70f+hash(key+27)*.025f);
       grass(bs,bd,.22f+hash(key+49)*.06f,tuft%2,key);
     }
   }
 }
 // Dense painted clumps, irregular spacing and four silhouettes. Broad fans
 // replace subpixel blades; the distant bands are thinned before upload.
 for(int cell=0;cell<int(Road::finishDistance/1.7f);cell++){
   for(int side:{-1,1})for(int band=0;band<24;band++){
   float key=float(cell*71+band*7+side+2),s=cell*1.7f+hash(key)*1.35f;
   if(band>=18&&cell%2)continue;
   float width=(band<18?1.05f:1.75f)+hash(key+31)*.65f;
   float spread=band<18?band*1.6f:29+(band-18)*6.f;
   float d=clearPlantOffset(s,side,width*.5f+.14f,side*(5.6f+spread+hash(key+81)));
   int variant=hash(key+97)<.15f?3:(cell+band)%3;
   // Every third band is persistent far cover. The denser layer dissolves
   // gradually in world distance, never switches on at a route-coordinate line.
   grass(s,d,width,variant,key,band>=18?270.f:(band%3==0?230.f:145.f));
   if(band>=18&&cell%13==0)shrubInstances.push_back({terrainPoint(road,s,d),1.1f+hash(key+19)*.9f,2.5f,bool(cell%2)});
   }
 }
 }
 for(const auto& shrub:shrubInstances)vegetation(shrub.root,shrub.height,shrub.width,2,shrub.flip);
 for(const auto& grass:grassInstances)drawGrass(grass);
 // Meadow patches have low vegetation and flowers instead of another rock wall.
 for(int i=0;i<int(Road::finishDistance/3.5f);i++){
   float s=i*3.5f;if(cliffWeight(s)>.4f)continue;
   V3 area=road.at(s).p-camera;area.y=0;if(dot(area,area)>300*300)continue;
   float off=-9-hash(float(i+67))*16;V3 p=terrainPoint(road,s,off);
   if(i%3==0)vegetation(p,.5f+hash(float(i))*.7f,1.6f,2,i%2);
   for(int flower=0;flower<7;flower++){
     V3 root=grounded(terrainPoint(road,s+hash(float(i*7+flower))*2,off+hash(float(i+flower))));
     if(!allowed(root,.08f)||visibility(root,170)<=0)continue;
     V3 top=root+V3{0,.18f+hash(float(i+flower+44))*.15f,0};
     triangle(m,root-V3{.012f,0,0},root+V3{.012f,0,0},top,{.32f,.36f,.17f},9);
     V3 petal=flower%2?V3{.70f,.62f,.34f}:V3{.65f,.56f,.66f};
     quad(m,top+V3{-.025f,.018f,0},top+V3{.025f,.018f,0},top+V3{.025f,-.018f,0},top+V3{-.025f,-.018f,0},petal,9);
   }
 }
 // Small warm-lit hamlets below the road, separated by clearings.
 for(int i=0;i<48;i++){
   float s=i*31.f+12;if(i%7>2)continue;
   V3 p=terrainPoint(road,s,70+hash(float(i))*23);float heading=road.at(s).heading;
   box(m,p+V3{0,1.6f,0},{3.5f,3.2f,4.8f},{.49f,.43f,.34f},heading,2);
   V3 f{std::sin(heading),0,std::cos(heading)},q{f.z,0,-f.x};
   for(int side:{-1,1})quad(m,p+q*(side*2.f)+f*2.6f+V3{0,3.2f,0},p+f*2.6f+V3{0,4.5f,0},p-f*2.6f+V3{0,4.5f,0},p+q*(side*2.f)-f*2.6f+V3{0,3.2f,0},{.27f,.23f,.26f},2);
   box(m,p+q*1.77f+V3{0,1.9f,0},{.025f,.55f,.55f},{.88f,.64f,.32f},heading,9);
 }

}
Mesh buildRider(const Rider&r,const Road&road,V3 camera,bool tuck,bool slide){
 (void)tuck;(void)slide;Mesh m;
 RoadSurfaceQuery surfaceQuery(road,r.s,7);
 auto onRoad=[&](V3 p){if(!surfaceQuery.project(p))p=groundPoint(road,p,r.s,true);return p;};
 V3 toward=norm(camera-(r.p+V3{0,.6f,0})),right=norm(cross(toward,{0,1,0}));
 V3 up=norm(cross(right,toward));
 if(road.course>=5){
   auto track=road.at(r.s);V3 transverse{std::cos(track.heading),0,-std::sin(track.heading)};
   float angle=std::atan(track.bank)*dot(right,transverse);
   V3 oldRight=right;right=right*std::cos(angle)+up*std::sin(angle);
   up=up*std::cos(angle)-oldRight*std::sin(angle);
 }
 if(r.crashed){
   if(r.spinOut){float roll=r.driftDirection*.45f*std::sin(clamp(r.crashTime/.55f,0,1)*pi);V3 old=right;right=right*std::cos(roll)+up*std::sin(roll);up=up*std::cos(roll)-old*std::sin(roll);}
   int who=std::clamp(r.pilot,0,3),frame=r.crashTime<.25f?0:(r.crashTime<.55f?1:2);
   const SpriteRect originalFalls[3]={{92.f/2172,126.f/724,744.f/2172,628.f/724},{816.f/2172,259.f/724,1370.f/2172,599.f/724},{1501.f/2172,295.f/724,2132.f/2172,629.f/724}};
   auto b=who==0?originalFalls[frame]:(who==3?meiFallRects[frame]:rideRects[who][frame==0?6:7]);
   float sheetAspect=who==0||who==3?3.f:1.5f;
   float width=frame==0?1.75f:1.95f,height=width*(b.y1-b.y0)/((b.x1-b.x0)*sheetAspect);
   V3 root=r.p+V3{0,.016f,0};bool flip=r.crashSide<0;
   // The fall drawing already contains its foreshortening. Keep an orthonormal
   // camera-facing basis, exactly as for riding, instead of laying it down twice.
   artQuad(m,root-right*width*.5f+up*height,root+right*width*.5f+up*height,root+right*width*.5f,root-right*width*.5f,
     22+who*.1f,flip?b.x1:b.x0,b.y0,flip?b.x0:b.x1,b.y1);
   V3 f{std::sin(r.heading),0,std::cos(r.heading)},q{f.z,0,-f.x};
   auto ground=[&](V3 p){return onRoad(p)+V3{0,.019f,0};};
   artQuad(m,ground(root-f*.35f-q*.85f),ground(root-f*.35f+q*.85f),ground(root+f*1.05f+q*.85f),ground(root+f*1.05f-q*.85f),7);
   V3 board=ground(root-q*(r.crashSide*(1.1f+std::min(.8f,r.crashTime)*.5f))+f*(.15f+std::min(.8f,r.crashTime)*.35f));
   // Drawn roll/bounce poses retain their thickness and trucks. The drawing
   // contains the perspective: never project it flat onto the asphalt again.
   int boardFrame=r.crashTime<.12f?0:r.crashTime<.24f?1:r.crashTime<.36f?2:r.crashTime<.48f?3:(r.crashSide<0?4:5);
   auto bb=boardFallRects[boardFrame];
   float pixelScale=1.12f/400.f,bw=(bb.x1-bb.x0)*1536*pixelScale,bh=(bb.y1-bb.y0)*1024*pixelScale;
   float bounce=r.crashTime<.48f?.13f*std::abs(std::sin(r.crashTime*pi/.24f)):0;
   auto boardGround=[&](float x,float z){return ground(board+right*x+f*z);};
   artQuad(m,boardGround(-.53f,-.20f),boardGround(.53f,-.20f),boardGround(.53f,.20f),boardGround(-.53f,.20f),7);
   board=board+V3{0,bounce+.008f,0};
   const V3 wheelColor[4]={{1,.40f,.04f},{1,.19f,.26f},{1,.74f,.04f},{.58f,.32f,.86f}};
   artQuad(m,board-right*bw*.5f+up*bh,board+right*bw*.5f+up*bh,board+right*bw*.5f,board-right*bw*.5f,25,bb.x0,bb.y0,bb.x1,bb.y1,wheelColor[who]);
   return m;
 }
 // Atlas poses have different silhouettes, but share a measured wheel anchor.
 const int who=std::clamp(r.pilot,0,3);
 // Uniform scaling within each pose. No independent width/height
 // tween, no sliced billboards and no deformation of the painted anatomy.
 auto drawPose=[&](int pose){
   bool animated=pose==0||pose==5;
   // Fixed reference geometry and contact: only cloth texels animate.
   int cel=pose==5?3:0;
   auto b=animated?windRects[who][cel]:rigRects[who][pose];
   float u0=b.x0,u1=b.x1,v0=b.y0,v1=b.y1;
   const float poseHeight[riderPoseCount]={1,.98f,.98f,.73f,.73f,.90f,1.03f,.92f,.92f,.80f,.80f,.72f,.72f};
   auto scaleRect=animated?windRects[who][pose==5?3:0]:b;
   float metresPerPixel=1.8f*poseHeight[pose]/((scaleRect.y1-scaleRect.y0)*1024);
   float h=(b.y1-b.y0)*1024*metresPerPixel,w=(b.x1-b.x0)*1536*metresPerPixel;
   const float* anchor=animated?windAnchors[who][cel]:rigAnchors[who][pose];
   float ax=(anchor[0]-b.x0)/(b.x1-b.x0),ay=(anchor[1]-b.y0)/(b.y1-b.y0);
   V3 base=r.p+V3{0,.006f,0}+right*((.5f-ax)*w)-up*((1-ay)*h);
   float mat=5+who*.1f;V3 tint{1,1,1};
   V3 sunShadow=V3{-.70f,0,-.64f}*(1.f/.32f);
   auto ground=[&](V3 p){return onRoad(p)+V3{0,.018f,0};};
   auto silhouette=[&](float x,float t){return base+up*(h*(1-t))+right*((x-.5f)*w);};
   auto projected=[&](V3 source){
     V3 result=source;
     for(int k=0;k<3;k++){float height=std::max(0.f,source.y-ground(result).y);result=source+sunShadow*height;}
     return ground(result);
   };
   for(int row=0;row<16;row++)for(int col=0;col<8;col++){
     float t0=row/16.f,t1=(row+1)/16.f,x0=col/8.f,x1=(col+1)/8.f;
     artQuad(m,projected(silhouette(x0,t0)),projected(silhouette(x1,t0)),projected(silhouette(x1,t1)),projected(silhouette(x0,t1)),14+who*.1f,mix(u0,u1,x0),mix(v0,v1,t0),mix(u0,u1,x1),mix(v0,v1,t1));
   }

   V3 f{std::sin(r.heading),0,std::cos(r.heading)},q{f.z,0,-f.x};
   V3 contact=r.p;
   artQuad(m,ground(contact-f*.40f-q*.26f),ground(contact-f*.40f+q*.26f),ground(contact+f*.40f+q*.26f),ground(contact+f*.40f-q*.26f),7);
   artQuad(m,silhouette(0,0),silhouette(1,0),silhouette(1,1),silhouette(0,1),mat,u0,v0,u1,v1,tint);
 };
 drawPose(r.pose);
 return m;
}
void appendEffects(Mesh&m,const RideEffects&e,V3 camera,const Rider&r,const Road&road){
 RoadSurfaceQuery surfaceQuery(road,r.s);
 auto ground=[&](V3 p){if(!surfaceQuery.project(p))p.y=terrainField.height(p);return p+V3{0,.014f,0};};
 auto streak=[&](V3 a,V3 b,float width,V3 color){
   V3 right=norm(cross(b-a,camera-(a+b)*.5f))*width;
   quad(m,a-right,a+right,b+right,b-right,color,19);
 };
 // Exposure streaks use particle velocity relative to the moving camera.
 V3 relative=V3{0,-19,0}-r.velocity;
 for(const auto&drop:e.rain){
   streak(drop.p,drop.p-relative*.018f,.008f,{.47f,.57f,.64f});
 }
 for(const auto&s:e.splashes){
   float age=s.age,t=age/.24f,radius=.018f+t*.075f;
   V3 tint=mix(V3{.52f,.63f,.69f},V3{.26f,.32f,.36f},t);
   for(int j=0;j<6;j++){
     float angle=(j/6.f+s.seed)*pi*2;
     V3 dir{std::cos(angle),0,std::sin(angle)};
     float height=std::max(0.f,age*1.3f-age*age*6);
     V3 p=s.p+dir*(age*.30f)+V3{0,height,0};
     streak(p,p+V3{0,.018f*(1-t),0},.007f*(1-t),tint);
     V3 a=s.p+dir*radius,b=s.p+V3{std::cos(angle+.65f),0,std::sin(angle+.65f)}*radius;
     a=ground(a);b=ground(b);
     streak(a,b,.006f*(1-t),tint);
   }
 }

 for(const auto&t:e.tracks){V3 f=norm(t.b-t.a),q{f.z*.025f,0,-f.x*.025f};
   float fade=t.age/7;quad(m,t.a-q,t.b-q,t.b+q,t.a+q,mix(V3{.15f,.14f,.16f},V3{.29f,.27f,.29f},fade),9);
 }
 for(const auto&bit:e.debris){
   float angle=bit.seed*6.28f+bit.age*(bit.leaf?8:16),size=bit.leaf?.065f:.028f;
   V3 right{std::cos(angle)*size,0,std::sin(angle)*size};
   V3 up{0,size*(.35f+.65f*std::abs(std::sin(angle*1.7f))),0};
   V3 col=bit.leaf?V3{.40f+bit.seed*.12f,.30f,.12f}:V3{.40f,.38f,.36f};
   quad(m,bit.p-right,bit.p+up,bit.p+right,bit.p-up,col,24);
 }
 for(const auto&p:e.puffs){
   V3 toward=norm(camera-p.p),right=norm(cross({0,1,0},toward)),up=norm(cross(toward,right));
   float t=p.age/p.life,size=p.size*(1+t*2.5f);
   V3 tint=p.spray?V3{.57f,.63f,.69f}:V3{.78f,.77f,.74f};
   artQuad(m,p.p-right*size+up*size,p.p+right*size+up*size,p.p+right*size-up*size,p.p-right*size-up*size,11+t*.9f,0,0,1,1,tint);
 }
}
Mesh buildSky(int width,float wet,float cameraYaw){
 // This first painted plate is deliberately bounded; a complete 360-degree
 // environment needs additional authored views. Never mirror the sun/lake.
 (void)wet;Mesh m;float span=std::min(.98f,.90f*float(width)/960);float center=.5f+std::sin(cameraYaw)*(1-span)*.5f;float u0=center-span*.5f,u1=u0+span;
 artQuad(m,{-1,1,0},{1,1,0},{1,-1,0},{-1,-1,0},-2,u0,0,u1,1);return m;
}
Mesh buildHud(int width,const Rider&r,const Road&road,float wet,bool paused,bool demo,float fps,bool help){
 Mesh m;V3 white{.98f,.95f,.87f},muted{.72f,.77f,.81f},ink{.03f,.045f,.07f},gold{1,.68f,.34f};
 auto label=[&](const std::string&s,float x,float y,int font,V3 c){uiText(m,width,s,x+1,y+2,font,ink);uiText(m,width,s,x,y,font,c);};
 char buf[80];std::snprintf(buf,sizeof(buf),"%02d",int(r.speed()*3.6f));
 label(buf,26,449,3,white);label("KM/H",30,427,0,white);
 for(int i=0;i<9;i++){float x=28+i*15.f;rect(m,x,519,12,6,ink,width);rect(m,x+1,520,10,4,i<int(r.speed()/58*9)?gold:V3{.27f,.28f,.29f},width);}
 float rx=float(width-234);label("DISTANCE",rx,451,0,white);
 std::snprintf(buf,sizeof(buf),"%.1f KM",std::max(0.f,Road::finishDistance-r.s)/1000);label(buf,rx,472,1,white);
 label("ALTITUDE",rx,500,0,muted);std::snprintf(buf,sizeof(buf),"%d M",int(std::round(r.p.y)));label(buf,rx+88,500,0,white);
 float minX=1e6f,maxX=-1e6f,minZ=1e6f,maxZ=-1e6f;
 for(const auto& pt:road.points){minX=std::min(minX,pt.p.x);maxX=std::max(maxX,pt.p.x);minZ=std::min(minZ,pt.p.z);maxZ=std::max(maxZ,pt.p.z);}
 float mapScale=std::min(68/(maxX-minX),128/(maxZ-minZ));
 auto mapPoint=[&](V3 p){return V3{width-88+(p.x-(minX+maxX)*.5f)*mapScale,451-(p.z-(minZ+maxZ)*.5f)*mapScale,0};};
 auto mapStroke=[&](V3 a,V3 b,float thick,V3 color){
   V3 q=norm(V3{b.y-a.y,a.x-b.x,0})*thick;
   auto screen=[&](V3 p){return V3{p.x/width*2-1,1-p.y/270,0};};
   quad(m,screen(a-q),screen(a+q),screen(b+q),screen(b-q),color,-1);
 };
 for(int outline=1;outline>=0;outline--)for(size_t j=2;j<road.points.size();j+=2)
   mapStroke(mapPoint(road.points[j-2].p),mapPoint(road.points[j].p),outline?2.5f:1.f,outline?ink:muted);
 V3 marker=mapPoint(r.p);rect(m,std::round(marker.x)-3,std::round(marker.y)-3,7,7,ink,width);rect(m,std::round(marker.x)-2,std::round(marker.y)-2,5,5,{.98f,.34f,.23f},width);
 float progress=clamp(r.s/Road::finishDistance,0,1),barX=rx;
 rect(m,barX,522,130,3,ink,width);rect(m,barX,523,std::round(progress*130),1,gold,width);
 for(int gate=1;gate<=3;gate++)rect(m,barX+std::round(gate*32.5f),521,2,5,progress>=gate*.25f?gold:muted*.5f,width);
 float safe=cornerSpeed(road,r.s,wet,true,r.pilot);
 if(!paused&&!r.crashed&&safe<33&&r.speed()>safe*1.08f)uiCentered(m,width,"BRAKE BEFORE THE TURN",width*.5f,117,0,gold);
 if(road.course==7&&!paused&&!r.crashed){
   if(auto hazard=upcomingHazard(r.s,135)){
     const char* kind=hazard->kind==HazardKind::Basalt?"ROCKFALL":"VOLCANIC FISSURE";
     std::snprintf(buf,sizeof(buf),"%s  /  %.0f M",kind,std::max(0.f,hazard->s-r.s));
     uiCentered(m,width,buf,width*.5f,142,0,{1,.56f,.25f});
     uiCentered(m,width,hazard->d<0?"PASS ON THE RIGHT  >>":"<<  PASS ON THE LEFT",width*.5f,160,0,white);
   }
 }
 if(demo)label("DEMO / F2 TO PLAY",float(width-179),20,0,muted);
 if(wet>.65f)label("WET ROAD",29,403,0,muted);
 if(r.crashed){uiCentered(m,width,r.spinOut?"SPINOUT":"WIPEOUT",width*.5f,227,2,gold);uiCentered(m,width,"BACK ON TRACK",width*.5f,266,0,white);}
 if(r.drift>.3f){uiCentered(m,width,r.overRotation>.15f?"COUNTERSTEER!":r.slideAngle>1.35f?"BRAKE SLIDE":"SLIDE",width*.5f,478,1,r.overRotation>.15f?V3{1,.35f,.22f}:white);for(int i=0;i<12;i++)rect(m,float(width/2-47+i*8),506,5,3,i<int(r.slideAngle/2.1f*12)?(i>9?V3{1,.35f,.22f}:gold):ink,width);}
 else if(r.footBrake>.2f)uiCentered(m,width,"FOOT BRAKING",width*.5f,478,1,white);
 else if(r.boost>.1f)uiCentered(m,width,"CLEAN EXIT!",width*.5f,478,1,gold);
 if(help&&!paused){
   float x=width*.5f-225;uiPanel(m,width,x,129,450,86,ink,.94f);
   label("ARROWS : STEER / UP : TUCK / DOWN : FOOT BRAKE",x+12,142,0,white);
   label("SPACE : SLIDE / R : RESTART / N : SOUND / M : WEATHER",x+12,165,0,white);
   std::snprintf(buf,sizeof(buf),"%d FPS / %d X 540 / H TO HIDE",int(fps),width);label(buf,x+12,191,0,muted);
 }
 if(paused){
   float x=width*.5f-204;uiFrame(m,width,x,130,408,286,gold,.90f);
   uiImage(m,width,width*.5f-117,143,234,116,-7);
   uiCentered(m,width,r.finished?"DESCENT COMPLETE":"PAUSE",width*.5f,274,1,white);
   uiButton(m,width,r.finished?"R / RESTART":"ENTER / RESUME",x+48,310,312,gold);
   uiCentered(m,width,"ARROWS : STEER    SPACE : SLIDE",width*.5f,365,0,muted);
   uiCentered(m,width,"ESC : MENU    N : SOUND    H : HELP",width*.5f,390,0,muted);
 }
 return m;
}
}

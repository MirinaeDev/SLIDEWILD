// Neon Descent is an elevated, closed deck, not a re-skinned terrain ribbon.
// The exact same surface frames drive road rendering, wheels and collisions.
namespace {
struct FutureProp {V3 p;float height;int kind;V3 up{0,1,0};};
std::vector<FutureProp> futureProps;
const float futurePropBounds[8][4]={{66,10,257,499},{365,245,765,499},{858,269,1076,499},{1175,352,1502,499},
 {59,543,267,993},{362,800,739,995},{801,513,1102,994},{1170,734,1472,999}};
const float futurePropAnchors[]={115.95f,545.7f,962.65f,1323.95f,125.1f,550.f,947.27f,1297.24f};
struct FutureBox {
 V3 center,x,y,z,half;
 bool touchesSphere(V3 p,float radius)const{
   V3 q=p-center;float a=std::max(0.f,std::abs(dot(q,x))-half.x),b=std::max(0.f,std::abs(dot(q,y))-half.y),c=std::max(0.f,std::abs(dot(q,z))-half.z);
   return a*a+b*b+c*c<radius*radius;
 }
};
struct FutureExtent {V3 low,high;};
FutureExtent futureExtent(const FutureBox& b){
 V3 r{std::abs(b.x.x)*b.half.x+std::abs(b.y.x)*b.half.y+std::abs(b.z.x)*b.half.z,
      std::abs(b.x.y)*b.half.x+std::abs(b.y.y)*b.half.y+std::abs(b.z.y)*b.half.z,
      std::abs(b.x.z)*b.half.x+std::abs(b.y.z)*b.half.y+std::abs(b.z.z)*b.half.z};
 return {b.center-r,b.center+r};
}
bool futureOverlap(FutureExtent a,FutureExtent b,float gap=.25f){
 return a.low.x<b.high.x+gap&&a.high.x+gap>b.low.x&&a.low.y<b.high.y+gap&&a.high.y+gap>b.low.y&&a.low.z<b.high.z+gap&&a.high.z+gap>b.low.z;
}
FutureBox futureBuildingBox(V3 base,float w,float d,float h,float yaw){
 return {base+V3{0,h*.5f,0},{std::cos(yaw),0,-std::sin(yaw)},{0,1,0},{std::sin(yaw),0,std::cos(yaw)},{w*.5f+.3f,h*.5f+.3f,d*.5f+.3f}};
}
std::vector<FutureBox> futureObstacles;
std::vector<FutureExtent> futureGateLots,futureFixedLots,futureFurnitureLots;
// Sixteen alpha-derived height bands cover every possible billboard rotation.
// A lamp's narrow foot and wide overhang must not share one oversized radius.
const std::array<std::array<float,16>,8>& futurePropRadii(){
 static const auto radii=[](){
   std::array<std::array<float,16>,8> result{};
   auto path=resourcePath("assets/future-props-v26.rgba");
   if(!std::filesystem::exists(path))path="assets/future-props-v26.rgba";
   std::ifstream f(path,std::ios::binary);unsigned w=0,h=0;f.read(reinterpret_cast<char*>(&w),4);f.read(reinterpret_cast<char*>(&h),4);
   if(!f||w!=1536||h!=1024)throw std::runtime_error("Invalid future furniture atlas");
   std::vector<unsigned char> pixels(size_t(w)*h*4);f.read(reinterpret_cast<char*>(pixels.data()),pixels.size());
   if(!f)throw std::runtime_error("Incomplete future furniture atlas");
   for(int kind=0;kind<8;kind++){
     auto& b=futurePropBounds[kind];float height=b[3]-b[1];
     for(int y=int(b[1]);y<int(b[3]);y++)for(int x=int(b[0]);x<int(b[2]);x++)if(pixels[(y*w+x)*4+3]>180){
       int band=std::min(15,int((b[3]-1-y)*16/height));
       result[kind][band]=std::max(result[kind][band],(std::abs(x+.5f-futurePropAnchors[kind])+1)/height);
     }
   }return result;
 }();return radii;
}
FutureExtent futurePropExtent(const FutureProp& p){
 const auto& b=futurePropBounds[p.kind];float radius=std::max(futurePropAnchors[p.kind]-b[0],b[2]-futurePropAnchors[p.kind])*p.height/(b[3]-b[1]);
 V3 top=p.p+p.up*p.height;
 // The exact height-band checks below refine this inexpensive broad phase.
 V3 extent{radius,radius,radius};
 return {{std::min(top.x,p.p.x)-extent.x,std::min(top.y,p.p.y)-extent.y,std::min(top.z,p.p.z)-extent.z},
         {std::max(top.x,p.p.x)+extent.x,std::max(top.y,p.p.y)+extent.y,std::max(top.z,p.p.z)+extent.z}};
}
bool futurePropClear(const FutureProp& prop){
 auto extent=futurePropExtent(prop);
 for(const auto& occupied:futureFurnitureLots)if(futureOverlap(extent,occupied,.35f))return false;
 for(const auto& obstacle:futureObstacles){
   if(!futureOverlap(extent,futureExtent(obstacle)))continue;
   for(int band=0;band<16;band++){
     float r=futurePropRadii()[prop.kind][band]*prop.height;if(r==0)continue;
     V3 center=prop.p+prop.up*(prop.height*(band+.5f)/16);
     // Enclose the complete band, including its vertical extent and texel edges.
     float radius=std::sqrt(r*r+std::pow(prop.height/32,2))+.06f;
     if(obstacle.touchesSphere(center,radius))return false;
   }
 }
 return true;
}
struct FutureArchEdge {float x0,y0,x1,y1;V3 color;};
const float futureArchBounds[4][4]={{20,28,811,442},{844,19,1643,442},{16,488,813,913},{844,483,1644,913}};
const std::array<std::vector<FutureArchEdge>,4>& futureArchContours(){
 static const auto contours=[](){
   std::array<std::vector<FutureArchEdge>,4> result;
   std::filesystem::path path=resourcePath("assets/future-arches-v25.rgba");
   if(!std::filesystem::exists(path))path="assets/future-arches-v25.rgba";
   std::ifstream f(path,std::ios::binary);unsigned w=0,h=0;
   f.read(reinterpret_cast<char*>(&w),4);f.read(reinterpret_cast<char*>(&h),4);
   if(!f||w!=1659||h!=948)throw std::runtime_error("Invalid painted gateway atlas");
   std::vector<unsigned char> pixels(size_t(w)*h*4);f.read(reinterpret_cast<char*>(pixels.data()),pixels.size());
   if(!f)throw std::runtime_error("Incomplete painted gateway atlas");
   for(int kind=0;kind<4;kind++){
     const auto& b=futureArchBounds[kind];int nx=int(std::ceil((b[2]-b[0])/3)),ny=int(std::ceil((b[3]-b[1])/3));
     auto opaque=[&](int x,int y){
       if(x<0||y<0||x>=nx||y>=ny)return false;
       int px=int(b[0]+(x+.5f)*(b[2]-b[0])/nx),py=int(b[1]+(y+.5f)*(b[3]-b[1])/ny);
       return pixels[(py*w+px)*4+3]>170;
     };
     for(int y=0;y<ny;y++)for(int x=0;x<nx;x++)if(opaque(x,y)){
       int px=int(b[0]+(x+.5f)*(b[2]-b[0])/nx),py=int(b[1]+(y+.5f)*(b[3]-b[1])/ny);size_t i=(py*w+px)*4;
       V3 color{pixels[i]/255.f,pixels[i+1]/255.f,pixels[i+2]/255.f};
       float x0=float(x)/nx,x1=float(x+1)/nx,y0=float(y)/ny,y1=float(y+1)/ny;
       auto edge=[&](float a,float c,float d,float e){result[kind].push_back({a,c,d,e,color});};
       if(!opaque(x-1,y))edge(x0,y0,x0,y1);
       if(!opaque(x+1,y))edge(x1,y1,x1,y0);
       if(!opaque(x,y-1))edge(x1,y0,x0,y0);
       if(!opaque(x,y+1))edge(x0,y1,x1,y1);
     }
   }
   return result;
 }();return contours;
}
void futureBeam(Mesh& m,V3 a,V3 b,float radius,V3 color,float material=48){
 V3 axis=norm(b-a),u=norm(cross(axis,std::abs(axis.y)>.9f?V3{1,0,0}:V3{0,1,0}))*radius,v=norm(cross(axis,u))*radius;
 quad(m,a-u-v,b-u-v,b+u-v,a+u-v,color,material);
 quad(m,a+u-v,b+u-v,b+u+v,a+u+v,color,material);
 quad(m,a+u+v,b+u+v,b-u+v,a-u+v,color,material);
 quad(m,a-u+v,b-u+v,b-u-v,a-u-v,color,material);
 quad(m,a-u-v,a+u-v,a+u+v,a-u+v,color,material);
 quad(m,b-u+v,b+u+v,b+u-v,b-u-v,color,material);
}
void futureFoundation(Mesh& m,V3 base,float w,float d,float bottom,float yaw,int family){
 if(bottom>=base.y)return;
 V3 x{std::cos(yaw)*w*.5f,0,-std::sin(yaw)*w*.5f},z{std::sin(yaw)*d*.5f,0,std::cos(yaw)*d*.5f};
 V3 c[]={base-x-z,base+x-z,base+x+z,base-x+z};
 const V3 tints[]={{.76f,.82f,.81f},{.67f,.74f,.77f},{.80f,.80f,.70f},{.63f,.73f,.75f}};
 float height=base.y-bottom;
 // Solid civil-engineering podiums, not hundreds of repeated window storeys.
 // The low-frequency concrete uses metres and never stretches a facade image.
 for(int side=0;side<4;side++)
   artQuad(m,c[side],c[(side+1)%4],c[(side+1)%4]-V3{0,height,0},c[side]-V3{0,height,0},60+family,0,0,side%2?d:w,height,tints[family]);
 quad(m,c[0]-V3{0,height,0},c[3]-V3{0,height,0},c[2]-V3{0,height,0},c[1]-V3{0,height,0},{.10f,.14f,.21f},48);
}
void futureBuilding(Mesh& m,V3 base,float w,float d,float h,float yaw,int variant,bool upper=false,bool rooftop=true){
 (void)upper;(void)rooftop;variant%=4;
 V3 x{std::cos(yaw),0,-std::sin(yaw)},z{std::sin(yaw),0,std::cos(yaw)};
 auto p=[&](float a,float b,float c){return base+x*a+V3{0,b,0}+z*c;};
 const V3 concrete{.84f,.88f,.84f},petrol{.21f,.39f,.42f},cream{.89f,.87f,.71f};
 auto block=[&](float a,float b,float c,float sx,float sy,float sz,V3 tint,float mat=71){box(m,p(a,b,c),{sx,sy,sz},tint,yaw,mat);};
 // Atlas cells are complete architectural elevations. A bay is 18 m wide,
 // including its ground-level door, and is never repeated up a tower.
 auto elevation=[&](float a,float b,float c,float width,float height,int cell){
   (void)height;
   const float bounds[4][4]={{4,100,764,474},{772,4,1532,474},{4,642,764,1004},{772,540,1532,1004}};
   const auto& bnd=bounds[cell];height=width*(bnd[3]-bnd[1])/(bnd[2]-bnd[0]);
   artQuad(m,p(a-width*.5f,b+height,c),p(a+width*.5f,b+height,c),p(a+width*.5f,b,c),p(a-width*.5f,b,c),70,bnd[0]/1536,bnd[1]/1024,bnd[2]/1536,bnd[3]/1024);
 };
 auto serviceFace=[&](int side,float bottom,float inset,float width,int cell){
   const float bounds[4][4]={{4,100,764,474},{772,4,1532,474},{4,642,764,1004},{772,540,1532,1004}};
   const auto& bnd=bounds[cell];float height=width*(bnd[3]-bnd[1])/(bnd[2]-bnd[0]);
   V3 along=side%2?z:x,normal=side%2?x:z;float sign=side<2?-1.f:1.f;
   V3 center=base+normal*(sign*((side%2?w:d)*.5f-inset+.035f))+V3{0,bottom,0};
   artQuad(m,center-along*(width*.5f)+V3{0,height,0},center+along*(width*.5f)+V3{0,height,0},center+along*(width*.5f),center-along*(width*.5f),70,bnd[0]/1536,bnd[1]/1024,bnd[2]/1536,bnd[3]/1024);
 };
 if(variant==0){ // Pit halls: broad gallery, stepped roof and workshop bays.
   block(0,h*.5f,0,w,h,d,concrete);
   int bays=std::max(1,int(w/18));float bay=std::min(18.f,w/bays-.8f),eh=bay/1.5f;
   for(int j=0;j<bays;j++)elevation((j+.5f)*w/bays-w*.5f,.25f,-d*.5f-.025f,bay,eh,0);
   float gallery=std::min(5.f,h*.23f);
   block(0,h-gallery*.5f-.5f,-d*.5f+.7f,w-1.4f,gallery,1.5f,petrol,72);
   block(0,h+.25f,-d*.22f,w,.5f,d*.56f,cream);
   for(int j=0;j<=bays;j++)block(-w*.5f+.4f+j*(w-.8f)/bays,h*.5f,-d*.5f+.4f,.8f,h,.8f,concrete);
   for(int j=0;j<3;j++)block((j-1)*w*.25f,h+1.3f,d*.18f,w*.15f,2.6f,d*.32f,petrol,68);
   for(int side:{1,3})serviceFace(side,.3f,0,std::min(d-1.6f,18.f),3);
   serviceFace(2,.3f,0,std::min(w-1.6f,24.f),0);
   // Tall terminal halls get a second glazed gallery, rather than a blank wall.
   if(h>30)block(0,h*.55f,-d*.5f+.8f,w-1.4f,5.f,1.7f,petrol,72);
 }else if(variant==1){ // Structural pylons: smooth shafts, recessed channels.
   block(0,h*.45f,0,w*.82f,h*.9f,d*.82f,concrete);
   block(0,h*.92f,0,w*.65f,h*.16f,d*.65f,cream);
   for(int side:{-1,1}){
     block(side*w*.40f,h*.48f,0,w*.13f,h*.94f,d*.94f,petrol);
     block(side*w*.21f,h*.45f,-d*.414f,w*.10f,h*.74f,.18f,{.18f,.48f,.44f},72);
   }
   float panel=std::min(w*.50f,15.f);
   elevation(0,h*.59f,-d*.411f-.03f,panel,panel/1.5f,1);
   // One broad identifier per tower, not a repeated pattern on every storey.
   artQuad(m,p(-w*.17f,h*.50f,-d*.418f),p(w*.17f,h*.50f,-d*.418f),p(w*.17f,h*.37f,-d*.418f),p(-w*.17f,h*.37f,-d*.418f),73);
   block(0,h+1.2f,0,w*.50f,2.4f,d*.50f,petrol,68);
   futureBeam(m,p(0,h+2.4f,0),p(0,h+6,0),.13f,concrete,71);
   block(0,h+6.15f,0,.7f,.3f,.7f,{1,.59f,.20f},49);
   for(int side:{1,3}){
     serviceFace(side,h*.24f,w*.035f,std::min(d*.72f,18.f),1);
     block((side==1?-1.f:1.f)*w*.467f,h*.70f,0,.12f,h*.38f,d*.23f,{.16f,.46f,.45f},72);
   }
   serviceFace(2,h*.58f,d*.09f,std::min(w*.5f,15.f),1);
 }else if(variant==2){ // Real stepped grandstand, facing the racing ribbon.
   const int tiers=8;float rise=h/tiers,run=d/tiers;
   for(int j=0;j<tiers;j++){
     float front=-d*.5f+j*run,height=(j+1)*rise;
     block(0,height*.5f,front+run*.5f,w,height,run,concrete);
     // Eight contiguous slices of one painted stand, not eight full facades.
     float v0=(644+(7-j)*32.f)/1024,v1=v0+32.f/1024;
     artQuad(m,p(-w*.5f+.35f,height,front-.025f),p(w*.5f-.35f,height,front-.025f),p(w*.5f-.35f,height-rise,front-.025f),p(-w*.5f+.35f,height-rise,front-.025f),74,4.f/1536,v0,764.f/1536,v1);
     block(0,height+.13f,front+.15f,w-.7f,.26f,.3f,cream);
   }
   for(int side:{-1,1})block(side*(w*.5f-.35f),h*.5f,d*.44f,.7f,h,d*.10f,petrol);
   block(0,h+1.8f,d*.34f,w*.45f,3.6f,d*.20f,petrol,72);
   block(0,h+3.8f,d*.27f,w,.4f,d*.45f,cream);
   serviceFace(2,.3f,0,std::min(w*.60f,20.f),3);
 }else{ // Energy/control buildings: fins and a few large instrument bays.
   block(0,h*.20f,0,w,h*.4f,d,concrete);
   block(0,h*.67f,d*.05f,w*.70f,h*.66f,d*.72f,petrol);
   for(int j=-1;j<=1;j++){
     float a=j*w*.28f;
     block(a,h*.67f,-d*.28f,w*.09f,h*.66f,d*.14f,cream);
     block(a,h*.70f,-d*.358f,w*.055f,h*.43f,.12f,{.14f,.44f,.46f},72);
   }
   float bw=std::min(24.f,w*.78f);
   elevation(0,.3f,-d*.5f-.025f,bw,bw/1.5f,3);
   block(0,h+1.3f,d*.05f,w*.48f,2.6f,d*.48f,concrete);
   for(int side:{1,3}){
     serviceFace(side,.3f,0,std::min(d*.65f,18.f),1);
     serviceFace(side,h*.45f,w*.15f,std::min(d*.60f,18.f),3);
   }
   serviceFace(2,.3f,0,std::min(w*.70f,24.f),3);
   serviceFace(2,h*.45f,d*.09f,std::min(w*.62f,24.f),3);
 }
}
void futureArch(Mesh& m,const Road& road,float s,int kind){
 // World-anchored sprite planes follow the bank as a rigid frame. Uniform
 // scaling preserves the painted arch; the empty opening remains transparent.
 const auto& b=futureArchBounds[kind%4];auto r=road.at(s);
 V3 across=norm(V3{std::cos(r.heading),r.bank,-std::sin(r.heading)});
 V3 tangent=norm(road.surface(s+.1f,0)-road.surface(s-.1f,0));
 // Follow the grade as well as the bank: the old horizontal depth axis left
 // one face's feet hanging over the plinth on the steep downhill sections.
 V3 forward=norm(tangent-across*dot(tangent,across));
 V3 up=norm(cross(forward,across));
 // Measured opaque foot widths differ between drawings. Every opening leaves
 // at least 12.5 horizontal metres clear, including alpha edge texels and the
 // projection changes across the arch's depth. Eleven metres touched the rail.
 const float spans[]={22.5f,22.5f,24.2f,32.1f};float span=spans[kind%4];
 float width=span*std::sqrt(1+r.bank*r.bank),height=width*(b[3]-b[1])/(b[2]-b[0]);
 FutureBox volume{r.p+up*(height*.5f-.07f),across,up,forward,{width*.5f,height*.5f,1.55f}};
 futureObstacles.push_back(volume);futureGateLots.push_back(futureExtent(volume));
 for(float depth:{-.68f,.68f}){
   V3 center=r.p+forward*depth-up*.07f,left=center-across*(width*.5f),right=center+across*(width*.5f);
   artQuad(m,left+up*height,right+up*height,right,left,58,b[0]/1659,b[1]/948,b[2]/1659,b[3]/948);
 }
 auto vertex=[&](float x,float y,float depth){return r.p+across*((x-.5f)*width)+up*((1-y)*height-.07f)+forward*depth;};
 for(const auto& e:futureArchContours()[kind%4])
   quad(m,vertex(e.x0,e.y0,-.68f),vertex(e.x1,e.y1,-.68f),vertex(e.x1,e.y1,.68f),vertex(e.x0,e.y0,.68f),e.color,64);
 // Closed cantilever plinths carry the opaque feet and their side walls.
 // The terminal already has a full-width apron. A second almost-coplanar
 // plinth there would intersect it and produce flashing at the attachment.
 if(s<4002)for(int side:{-1,1}){
   float edge=side*(span*.5f+.3f);
   for(int j=0;j<6;j++){
     float at=s-1.35f+j*.45f;
     V3 a=road.surface(at,side*7.f)-V3{0,.08f,0},b=road.surface(at+.45f,side*7.f)-V3{0,.08f,0};
     V3 c=road.surface(at+.45f,edge)-V3{0,.08f,0},d=road.surface(at,edge)-V3{0,.08f,0},down{0,.95f,0};
     quad(m,a,b,c,d,{.72f,.78f,.83f},66);quad(m,d-down,c-down,b-down,a-down,{.45f,.53f,.63f},68);
     quad(m,d,c,c-down,d-down,{.75f,.81f,.86f},68);
     if(j==0)quad(m,a,d,d-down,a-down,{.75f,.81f,.86f},68);
     if(j==5)quad(m,c,b,b-down,c-down,{.75f,.81f,.86f},68);
   }
   futureBeam(m,road.surface(s,side*5.8f)-V3{0,3,0},road.surface(s,side*(span*.5f-.8f))-V3{0,1.05f,0},.24f,{.28f,.37f,.45f},68);
 }
}
StaticWorld buildFutureWorld(const Road& road){
 StaticWorld result;Mesh& m=result.vertices;m.reserve(240000);terrainField={};futureProps.clear();
 futureObstacles.clear();futureGateLots.clear();futureFixedLots.clear();futureFurnitureLots.clear();
 const V3 metal{.36f,.43f,.56f},dark{.12f,.16f,.25f},cyan{.25f,.80f,.86f};
 auto point=[&](float s,float d,float h=0){return road.surface(s,d)+V3{0,h,0};};
 auto place=[&](float s,float d,float height,int kind){
   for(float shift:{0.f,3.f,-3.f,6.f,-6.f,10.f,-10.f,14.f,-14.f}){
     float at=s+shift;if(at<3||at>road.size()-4)continue;
     V3 across=norm(point(at,d+.1f)-point(at,d-.1f)),forward=norm(point(at+.1f,d)-point(at-.1f,d));
     V3 up=norm(cross(forward,across));FutureProp prop{point(at,d)+up*.015f,height,kind,up};
     if(!futurePropClear(prop))continue;
     futureFurnitureLots.push_back(futurePropExtent(prop));futureProps.push_back(prop);return;
   }
 };
 result.segmentOffsets.push_back(0);
 for(float s=0;s<road.size();s+=2){
   V3 a=point(s,-4.7f),b=point(s+2,-4.7f),c=point(s+2,4.7f),d=point(s,4.7f);
   size_t first=m.size();quad(m,a,b,c,d,{1,1,1},1);
   const float uv[6][2]={{-4.7f,s},{-4.7f,s+2},{4.7f,s+2},{-4.7f,s},{4.7f,s+2},{4.7f,s}};
   for(int k=0;k<6;k++){m[first+k].u=uv[k][0];m[first+k].v=uv[k][1];}
   // Solid slab, closed sides and underside; no daylight under the road edge.
   quad(m,point(s,-7,-1.3f),point(s,7,-1.3f),point(s+2,7,-1.3f),point(s+2,-7,-1.3f),dark,68);
   for(int side:{-1,1}){
     float edge=side*4.7f,out=side*7.f;
     quad(m,point(s,edge),point(s+2,edge),point(s+2,out),point(s,out),metal,66);
     artQuad(m,point(s,out),point(s+2,out),point(s+2,out,-1.3f),point(s,out,-1.3f),65,s/4.8f,2.002f,(s+2)/4.8f,2.998f);
     // Chamfered ceramic caps, painted impact pads, reflectors and actual backs.
     // Longitudinal UVs are continuous through mesh joins and bank changes.
     float inner=side*4.85f,lip=side*4.98f,outer=side*5.55f,cap=side*5.42f;
     float row=int(s/160)%3==1?1.f:0.f;
     artQuad(m,point(s,inner,.60f),point(s+2,inner,.60f),point(s+2,inner),point(s,inner),65,s/4.8f,row+.002f,(s+2)/4.8f,row+.998f);
     quad(m,point(s,inner,.60f),point(s+2,inner,.60f),point(s+2,lip,.78f),point(s,lip,.78f),metal,67);
     quad(m,point(s,lip,.78f),point(s+2,lip,.78f),point(s+2,cap,.78f),point(s,cap,.78f),metal,67);
     quad(m,point(s,cap,.78f),point(s+2,cap,.78f),point(s+2,outer,.60f),point(s,outer,.60f),metal,67);
     quad(m,point(s,outer),point(s,outer,.60f),point(s+2,outer,.60f),point(s+2,outer),dark,68);
     auto frame=road.at(s+1);float grade=std::abs(point(s+2,side*5.2f).y-point(s,side*5.2f).y)*.5f;
     futureObstacles.push_back({point(s+1,side*5.2f,.39f),{std::cos(frame.heading),0,-std::sin(frame.heading)},{0,1,0},{std::sin(frame.heading),0,std::cos(frame.heading)},
       {.38f,.39f+grade+std::abs(frame.bank)*.38f,1.06f}});
   }
 }
 for(float s:{0.f,road.size()})quad(m,point(s,-7),point(s,7),point(s,7,-1.3f),point(s,-7,-1.3f),metal,48);
 // Repeated structural ribs are intentional engineering, rather than scenery tiles.
 for(float s=22;s<road.size();s+=38){
   auto r=road.at(s);futureBeam(m,point(s,-6,-1.5f),point(s,6,-1.5f),.28f,metal);
   box(m,r.p+V3{0,-32,0},{1.7f,61,2.1f},dark,r.heading,68);
   futureBeam(m,r.p+V3{0,-10,0},point(s,6,-1.4f),.24f,metal);
   futureBeam(m,r.p+V3{0,-10,0},point(s,-6,-1.4f),.24f,metal);
 }
 // Drawn gateways mark distinct sectors instead of repeating untextured ribs.
 int gate=0;
 for(float s:{110.f,450.f,900.f,1210.f,1670.f,1870.f,2240.f,2630.f,2960.f,3670.f,4000.f}){
   auto r=road.at(s);futureArch(m,road,s,s==4000?3:gate++%3);
   if(s==4000){
     V3 side{std::cos(r.heading),0,-std::sin(r.heading)},c=r.p+V3{0,7.1f,0}-V3{std::sin(r.heading),0,std::cos(r.heading)}*.32f;
     artQuad(m,c-side*2.8f+V3{0,1.1f,0},c+side*2.8f+V3{0,1.1f,0},c+side*2.8f-V3{0,.1f,0},c-side*2.8f-V3{0,.1f,0},26,0,2.f/3,1,1);
   }
 }
 for(float begin:{1420.f,3350.f})for(int ring=0;ring<3;ring++)futureArch(m,road,begin+ring*26,begin<2000?0:1);
 for(float s:{4018.f,4050.f,4082.f,4114.f})futureArch(m,road,s,3);
 // Reserve the terminal before procedural city lots, including their cornices.
 auto end=road.at(road.size());V3 forward{std::sin(end.heading),0,std::cos(end.heading)},right{forward.z,0,-forward.x};
 for(int side:{-1,1})for(int j=0;j<3;j++){
   auto volume=futureBuildingBox(road.at(4028+j*44.f).p+right*(side*34.f),30,24,16+j*4,end.heading);
   futureFixedLots.push_back(futureExtent(volume));futureObstacles.push_back(volume);
 }
 auto terminal=futureBuildingBox(end.p+forward*39+V3{0,-3,0},108,36,44,end.heading);
 futureFixedLots.push_back(futureExtent(terminal));futureObstacles.push_back(terminal);
 for(int row=0;row<3;row++)for(int tile=0;tile<16;tile++){
   float x=-4.7f+tile*.5875f,z=4000+row*.59f;
   quad(m,point(z,x,.025f),point(z+.59f,x,.025f),point(z+.59f,x+.5875f,.025f),point(z,x+.5875f,.025f),
     (row+tile)%2?V3{.08f,.09f,.14f}:V3{.9f,.95f,1},49);
 }
 // A racing acropolis: low hangars and stepped stands by the course, utility
 // pylons farther out. These have different silhouettes, not window variants.
 // Lots clear both the entire course and their neighbours; tall buildings do
 // not interpenetrate when a row follows the inside of a curved road.
 struct Lot {V3 p;float radius;};std::vector<Lot> lots;
 for(int j=0;j<46;j++)for(int side:{-1,1})for(int row=0;row<3;row++){
   float s=30+j*86.f+row*23,seed=float(j*23+row*7+(side+1)*47);
   const int families[3][6]={{0,2,0,3,2,0},{3,0,1,0,3,2},{1,3,0,1,3,0}};
   int family=families[row][(j+(side>0?2:0))%6];
   float w=36+12*(j%3),d=24+6*(j%2),h=16+hash(seed+4)*12;
   if(family==1){w=18+6*(j%2);d=w;h=68+hash(seed+4)*46;}
   if(family==2){w=48+12*(j%2);d=28;h=14+4*(j%2);}
   if(family==3){w=30+6*(j%2);d=28;h=32+hash(seed+4)*22;}
   float offset=side*(28+row*64+hash(seed+2)*8);
   auto r=road.at(s);V3 base{};
   // Fronts face the track, rather than showing the side of a generic tower.
   float yaw=r.heading+side*1.5707963268f;
   float radius=std::sqrt(w*w+d*d)*.5f;
   bool clear=false;FutureBox volume{};
   for(float shift:{0.f,12.f,24.f,36.f}){
     base=point(s,offset+side*shift);base.y=r.p.y-4-row*14;
     if(road.locate(base).distance<radius+10)continue;
     bool vacant=true;for(const auto& lot:lots){V3 delta=base-lot.p;delta.y=0;if(length(delta)<radius+lot.radius+3){vacant=false;break;}}
     if(!vacant)continue;
     float floor=road.at(road.size()).p.y-90;
     volume=futureBuildingBox({base.x,floor,base.z},w,d,base.y+h+6.5f-floor,yaw);
     auto bounds=futureExtent(volume);
     for(const auto& fixed:futureFixedLots)if(futureOverlap(bounds,fixed,1.5f)){vacant=false;break;}
     for(const auto& gate:futureGateLots)if(futureOverlap(bounds,gate,1.5f)){vacant=false;break;}
     if(vacant){clear=true;break;}
   }
   if(!clear)continue;
   lots.push_back({base,radius});futureObstacles.push_back(volume);
   futureFoundation(m,base,w,d,road.at(road.size()).p.y-90,yaw,family);
   futureBuilding(m,base,w,d,h,yaw,family);
 }
 // Fixed signs are placed first, so later furniture also clears their mast and
 // face. A central bolted mast replaces the two feet that straddled the deck.
 for(int j=0;j<116;j++){
   float s=18+j*34.f;int side=j%2?1:-1;
   float bend=wrap(road.at(s+55).heading-road.at(s).heading);
   if(std::abs(bend)<=.65f)continue;
   for(float shift:{0.f,-4.f,4.f,-8.f,8.f}){
     float at=s+8+shift;auto frame=road.at(at);V3 across=norm(V3{std::cos(frame.heading),frame.bank,-std::sin(frame.heading)});
     V3 ahead{std::sin(frame.heading),0,std::cos(frame.heading)},up=norm(cross(ahead,across)),root=point(at,side*6.3f);
     V3 c=root+up*2.35f;FutureBox head{c,across,up,ahead,{1.22f,2.44f/12,.10f}};
     bool clear=true;
     for(const auto& gate:futureGateLots)if(futureOverlap(futureExtent(head),gate,1)){clear=false;break;}
     if(!clear)continue;
     futureBeam(m,root,c,.065f,metal,68);
     futureBeam(m,root-up*.02f,root+up*.12f,.15f,metal,68);
     futureBeam(m,c-across*1.12f,c+across*1.12f,.065f,metal,68);
     V3 a=c-across*1.22f+up*(2.44f/12)-ahead*.11f,b=c+across*1.22f+up*(2.44f/12)-ahead*.11f;
     V3 e=b-up*(2.44f/6),d=a-up*(2.44f/6),back=ahead*.22f;
     float u0=bend>0?0.f:1.f,u1=1-u0;
     artQuad(m,a,b,e,d,69,u0,0,u1,1);
     quad(m,b+back,a+back,d+back,e+back,metal,68);
     quad(m,a,a+back,b+back,b,metal,68);quad(m,d,e,e+back,d+back,metal,68);
     quad(m,a,d,d+back,a+back,metal,68);quad(m,b,b+back,e+back,e,metal,68);
     futureObstacles.push_back(head);futureObstacles.push_back({root+up*1.175f,across,up,ahead,{.16f,1.2f,.16f}});
     break;
   }
 }
 // Keep the variety and count by trying nearby vacant slots before omitting
 // an item. Clearance uses its complete alpha silhouette, not just its foot.
 for(int j=0;j<116;j++){
   float s=18+j*34.f;int side=j%2?1:-1;
   place(s,side*6.3f,3.6f,0);
   if(j%3!=0)place(s+12,-side*6.3f,.70f,1);
   if(j%9==0){
     place(s+18,side*6.3f,.65f,5);
     place(s+22,side*6.3f,1.15f,2);
   }
 }
 // Terminal apron: solid architectural background replaces an exposed world edge.
 for(int side:{-1,1}){
   for(int j=0;j<3;j++){
     float s=4028+j*44.f;
     futureBuilding(m,road.at(s).p+right*(side*34.f),24,30,16+j*4,end.heading+side*1.5707963268f,0);
     place(s,side*15.f,2.1f,4);
     place(s+4,side*14.f,.95f,3);
     place(s-4,side*15.f,2.8f,6);
     place(s+9,side*13.f,1.f,1);
     if(j==0)place(s+15,side*11.f,.9f,7);
   }
   for(int j=0;j<12;j++)place(4008+j*12,side*6.3f,j%3==0?3.6f:.7f,j%3==0?0:1);
 }
 for(float s=4002;s<road.size();s+=2)for(int side:{-1,1}){
   quad(m,point(s,side*7,-.03f),point(s+2,side*7,-.03f),point(s+2,side*19,-.03f),point(s,side*19,-.03f),metal,66);
   quad(m,point(s,side*19,-.03f),point(s+2,side*19,-.03f),point(s+2,side*19,-1.3f),point(s,side*19,-1.3f),dark,68);
 }
 futureBuilding(m,end.p+forward*39+V3{0,-3,0},108,36,44,end.heading,0);
 // The finish hall uses the same ground-level pit architecture as the venue.
 V3 entrance=end.p+forward*20.94f;
 artQuad(m,entrance-right*24+V3{0,23.62f,0},entrance+right*24+V3{0,23.62f,0},entrance+right*24,entrance-right*24,70,4.f/1536,100.f/1024,764.f/1536,474.f/1024);
 for(int x=-5;x<=5;x++){
   V3 p=end.p+right*(x*8.f)+forward*14;
   futureBeam(m,p,p+V3{0,7,0},.18f,metal);
   box(m,p+V3{0,7,0},{1.4f,.4f,1.4f},cyan,end.heading,49);
 }
 result.segmentOffsets.push_back(Uint32(m.size()));indexRocks(m);indexVerges(m,0);return result;
}
void appendFutureProps(Mesh& m,V3 camera){
 for(const auto& prop:futureProps){
   float distance=length(prop.p-camera),fade=clamp((680-distance)/180,0,1);if(fade<=0)continue;
   const auto& b=futurePropBounds[prop.kind];
   V3 right=norm(cross(prop.up,norm(camera-prop.p)));
   // Anchor the opaque foot, rather than the rectangle's centre. This matters
   // for the overhanging lamps and directional signs, whose pole is off-centre.
   float scale=prop.height/(b[3]-b[1]);
   V3 left=prop.p+right*((b[0]-futurePropAnchors[prop.kind])*scale),end=prop.p+right*((b[2]-futurePropAnchors[prop.kind])*scale),top=prop.up*prop.height;
   if(distance<140){
     V3 along=norm(cross(right,prop.up)),c=prop.p+prop.up*.009f;
     float radius=std::min(.65f,(b[2]-b[0])*scale*.43f),depth=std::min(.35f,radius*.55f);
     artQuad(m,c-right*radius-along*depth,c+right*radius-along*depth,c+right*radius+along*depth,c-right*radius+along*depth,7);
   }
   artQuad(m,left+top,end+top,end,left,29+fade*.4f,b[0]/1536,b[1]/1024,b[2]/1536,b[3]/1024);
 }
}
}

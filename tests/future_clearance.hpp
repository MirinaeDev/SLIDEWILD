#pragma once
#include <fstream>
#include <filesystem>
#include <map>
#include <array>

// Check the emitted render geometry, independently of the placement volumes.
// Actual alpha texels distinguish an arch opening from its opaque pillars.
struct ClearanceAlpha {
 unsigned w=0,h=0;std::vector<unsigned char> bytes;
 explicit ClearanceAlpha(const char* name){
   auto path=std::filesystem::path(SDL_GetBasePath())/"assets"/name;
   if(!std::filesystem::exists(path))path=std::filesystem::current_path()/"assets"/name;
   std::ifstream f(path,std::ios::binary);f.read(reinterpret_cast<char*>(&w),4);f.read(reinterpret_cast<char*>(&h),4);
   if(!f||!w||!h)throw std::runtime_error("Missing clearance test atlas");
   bytes.resize(size_t(w)*h*4);f.read(reinterpret_cast<char*>(bytes.data()),bytes.size());
   if(!f)throw std::runtime_error("Incomplete clearance test atlas");
 }
 bool opaque(float u,float v,int threshold)const{
   int x=std::clamp(int(u*w),0,int(w)-1),y=std::clamp(int(v*h),0,int(h)-1);
   return bytes[(y*w+x)*4+3]>threshold;
 }
};
inline void checkFutureClearance(const dh::StaticWorld& world,const dh::Road& road){
 using namespace dh;
 ClearanceAlpha furniture("future-props-v26.rgba"),arches("future-arches-v25.rgba");
 struct Face {size_t index;V3 low,high;};std::vector<Face> faces;
 std::map<std::pair<int,int>,std::vector<size_t>> cells;
 auto cell=[](float p){return int(std::floor(p/12));};
 for(size_t k=0;k<world.vertices.size();k+=3){
   float mat=world.vertices[k].material;
   if(mat!=30&&mat!=58&&!(mat>=60&&mat<=65)&&!(mat>=67&&mat<=74))continue;
   auto a=world.vertices[k].p,b=world.vertices[k+1].p,c=world.vertices[k+2].p;
   Face f{k,{std::min({a.x,b.x,c.x}),std::min({a.y,b.y,c.y}),std::min({a.z,b.z,c.z})},
            {std::max({a.x,b.x,c.x}),std::max({a.y,b.y,c.y}),std::max({a.z,b.z,c.z})}};
   size_t index=faces.size();faces.push_back(f);
   for(int x=cell(f.low.x);x<=cell(f.high.x);x++)for(int z=cell(f.low.z);z<=cell(f.high.z);z++)cells[{x,z}].push_back(index);
 }
 // The opaque parts of the arches must also clear rails, signs and facades.
 // Do not test the arch's own extrusion against its front/back attachment.
 size_t archSections=0;
 for(size_t k=0;k<world.vertices.size();k+=6){
   const auto& a=world.vertices[k];if(a.material!=58)continue;
   const auto& b=world.vertices[k+1];const auto& d=world.vertices[k+5];
   V3 low=a.p,high=a.p;
   for(V3 p:{b.p,d.p,world.vertices[k+2].p}){low={std::min(low.x,p.x),std::min(low.y,p.y),std::min(low.z,p.z)};high={std::max(high.x,p.x),std::max(high.y,p.y),std::max(high.z,p.z)};}
   std::vector<size_t> nearby;
   for(int x=cell(low.x);x<=cell(high.x);x++)for(int z=cell(low.z);z<=cell(high.z);z++){
     auto found=cells.find({x,z});if(found!=cells.end())nearby.insert(nearby.end(),found->second.begin(),found->second.end());
   }
   std::sort(nearby.begin(),nearby.end());nearby.erase(std::unique(nearby.begin(),nearby.end()),nearby.end());
   for(int row=0;row<48;row++){
     float v=(row+.5f)/48;V3 start=mix(a.p,d.p,v),delta=b.p-a.p;archSections++;
     for(size_t index:nearby){
       const auto& f=faces[index];const auto& q=world.vertices[f.index];
       if(q.material==58||q.material==64)continue;
       if(low.x>f.high.x||high.x<f.low.x||low.y>f.high.y||high.y<f.low.y||low.z>f.high.z||high.z<f.low.z)continue;
       const auto& e=world.vertices[f.index+1];const auto& g=world.vertices[f.index+2];
       V3 e1=e.p-q.p,e2=g.p-q.p,p=cross(delta,e2);float det=dot(e1,p);if(std::abs(det)<1e-7f)continue;
       V3 distance=start-q.p;float u=dot(distance,p)/det;if(u<0||u>1)continue;
       V3 crossD=cross(distance,e1);float w=dot(delta,crossD)/det;if(w<0||u+w>1)continue;
       float t=dot(e2,crossD)/det;if(t<=.001f||t>=.999f)continue;
       if(!arches.opaque(a.u+(b.u-a.u)*t,a.v+(d.v-a.v)*v,166))continue;
       auto position=road.locate(start+delta*t);
       // Feet are deliberately embedded by 7 cm in their supporting apron.
       if((start+delta*t).y<road.surface(position.s,position.d).y+.035f)continue;
       throw std::runtime_error("Painted arch intersects static material "+std::to_string(q.material)+" at s="+std::to_string(position.s)+" d="+std::to_string(position.d)+" above deck="+std::to_string((start+delta*t).y-road.surface(position.s,position.d).y));
     }
   }
 }
 size_t checked=0,segments=0;
 for(float at:{100.f,440.f,890.f,1200.f,1410.f,1460.f,1660.f,1860.f,2230.f,2620.f,2950.f,3340.f,3390.f,3660.f,3990.f,4015.f,4045.f,4080.f,4112.f}){
   for(float angle:{-1.f,0.f,1.f,2.4f}){
     auto r=road.at(at);V3 camera=r.p+V3{-6*std::sin(r.heading+angle),2,-6*std::cos(r.heading+angle)};
     Rider rider;rider.reset(road);rider.s=at;rider.p=r.p;Mesh props;appendTrees(props,road,rider,camera);
     for(size_t k=0;k<props.size();k+=6){
       const auto& a=props[k];const auto& b=props[k+1];const auto& d=props[k+5];
       if(a.material<29||a.material>=29.5f||length(d.p-camera)>38)continue;
       checked++;
       V3 corners[]={a.p,b.p,props[k+2].p,d.p};V3 low=a.p,high=a.p;
       for(auto p:corners){low={std::min(low.x,p.x),std::min(low.y,p.y),std::min(low.z,p.z)};high={std::max(high.x,p.x),std::max(high.y,p.y),std::max(high.z,p.z)};}
       std::vector<size_t> nearby;
       for(int x=cell(low.x);x<=cell(high.x);x++)for(int z=cell(low.z);z<=cell(high.z);z++){
         auto found=cells.find({x,z});if(found!=cells.end())nearby.insert(nearby.end(),found->second.begin(),found->second.end());
       }
       std::sort(nearby.begin(),nearby.end());nearby.erase(std::unique(nearby.begin(),nearby.end()),nearby.end());
       for(int row=0;row<32;row++){
         float v=(row+.5f)/32;V3 start=mix(a.p,d.p,v),delta=b.p-a.p;segments++;
         for(size_t index:nearby){
           const auto& f=faces[index];
           if(low.x>f.high.x||high.x<f.low.x||low.y>f.high.y||high.y<f.low.y||low.z>f.high.z||high.z<f.low.z)continue;
           const auto& q=world.vertices[f.index];const auto& e=world.vertices[f.index+1];const auto& g=world.vertices[f.index+2];
           V3 e1=e.p-q.p,e2=g.p-q.p,p=cross(delta,e2);float determinant=dot(e1,p);if(std::abs(determinant)<1e-7f)continue;
           V3 distance=start-q.p;float u=dot(distance,p)/determinant;if(u<0||u>1)continue;
           V3 crossD=cross(distance,e1);float w=dot(delta,crossD)/determinant;if(w<0||u+w>1)continue;
           float t=dot(e2,crossD)/determinant;if(t<=.001f||t>=.999f)continue;
           if(!furniture.opaque(a.u+(b.u-a.u)*t,a.v+(d.v-a.v)*v,185))continue;
           if(q.material==58&&!arches.opaque(q.u+(e.u-q.u)*u+(g.u-q.u)*w,q.v+(e.v-q.v)*u+(g.v-q.v)*w,166))continue;
           throw std::runtime_error("Opaque furniture intersects static material "+std::to_string(q.material)+" near s="+std::to_string(at)+" atlas u="+std::to_string(a.u));
         }
       }
     }
   }
 }
 if(checked<400)throw std::runtime_error("Clearance pass removed too much trackside furniture");
 // Other towers must not put facade modules inside one of the fixed pavilions.
 auto end=road.at(road.size());V3 x{std::cos(end.heading),0,-std::sin(end.heading)},z{std::sin(end.heading),0,std::cos(end.heading)};
 for(int side:{-1,1})for(int pavilion=0;pavilion<3;pavilion++){
   V3 center=road.at(4028+pavilion*44.f).p+x*(side*34.f);
   for(size_t k=0;k<world.vertices.size();k+=3){
     if(world.vertices[k].material!=30&&world.vertices[k].material!=70)continue;
     V3 p=(world.vertices[k].p+world.vertices[k+1].p+world.vertices[k+2].p)*(1.f/3)-center;
     if(std::abs(dot(p,x))<14.95f&&std::abs(dot(p,z))<11.95f&&p.y>.05f&&p.y<16+pavilion*4-.05f)
       throw std::runtime_error("A city facade penetrates a terminal pavilion");
   }
 }
 std::cout<<"PASS Nova overlap regression: "<<checked<<" nearby sprites at 76 views, "<<segments<<" alpha-aware cross-sections, "<<archSections<<" arch sections; terminal pavilion clearance\n";
}

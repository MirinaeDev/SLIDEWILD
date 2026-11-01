#pragma once
#include <filesystem>
#include <fstream>
#include <map>
#include <array>
// Inspect actual rendered sprite quads and opaque pixels against the rendered
// support surface. This does not call the production contact-placement helper.
inline void checkArrivalSprites(const dh::Road& road){
 using namespace dh;
 struct Atlas {unsigned w=0,h=0;std::vector<unsigned char> pixels;};
 static std::map<std::string,Atlas> atlases;
 auto load=[&](const std::string& name)->Atlas&{
   auto& a=atlases[name];if(!a.pixels.empty())return a;
   auto path=std::filesystem::path(SDL_GetBasePath())/"assets"/(name+".rgba");
   if(!std::filesystem::exists(path))path=std::filesystem::current_path()/"assets"/(name+".rgba");
   std::ifstream f(path,std::ios::binary);f.read(reinterpret_cast<char*>(&a.w),4);f.read(reinterpret_cast<char*>(&a.h),4);
   if(!f||!a.w||!a.h)throw std::runtime_error("Missing arrival sprite atlas");
   a.pixels.resize(size_t(a.w)*a.h*4);f.read(reinterpret_cast<char*>(a.pixels.data()),a.pixels.size());
   if(!f)throw std::runtime_error("Incomplete arrival sprite atlas");
   return a;
 };
 const char* biomes[]={"","desert","urban","scotland","tropical"};size_t checked=0;
 auto end=road.at(road.size());V3 forward{std::sin(end.heading),0,std::cos(end.heading)},right{forward.z,0,-forward.x};
 for(float at:{4030.f,4080.f,4125.f})for(float angle:{-.7f,0.f,.7f}){
   Rider rider;rider.reset(road);rider.s=at;rider.p=road.surface(at,0);
   V3 camera=rider.p+V3{-5*std::sin(end.heading+angle),2.2f,-5*std::cos(end.heading+angle)};
   Mesh props;appendTrees(props,road,rider,camera);
   for(size_t k=0;k+5<props.size();k+=6){
     const auto& a=props[k];const auto& b=props[k+1];const auto& d=props[k+5];float mat=a.material;
     std::string name;
     if(road.biome()==0){if(mat==27)name="parking-props-v16";else if(mat>=20&&mat<20.5f)name="grass-v3";else continue;}
     else if(road.course==7){if(mat>=29&&mat<29.5f)name="caldeira-props-v24";else continue;}
     else if(road.course<=4){
       std::string prefix=biomes[road.biome()];
       if(mat>=29&&mat<29.5f)name=prefix+"-props-v18";
       else if(mat>=32&&mat<32.5f)name=prefix+"-details-v18";
       else if(mat>=33&&mat<33.5f)name=prefix+"-habitat-v18";
       else continue;
     }else continue;
     V3 middle=(d.p+props[k+2].p)*.5f,delta=middle-end.p;
     if(dot(delta,forward)<-140||dot(delta,forward)>-2||std::abs(dot(delta,right))>21||length(middle-camera)>85||length(a.p-d.p)>6)continue;
     auto& atlas=load(name);
     float expected=std::abs((b.u-a.u)*atlas.w/((d.v-a.v)*atlas.h));
     if(std::abs(length(b.p-a.p)/length(a.p-d.p)-expected)>.003f||std::abs(dot(norm(b.p-a.p),norm(a.p-d.p)))>.001f)
       throw std::runtime_error("Arrival sprite loses its native aspect or orthogonal basis");
     int x0=int(std::round(std::min(a.u,b.u)*atlas.w)),x1=int(std::round(std::max(a.u,b.u)*atlas.w));
     int y0=int(std::round(a.v*atlas.h)),y1=int(std::round(d.v*atlas.h));
     int foot=y1-1;std::vector<int> opaque;
     for(;foot>=y0;foot--){opaque.clear();for(int x=x0;x<x1;x++)if(atlas.pixels[(size_t(foot)*atlas.w+x)*4+3]>190)opaque.push_back(x);if(opaque.size()>=2)break;}
     if(opaque.empty())throw std::runtime_error("Empty arrival drawing");
     float u=((opaque[opaque.size()/2]+.5f)/atlas.w-a.u)/(b.u-a.u),v=(float(foot+1)/atlas.h-a.v)/(d.v-a.v);
     V3 contact=a.p+(b.p-a.p)*u+(d.p-a.p)*v;
     float gap=contact.y-sceneryHeight(contact);
     if(gap>.045f||gap<-.16f)throw std::runtime_error("Arrival sprite contact is off its rendered support, course="+std::to_string(road.course)+" material="+std::to_string(mat)+" gap="+std::to_string(gap)+" xyz="+std::to_string(contact.x)+","+std::to_string(contact.y)+","+std::to_string(contact.z)+" uv="+std::to_string(a.u)+","+std::to_string(a.v)+" along="+std::to_string(dot(delta,forward)));
     checked++;
   }
 }
 if(checked<20)throw std::runtime_error("Too few arrival sprites to verify");
 std::cout<<"PASS arrival sprites course "<<road.course<<": "<<checked<<" alpha contacts on rendered ground; native aspect and nine camera views\n";
}

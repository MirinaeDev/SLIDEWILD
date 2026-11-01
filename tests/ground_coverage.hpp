#pragma once
#include "world.hpp"
#include <map>
#include <stdexcept>
#include <iostream>
inline void checkGroundSeams(const dh::StaticWorld& world,const dh::Road& road){
 using namespace dh;int selected=road.course;
 // Sample shoulders between the road ribbon and the coarser heightfield.
   // Closed rocks alone do not prove that these two ground meshes meet.
   std::map<std::pair<int,int>,std::vector<size_t>> groundCells;
   for(size_t i=0;i<world.vertices.size();i+=3){
     if(world.vertices[i].material!=3&&world.vertices[i].material!=31&&world.vertices[i].material!=51)continue;
     auto a=world.vertices[i].p,b=world.vertices[i+1].p,c=world.vertices[i+2].p;
     for(int x=int(std::floor(std::min({a.x,b.x,c.x})/8));x<=int(std::floor(std::max({a.x,b.x,c.x})/8));x++)
       for(int z=int(std::floor(std::min({a.z,b.z,c.z})/8));z<=int(std::floor(std::max({a.z,b.z,c.z})/8));z++)groundCells[{x,z}].push_back(i);
   }
   int coverage=0;
   for(float s=10;s<4100;s+=31)for(int side:{-1,1})for(float offset:{5.8f,7.f,10.f,11.8f,13.5f}){
     V3 p=road.surface(s,side*offset);if(road.locate(p).distance<5)continue;bool covered=false;
     for(auto i:groundCells[{int(std::floor(p.x/8)),int(std::floor(p.z/8))}]){
       V3 a=world.vertices[i].p,e=world.vertices[i+1].p-a,f=world.vertices[i+2].p-a,q=p-a;
       float det=e.x*f.z-e.z*f.x;if(std::abs(det)<1e-6f)continue;
       float u=(q.x*f.z-q.z*f.x)/det,v=(e.x*q.z-e.z*q.x)/det;
       if(u>=-.001f&&v>=-.001f&&u+v<=1.001f){covered=true;break;}
     }
     if(!covered)throw std::runtime_error("Open ground seam course "+std::to_string(selected)+" s "+std::to_string(s)+" offset "+std::to_string(side*offset));
     coverage++;
   }
   std::cout<<"PASS ground seam coverage "<<selected<<": "<<coverage<<" shoulder samples\n";
}

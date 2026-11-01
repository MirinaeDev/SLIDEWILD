#include "camera_projection.hpp"
#include <iostream>
#include <stdexcept>
using namespace dh;
int main(){try{
 auto reversed=perspectiveReversed(69*pi/180,16.f/9,.1f,8000);
 auto depth=[&](float z){return (reversed.m[10]*z+reversed.m[11])/z;};
 if(std::abs(depth(.1f)-1)>1e-6f||std::abs(depth(8000))>1e-7f||depth(.05f)<=1||depth(8100)>=0)throw std::runtime_error("Reversed depth clip range mismatch");
 int collapsed=0;auto old=perspective(69*pi/180,16.f/9,.1f,8000);
 for(float distance:{100.f,300.f,500.f,1000.f,2000.f,4000.f,7500.f}){
   float back=depth(distance),front=depth(distance-.025f);
   if(!(front>back))throw std::runtime_error("Facade lost 2.5 cm depth separation");
   float oldBack=(old.m[10]*distance+old.m[11])/distance;
   float oldFront=(old.m[10]*(distance-.025f)+old.m[11])/(distance-.025f);
   collapsed+=oldFront==oldBack;
 }
 if(!collapsed)throw std::runtime_error("Depth regression fixture no longer reproduces original precision loss");
 std::cout<<"PASS reversed D32 depth: near/far clipping, 2.5 cm facade separation to 7500 m; old projection collapsed "<<collapsed<<" samples\n";
 size_t samples=0;
 for(float aspect:{16.f/9,21.f/9})for(float roll:{-.6f,-.15f,0.f,.15f,.6f})
 for(float pitch:{-.6f,0.f,.35f})for(float yaw:{-2.f,0.f,1.7f})for(float fov:{66.f,95.f}){
   V3 eye{37,18,-25},forward{std::sin(yaw)*std::cos(pitch),std::sin(pitch),std::cos(yaw)*std::cos(pitch)};
   auto camera=cameraProjection(eye,eye+forward*20,fov*pi/180,aspect,roll);
   for(float u:{.05f,.3f,.5f,.9f})for(float v:{.1f,.5f,.85f}){
     // A sky ray and a mesh landmark on that ray must land on the same pixel,
     // across roll, pitch, FOV and aspect changes. Error tolerance < .01 pixel.
     V3 p=eye+camera.ray(u,v)*500;float q[]={p.x,p.y,p.z,1},clip[4]{};
     for(int row=0;row<4;row++)for(int col=0;col<4;col++)clip[row]+=camera.matrix.m[row*4+col]*q[col];
     float screenU=(clip[0]/clip[3]+1)*.5f,screenV=(1-clip[1]/clip[3])*.5f;
     if(std::abs(screenU-u)>1e-5f||std::abs(screenV-v)>1e-5f)throw std::runtime_error("Panorama ray and world projection disagree");
     samples++;
   }
 }
 std::cout<<"PASS shared camera frame: "<<samples<<" world/sky landmarks, both roll directions, steep pitch, 16:9/21:9 and speed FOV\n";
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}

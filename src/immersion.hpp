#pragma once
#include "simulation.hpp"
namespace dh {
// These analytic road-space masks are mirrored in world.hlsl. They describe
// real places on the course, not a screen overlay or a random camera timer.
inline float repairMask(float s,float d){
 float along=std::abs(std::sin(s*.081f+.8f)),across=std::abs(d-.75f*std::sin(s*.043f));
 return clamp((.27f-along)*9,0,1)*clamp((3.3f-across)*2,0,1);
}
inline float waterMask(float s,float d,float wet){
 float pool=std::sin(s*.117f+d*.29f)*std::sin(s*.047f-d*.83f);
 return wet*clamp((pool-.39f)*3.2f,0,1)*clamp((4.2f-std::abs(d))*2,0,1);
}
inline Mat sunlightMatrix(V3 center){
 V3 sun=norm(V3{.70f,.32f,.64f}),right=norm(cross(V3{0,1,0},sun*-1)),up=cross(sun*-1,right);
 constexpr float size=100,texel=size/1536;
 center=center+right*(std::round(dot(center,right)/texel)*texel-dot(center,right))
              +up*(std::round(dot(center,up)/texel)*texel-dot(center,up));
 Mat projection=identity();projection.m[0]=projection.m[5]=2/size;
 projection.m[10]=1.f/239;projection.m[11]=-1.f/239;
 return projection*view(center+sun*110,center);
}
struct ImmersionState {
 float rough=0,water=0,slideEntry=0,gripReturn=0,exposure=1;
 float lastDrift=0;
 void tick(const Rider&r,float wet,float dt){
   if(dt<=0)return;
   rough=repairMask(r.s,r.d);water=waterMask(r.s,r.d,wet);
   slideEntry*=std::exp(-dt*12);gripReturn*=std::exp(-dt*10);
   if(lastDrift<.3f&&r.drift>=.3f)slideEntry=1;
   if(lastDrift>.65f&&r.drift<=.65f&&!r.crashed)gripReturn=1;
   lastDrift=r.drift;
   // Small, slow artistic exposure compensation; never changes driving physics.
   float shade=.5f+.5f*std::sin(r.s*.018f);
   exposure=approach(exposure,1+shade*.045f-wet*.015f,1.8f,dt);
 }
};
}

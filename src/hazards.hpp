#pragma once
#include "math.hpp"
#include <array>
namespace dh {
enum class HazardKind {Basalt,LavaVent};
struct RoadHazard {float s,d,radius;HazardKind kind;float phase;};
// Authored in readable gaps between corners. No hazard blocks both lanes.
inline constexpr std::array<RoadHazard,12> caldeiraHazards={{
 {370,-2,.85f,HazardKind::Basalt,0},{575,2.1f,.9f,HazardKind::LavaVent,1.1f},
 {1005,1.9f,.9f,HazardKind::Basalt,0},{1230,-2.1f,.95f,HazardKind::LavaVent,3.4f},
 {1600,-1.9f,.85f,HazardKind::Basalt,0},{1990,2.1f,1,HazardKind::LavaVent,5.7f},
 {2210,-2,.9f,HazardKind::Basalt,0},{2640,2,.9f,HazardKind::Basalt,0},
 {2845,-2.1f,.95f,HazardKind::LavaVent,2.3f},{3210,-2,.85f,HazardKind::Basalt,0},
 {3450,2.1f,1,HazardKind::LavaVent,4.1f},{3650,1.9f,.85f,HazardKind::Basalt,0}
}};
inline float lavaPhase(float time,float offset){return std::fmod(std::max(0.f,time+offset),8.f);}
inline float lavaBurst(float time,float offset){
 float phase=lavaPhase(time,offset);
 return clamp((phase-3.4f)/.35f,0,1)*clamp((5.35f-phase)/.65f,0,1);
}
inline bool hazardActive(const RoadHazard& h,float time){
 float phase=lavaPhase(time,h.phase);
 return h.kind==HazardKind::Basalt||(phase>=3.4f&&phase<5.35f);
}
inline const RoadHazard* upcomingHazard(float s,float range=160){
 for(const auto& h:caldeiraHazards)if(h.s>=s-8&&h.s<=s+range)return &h;
 return nullptr;
}
inline float hazardInfluence(float s){
 auto h=upcomingHazard(s);if(!h)return 0;
 float ahead=h->s-s;
 float enter=clamp((160-ahead)/75,0,1),leave=clamp((ahead+8)/8,0,1);
 return enter*enter*(3-2*enter)*leave;
}
inline float hazardLane(float s){
 auto h=upcomingHazard(s);return h?(h->d<0?1.75f:-1.75f)*hazardInfluence(s):0;
}
// Swept collision in road coordinates; a fast frame cannot skip a boulder.
inline bool hitsRoadHazard(float oldS,float oldD,float s,float d,float time){
 for(const auto& h:caldeiraHazards){
   if(!hazardActive(h,time)||s<h.s-h.radius-.65f||oldS>h.s+h.radius+.65f)continue;
   float ds=s-oldS,dd=d-oldD,scaleS=h.radius+.65f,scaleD=h.radius+.35f;
   float x=(oldS-h.s)/scaleS,y=(oldD-h.d)/scaleD,vx=ds/scaleS,vy=dd/scaleD;
   float t=clamp(-(x*vx+y*vy)/std::max(.000001f,vx*vx+vy*vy),0,1);
   x+=vx*t;y+=vy*t;if(x*x+y*y<=1)return true;
 }return false;
}
}

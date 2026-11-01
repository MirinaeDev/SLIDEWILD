#pragma once
#include "simulation.hpp"
#include <array>
namespace dh {
inline Input racingInput(const Road& road,const Rider& r,float wet,float lane,int level,int personality){
 if(level==0){
   Rider steering=r;steering.d-=lane*(road.course==7?1-hazardInfluence(r.s):1);
   auto input=demoInput(road,steering,wet);
   float ceiling=std::min(49.f+personality,cornerSpeed(road,r.s,wet,true,r.pilot)*(.96f+personality*.012f));
   input.brake=std::max(input.brake,clamp((r.pace-ceiling)*.45f,0,1));return input;
 }
 return previewInput(road,r,wet,lane,level,personality);
}
// Each opponent drives the same Rider simulation. No position teleportation,
// hidden speed boost or rubber band depends on the human player's rank.
struct Race {
 struct Rival {Rider rider,previous;float lane=0,finish=-1;};
 std::array<Rival,3> rivals{};
 bool enabled=false,started=false;int difficulty=1;float elapsed=0,playerFinish=-1;
 static constexpr const char* difficultyNames[3]={"AMATEUR","ARCADE","EXPERT"};
 void reset(const Road&road,const Rider&player,bool active,int level){
   *this=Race{};enabled=active;difficulty=std::clamp(level,0,2);
   for(int j=0;j<3;j++){
     auto& b=rivals[j];b.rider.reset(road);b.rider.pilot=(player.pilot+j+1)%4;
     b.rider.s=std::max(1.f,player.s+9+j*8);b.lane=(j%2?1.f:-1.f)*1.25f;b.rider.d=b.lane;
     b.rider.p=road.surface(b.rider.s,b.lane);b.rider.heading=road.at(b.rider.s).heading;
     b.rider.pace=player.pace;b.rider.velocity=V3{std::sin(b.rider.heading),0,std::cos(b.rider.heading)}*player.pace;b.previous=b.rider;
   }
 }
 void tick(const Road&road,Rider&player,float overrideWet,bool countdown,float dt,float previousPlayerS=-1){
   if(!enabled||dt<=0)return;
   if(!countdown)started=true;
   if(started)elapsed+=dt;
   for(auto&b:rivals)b.previous=b.rider;
   for(int j=0;j<3;j++){
     auto&b=rivals[j];auto&r=b.rider;
     if(b.finish>=0){
       r.pace*=std::exp(-dt*.8f);r.s=std::min(Road::runoutEnd,r.s+r.pace*dt);
       r.d=approach(r.d,(j-1)*2.4f,2,dt);r.p=road.surface(r.s,r.d);r.heading=road.at(r.s).heading;
       r.velocity=V3{std::sin(r.heading),0,std::cos(r.heading)}*r.pace;r.drift=0;r.lean=0;r.crouch=0;
       r.slideAngle=0;r.slideAngularSpeed=0;r.overRotation=0;r.footBrake=r.pace>1?1:0;
       r.wheelTravel+=r.pace*dt;r.runTime+=dt;r.animatePose(dt);continue;
     }
     float wet=std::max(overrideWet,courseRain(road.course,r.s));
     float curvature=wrap(road.at(r.s+12).heading-road.at(r.s+10).heading)/2;
     float apexLane=std::copysign((1.3f+difficulty*.25f)*clamp(std::abs(curvature)/.028f,0,1),curvature);
     float lane=mix((j%2?1.f:-1.f)*.75f,apexLane,clamp(std::abs(curvature)*75,0,1)),nearS=1000,nearD=0;
     auto obstacle=[&](const Rider&o){float ds=o.s-r.s;if(ds>-.5f&&ds<18&&std::abs(o.d-r.d)<1.1f&&ds<nearS){nearS=ds;nearD=o.d;}};
     obstacle(player);for(int k=0;k<3;k++)if(k!=j)obstacle(rivals[k].rider);
     if(nearS<18)lane=nearD>0?-1.6f:1.6f;
     b.lane=approach(b.lane,lane,2,dt);
     Input in=racingInput(road,r,wet,b.lane,difficulty,j);
     if(countdown)in.brake=std::max(in.brake,clamp((r.pace-22.f)*.42f,0,1));
     if(nearS<5&&std::abs(nearD-r.d)<.8f)in.brake=std::max(in.brake,.65f);
     in.tuck=!countdown&&in.brake<.03f&&(difficulty>0||!in.slide);
     float oldS=r.s;r.tick(road,in,wet,dt);
     if(r.finished){float part=clamp((Road::finishDistance-oldS)/std::max(.0001f,r.s-oldS),0,1);b.finish=elapsed-dt+part*dt;}
   }
   // Soft body contacts cost speed, while avoidance supplies passing lanes.
   // They do not teleport either rider or throw the human straight into a wall.
   if(playerFinish<0&&!player.crashed&&!countdown)for(auto&b:rivals){
     auto&r=b.rider;if(r.crashed||r.finished)continue;
     if(std::abs(player.s-r.s)<1.0f&&std::abs(player.d-r.d)<.65f){
       float drag=std::exp(-dt*.8f);player.pace*=drag;r.pace*=drag;
       player.velocity=player.velocity*drag;r.velocity=r.velocity*drag;
     }
   }
   if(player.finished&&playerFinish<0){
     float part=previousPlayerS>=0?clamp((Road::finishDistance-previousPlayerS)/std::max(.0001f,player.s-previousPlayerS),0,1):1;
     playerFinish=elapsed-dt+part*dt;
   }
 }
 int place(const Rider&player)const{
   int p=1;for(const auto&b:rivals){
     if(playerFinish>=0){if(b.finish>=0&&b.finish<playerFinish)p++;}
     else if(b.finish>=0||b.rider.s>player.s)p++;
   }return p;
 }
 void complete(const Road&road,Rider&player,float overrideWet){
   if(!enabled||playerFinish<0)return;
   // Once the human has stopped, resolve remaining opponents with the same
   // fixed-step simulation so the result sheet contains actual finish times.
   for(int step=0;step<21600;step++){
     bool done=true;for(const auto&b:rivals)done&=b.finish>=0;
     if(done)break;
     tick(road,player,overrideWet,false,1.f/120);
   }
 }
};
}

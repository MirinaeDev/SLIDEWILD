#pragma once
#include "simulation.hpp"
#include "immersion.hpp"
#include <algorithm>
namespace dh {
struct Puff { V3 p,v;float age=0,life=1,size=.1f;bool spray=false; };
struct Track { V3 a,b;float age=0; };
struct RainDrop { V3 p;float floor=0,s=0; };
struct Splash { V3 p;float age=0,seed=0; };
struct Debris {V3 p,v;float floor=0,age=0,life=1,seed=0;bool leaf=false;float roadAt=0;};
struct RideEffects {
 std::vector<Puff> puffs;std::vector<Track> tracks;
 std::vector<RainDrop> rain;std::vector<Splash> splashes;
 std::vector<Debris> debris;float debrisBudget=0;
 float rainBudget=0;unsigned rainSeed=0;
 float emission=0;unsigned seed=0;bool contact=false;V3 last[2]{};
 void reset(){*this=RideEffects{};}
 void tick(const Rider&r,const Road&road,float wet,float dt,float(*sceneryGround)(V3)=nullptr){
  if(dt<=0)return;
  float rough=repairMask(r.s,r.d),pool=waterMask(r.s,r.d,wet);
  for(auto&bit:debris){
    bit.age+=dt;bit.p=bit.p+bit.v*dt;bit.v.y-=dt*(bit.leaf?2.5f:8.f);
    bit.floor=roadGroundPoint(road,bit.p,bit.roadAt).y+.012f;
    if(bit.p.y<bit.floor){bit.p.y=bit.floor;bit.v=bit.v*.45f;bit.v.y=std::abs(bit.v.y)*.3f;}
  }
  std::erase_if(debris,[](const Debris&b){return b.age>b.life;});
  if(!r.crashed&&r.speed()>18&&wet<.5f){
    debrisBudget+=dt*(rough*8+r.drift*5+(std::abs(r.d)>3.5f?9:1));
    while(debrisBudget>=1){
      debrisBudget-=1;float n=hash(float(++seed));V3 f{std::sin(r.heading),0,std::cos(r.heading)},q{f.z,0,-f.x};
      V3 p=r.p-f*.3f+q*((n-.5f)*.7f);p=roadGroundPoint(road,p,r.s)+V3{0,.025f,0};
      bool leaf=n>.62f&&road.course<6;
      debris.push_back({p,q*((n-.5f)*2)+V3{0,leaf?.7f:.45f,0}-f*.7f,p.y,0,leaf?1.3f:.7f,n,leaf,r.s});
    }
  }
  if(debris.size()>96)debris.erase(debris.begin(),debris.end()-96);
  for(auto&s:splashes)s.age+=dt;
  std::erase_if(splashes,[](const Splash&s){return s.age>.24f;});
  for(auto&drop:rain){
   drop.p.y-=19.f*dt;
   if(drop.p.y<=drop.floor){
    if(std::abs(drop.s-r.s)<40)splashes.push_back({{drop.p.x,drop.floor+.012f,drop.p.z},0,hash(float(++rainSeed))});
    drop.floor=1e8f;
   }
  }
  std::erase_if(rain,[&](const RainDrop&d){return d.floor>1e7f||std::abs(d.s-r.s)>90;});
  rainBudget+=wet*1800*dt;
  while(rainBudget>=1){
   rainBudget-=1;float n=float(++rainSeed);
   bool near=rainSeed%2==0;
   float s=clamp(r.s+(near?-6.f:-14.f)+hash(n)*(near?31.f:85.f),0,road.size()),d=(hash(n+317)-.5f)*(near?14.f:25.f);
   V3 p=terrainPoint(road,s,d);if(std::abs(d)<Road::halfWidth)p=roadGroundPoint(road,p,s);
   else if(std::abs(d)>7&&sceneryGround)p.y=sceneryGround(p);
   float floor=p.y;
   p.y+=1.5f+hash(n+823)*(near?6.f:11.f);
   rain.push_back({p,floor,s});
  }
  if(rain.size()>1600)rain.erase(rain.begin(),rain.end()-1600);
  if(splashes.size()>500)splashes.erase(splashes.begin(),splashes.end()-500);
  for(auto&p:puffs){p.age+=dt;p.p=p.p+p.v*dt;p.v=p.v*std::exp(-dt*2);p.v.y+=dt*.35f;}
  for(auto&t:tracks)t.age+=dt;
  if(r.crashed&&r.crashTime==0){
   for(int i=0;i<16;i++){
     float n=hash(float(++seed)),angle=n*pi*2;
     puffs.push_back({r.p+V3{0,.12f,0},{std::cos(angle)*1.7f,.3f+n,std::sin(angle)*1.7f},0,.55f+n*.35f,.08f,wet>.5f});
   }
  }
  std::erase_if(puffs,[](const Puff&p){return p.age>=p.life;});
  std::erase_if(tracks,[](const Track&t){return t.age>7;});
  bool active=!r.crashed&&r.speed()>7&&(r.drift>.32f||wet>.5f);
  if(!active){emission=0;contact=false;return;}
  emission+=dt;
  float interval=1.f/(wet>.5f?45+pool*75:32+clamp(std::abs(r.slip),0,1.6f)*22);
  if(emission<interval)return;
  emission=std::fmod(emission,interval);
  V3 f{std::sin(r.heading),0,std::cos(r.heading)},side{f.z,0,-f.x};
  for(int k=0;k<2;k++){
   V3 wheel=r.p-f*.24f+side*(k==0?-.25f:.25f);auto rp=road.at(road.project(wheel,r.s));V3 lateral{std::cos(rp.heading),0,-std::sin(rp.heading)};wheel.y=rp.p.y+rp.bank*dot(wheel-rp.p,lateral)+.035f;
   if(contact&&r.drift>.32f&&wet<.6f&&length(wheel-last[k])<2)tracks.push_back({last[k],wheel,0});
   last[k]=wheel;
   float n=hash(float(++seed));bool spray=wet>.5f;
   V3 v=side*((n-.5f)*(spray?2.5f+pool*3:2.8f))+V3{.25f,spray?1.2f+pool*1.2f:1.1f,0}-f*(spray?2.f:0.f);
   // A little wake carries dry smoke above the deck before the chase camera
   // overtakes it; otherwise almost the entire puff is hidden below asphalt.
   if(!spray)v=v+r.velocity*.35f;
   float slideHeat=clamp(std::abs(r.slip)/1.57f,0,1)*clamp(r.speed()/30,0,1);
   puffs.push_back({wheel,v,0,spray?.45f+pool*.3f:.8f+n*.3f,spray?.04f+pool*.075f:.16f+n*.075f+slideHeat*.10f,spray});
   if(pool>.35f)splashes.push_back({wheel,0,n});
  }
  contact=true;
  if(puffs.size()>240)puffs.erase(puffs.begin(),puffs.end()-240);
  if(tracks.size()>1300)tracks.erase(tracks.begin(),tracks.end()-1300);
 }
};
}

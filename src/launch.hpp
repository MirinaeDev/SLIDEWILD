#pragma once
#include "simulation.hpp"
namespace dh {
// The countdown is a driving state, never a stopped simulation. Clock starts
// when control is handed over; velocity and position are left untouched.
struct RollingStart {
 float remaining=0,handover=0,go=0;bool crossed=false;
 void begin(){remaining=3;handover=0;go=0;crossed=false;}
 bool active()const{return remaining>0;}
 Input tick(const Road&road,const Rider&r,float wet,Input player,float dt){
   crossed=false;
   if(remaining>0){
     Input automatic=demoInput(road,r,wet);automatic.tuck=false;
     remaining=std::max(0.f,remaining-dt);
     if(remaining==0){crossed=true;handover=.30f;go=.9f;}
     return automatic;
   }
   go=std::max(0.f,go-dt);
   if(handover>0){
     handover=std::max(0.f,handover-dt);auto automatic=demoInput(road,r,wet);
     player.steer=mix(player.steer,automatic.steer,handover/.30f);
   }
   return player;
 }
};
}

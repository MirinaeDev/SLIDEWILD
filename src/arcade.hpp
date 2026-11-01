#pragma once
#include "simulation.hpp"
#include "ui.hpp"
namespace dh {
// Events are earned from simulation state, never from a repeating UI timer.
struct ArcadeFeedback {
 float hold=0,cooldown=0,life=0,previousBoost=0;int badge=-1,speedAwards=0,driftAwards=0;
 void tick(const Rider&r,float dt){
   if(dt<=0)return;
   life=std::max(0.f,life-dt);cooldown=std::max(0.f,cooldown-dt);
   if(r.crashed){hold=0;previousBoost=0;return;}
   hold=r.speed()>44.45f?hold+dt:0;
   if(hold>.8f&&cooldown==0){badge=0;life=2.3f;cooldown=14;speedAwards++;}
   if(r.boost>previousBoost+.15f&&r.edgeImpact==0){badge=1;life=2.3f;driftAwards++;}
   previousBoost=r.boost;
 }
 void draw(Mesh&m,int width,bool finish=false)const{
   if(!finish&&life<=0)return;
   int row=finish?2:badge;float age=2.3f-life;
   float w=finish?386.f:310.f,h=w/3.f;
   float entrance=1-std::pow(1-clamp(age/.22f,0,1),3);
   float exit=1-clamp(life/.24f,0,1);
   // Keep the badge's native proportions and size throughout its motion.
   // Rewards enter from the edge, leaving the upcoming corner visible.
   float x=finish?(width-w)*.5f:width-w-25+(1-entrance+exit*exit)*(w+30);
   uiImage(m,width,x,finish?112.f:96.f,w,h,-19,0,row/3.f,1,(row+1)/3.f);
 }
};
// The finish stops the race clock. Momentum continues into the parking area;
// control is handed to this short deterministic coast, then results open.
struct FinishRunout {
 bool active=false,done=false;float elapsed=0,entrySpeed=0,startS=0;
 void begin(const Rider&r){active=true;done=false;elapsed=0;entrySpeed=r.speed();startS=r.s;}
 void tick(Rider&r,const Road&road,float dt){
   if(!active||dt<=0)return;
   elapsed+=dt;
   float distance=std::min(110.f,entrySpeed*3.f);
   float duration=2*distance/std::max(entrySpeed,1.f),t=clamp(elapsed/std::max(duration,.1f),0,1);
   r.s=std::min(Road::runoutEnd,startS+distance*(2*t-t*t));
   r.d=approach(r.d,0,4,dt);r.p=road.surface(r.s,r.d);r.heading=road.at(r.s).heading;
   r.pace=entrySpeed*(1-t);r.velocity=V3{std::sin(r.heading),0,std::cos(r.heading)}*r.pace;
   r.lean=approach(r.lean,0,5,dt);r.drift=0;r.boardAngle=approach(r.boardAngle,0,7,dt);
   r.slideAngle=0;r.slideAngularSpeed=0;r.overRotation=0;r.footBrake=r.pace>1?1:0;
   r.wheelTravel+=r.pace*dt;r.windPhase+=windRate(r.pace)*dt;
   r.crouch=approach(r.crouch,0,4,dt);r.boost=0;r.runTime+=dt;r.animatePose(dt);
   if(elapsed>=6){active=false;done=true;r.velocity={};r.pace=0;}
 }
};
}

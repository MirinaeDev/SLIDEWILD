#pragma once
#include "ui.hpp"
#include "simulation.hpp"
#include <optional>
namespace dh {
inline void screenLine(Mesh& m,int width,V3 a,V3 b,float thickness,V3 color){
 V3 d=b-a;float l=std::sqrt(d.x*d.x+d.y*d.y);if(l<.01f)return;
 V3 n{-d.y/l*thickness*.5f,d.x/l*thickness*.5f,0};
 auto p=[&](V3 v){return V3{std::round(v.x)/width*2-1,1-std::round(v.y)/270,0};};
 quad(m,p(a+n),p(b+n),p(b-n),p(a-n),color,-1);
}
struct RoutePresentation {
 mutable std::optional<Road> route;
 void draw(Mesh&m,int width,int country,float age,bool race)const{
   if(!route||route->course!=country)route.emplace(country);
   const V3 ink{.025f,.045f,.08f},cream{1,.95f,.82f};
   const V3 palette[courseCount]={{.40f,.88f,.80f},{1,.62f,.30f},{.64f,.70f,1},{.62f,.86f,.62f},{.32f,.91f,.94f},{1,.43f,.29f},{.66f,.51f,1},{1,.33f,.14f}};
   V3 color=palette[country];float cx=width*.5f;
   uiPanel(m,width,0,0,float(width),540,ink,.91f);
   uiText(m,width,"NEXT DESCENT",42,35,1,color);
   uiText(m,width,race?"RACE / 4 RIDERS":"TIME ATTACK",width-215,38,0,cream);
   float arrival=1-std::pow(1-clamp(age/.45f,0,1),3);
   uiCentered(m,width,courses[country].name,cx+(1-arrival)*100,86,2,cream);
   uiCentered(m,width,std::string(courses[country].country)+" / 4.0 KM",cx,132,0,color);
   // True course geometry, including elevation, projected onto an isometric plane.
   std::vector<V3> path;V3 lo{1e9f,1e9f,0},hi{-1e9f,-1e9f,0};
   for(int i=0;i<=400;i++){
     auto p=route->at(i*10.f).p;V3 q{(p.x-p.z)*.7071f,(p.x+p.z)*.35355f-p.y*.8f,0};
     path.push_back(q);lo.x=std::min(lo.x,q.x);lo.y=std::min(lo.y,q.y);hi.x=std::max(hi.x,q.x);hi.y=std::max(hi.y,q.y);
   }
   float scale=std::min(width*.70f/std::max(hi.x-lo.x,1.f),210/std::max(hi.y-lo.y,1.f));
   for(auto&p:path){p.x=cx+(p.x-(lo.x+hi.x)*.5f)*scale;p.y=300+(p.y-(lo.y+hi.y)*.5f)*scale;}
   V3 origin{cx,321,0},u{width*.36f,65,0},v{-width*.27f,80,0};
   for(int j=-5;j<=5;j++){
     float t=j/5.f;screenLine(m,width,origin+u*t-v,origin+u*t+v,1,color*.25f);
     screenLine(m,width,origin+v*t-u,origin+v*t+u,1,color*.25f);
   }
   float progress=clamp((age-.30f)/1.65f,0,1);int drawn=int(progress*400);
   for(int i=1;i<=400;i++){
     screenLine(m,width,path[i-1]+V3{0,13,0},path[i]+V3{0,13,0},7,ink*.65f);
     screenLine(m,width,path[i-1],path[i],7,ink);
     screenLine(m,width,path[i-1],path[i],3,i<=drawn?color:color*.23f);
   }
   for(int i=0;i<=4;i++){
     auto p=path[i*100];bool lit=drawn>=i*100;
     screenLine(m,width,p,p+V3{0,13,0},1,color*.4f);
     rect(m,p.x-5,p.y-5,10,10,ink,width);rect(m,p.x-3,p.y-3,6,6,lit?cream:color*.4f,width);
     if(i==0||i==4)uiText(m,width,i==0?"START":"FINISH",p.x+10,p.y-7,0,lit?cream:color*.5f);
   }
   auto tip=path[drawn];rect(m,tip.x-2,tip.y-2,5,5,cream,width);
   uiCentered(m,width,progress<1?"SCOUTING THE COURSE":"MAKE YOUR LINE.",cx,458,1,progress<1?color:cream);
   for(int j=0;j<24;j++)rect(m,cx-119+j*10,493,7,3,float(j)/24<progress?color:color*.2f,width);
   uiCentered(m,width,"READ THE ROAD  /  BRAKE  /  SLIDE",cx,516,0,cream);
 }
};
}

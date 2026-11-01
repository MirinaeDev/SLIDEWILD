#pragma once
#include "math.hpp"
#include "pilots.hpp"
#include "courses.hpp"
#include "hazards.hpp"
#include "rider_pose.hpp"
#include <vector>
namespace dh {
struct RoadPoint {V3 p;float heading=0,bank=0;};
struct RoadLocation {float s=0,d=0,distance=0;};
struct Road {
 static constexpr float step=2,halfWidth=4.7f,finishDistance=4000,runoutEnd=4130;
 std::vector<RoadPoint> points;
 int course=0;
 int biome()const{return course==5?0:course;}
 float bankAssist(float s)const{float b=at(s).bank;return course>=5?-5.5f*b/std::sqrt(1+b*b):0;}
 explicit Road(int selected=0):course(std::clamp(selected,0,courseCount-1)){V3 p{};float h=0; for(int i=0;i<=2080;i++){
   float s=i*step;
   float k=.0007f*std::sin(s*.018f);
   // Compact, smoothly entered switchbacks, ~175 degrees each.
   if(s>440&&s<530)k-=.0533f*std::sin((s-440)/90*pi);
   if(s>690&&s<790)k+=.0480f*std::sin((s-690)/100*pi);
   if(s>1000&&s<1090)k-=.0533f*std::sin((s-1000)/90*pi);
   if(s>1240&&s<1340)k+=.0460f*std::sin((s-1240)/100*pi);
   if(s>1680&&s<1780)k-=.0475f*std::sin((s-1680)/100*pi);
   if(s>2050&&s<2160)k+=.0435f*std::sin((s-2050)/110*pi);
   if(s>2490&&s<2580)k-=.0530f*std::sin((s-2490)/90*pi);
   if(s>2820&&s<2930)k+=.0430f*std::sin((s-2820)/110*pi);
   if(s>3290&&s<3390)k-=.0475f*std::sin((s-3290)/100*pi);
   if(s>3670&&s<3780)k+=.0435f*std::sin((s-3670)/110*pi);
   if(course>0){
     // Curvature integrates to the authored angle, independent of turn length.
     // Desert: sweepers, linked S curves, one tight canyon bend. City: blocks,
     // boulevard chicanes and fast quays. Neither repeats alpine switchbacks.
     struct Bend {float begin,length,degrees;};
     const Bend desert[]={{170,240,-40},{510,190,72},{840,220,-55},{1170,140,90},
       {1480,190,-115},{1800,210,38},{2110,200,-48},{2430,105,145},
       {2750,200,-60},{3060,180,75},{3370,170,-95},{3670,180,35}};
     const Bend city[]={{200,65,-90},{350,60,90},{540,130,50},{740,70,-115},
       {925,85,70},{1130,65,-90},{1290,65,90},{1480,110,65},{1670,80,-130},
       {1890,70,95},{2090,115,-50},{2310,60,85},{2460,65,-85},{2630,105,-60},
       {2840,80,125},{3050,65,-90},{3220,65,90},{3400,100,-75},{3590,60,60},{3740,85,-55}};
     const Bend highlands[]={{190,125,-65},{385,85,105},{575,75,-105},{790,145,65},
       {1020,90,-145},{1235,85,130},{1430,110,-60},{1630,70,105},{1800,75,-100},
       {1980,140,75},{2195,90,-150},{2410,100,130},{2630,130,-65},{2840,75,100},
       {3025,85,-110},{3240,135,70},{3470,85,-130},{3690,95,115}};
     const Bend tropical[]={{200,155,-45},{440,75,95},{615,80,-90},{820,130,60},
       {1050,80,-120},{1250,90,115},{1460,120,-60},{1650,65,90},{1810,70,-90},
       {2000,150,55},{2240,90,-145},{2460,90,135},{2680,135,-65},{2890,70,95},
       {3070,75,-100},{3270,120,65},{3480,85,-125},{3690,90,105}};
     const Bend park[]={{170,215,-150},{465,210,155},{765,135,-80},{975,130,85},
       {1190,225,-180},{1485,220,175},{1780,70,-125},{1960,70,125},
       {2150,205,-155},{2430,205,155},{2725,65,-145},{2895,65,145},
       {3070,160,-100},{3290,165,100},{3530,65,-130},{3710,65,130}};
     const Bend future[]={{180,240,-110},{500,130,95},{700,95,-125},{940,240,155},
       {1270,100,-120},{1460,160,100},{1710,95,-125},{1950,250,180},
       {2300,95,-135},{2480,100,130},{2670,130,-100},{2870,100,110},
       {3060,240,-175},{3400,100,120},{3580,90,-125},{3750,110,110}};
     const Bend volcano[]={{180,170,-60},{430,85,110},{620,95,-115},{820,150,75},
       {1050,100,-140},{1250,95,130},{1440,130,-75},{1640,85,115},{1820,90,-120},
       {2020,150,80},{2250,100,-150},{2470,105,135},{2690,140,-75},{2900,85,110},
       {3080,95,-120},{3280,130,80},{3480,95,-135},{3690,105,120}};
     k=0;
     for(int j=0;j<(course==1?12:course==2?20:(course==5||course==6)?16:18);j++){
       auto bend=course==1?desert[j]:course==2?city[j]:course==3?highlands[j]:course==4?tropical[j]:course==5?park[j]:course==6?future[j]:volcano[j];
       if(s>bend.begin&&s<bend.begin+bend.length)
         k+=bend.degrees*pi/180*pi/(2*bend.length)*std::sin((s-bend.begin)/bend.length*pi);
     }
   }
   if(s>3900)k=0;
   // Smooth banking follows the curvature envelope, with no abrupt bank seams.
   float bank=course==7?-.58f*std::tanh(k*35):course==6?-1.22f*std::tanh(k*45):course==5?-.78f*std::tanh(k*52):clamp(-k*1.5f,-.055f,.055f);
   points.push_back({p,h,bank});
   float grade=.072f+.055f*std::sin(s*.018f)+.027f*std::sin(s*.047f);
   if(course==1)grade=.078f+.060f*std::sin(s*.014f)+.019f*std::sin(s*.039f);
   if(course==2)grade=.073f+.034f*std::sin(s*.021f)+.017f*std::sin(s*.051f);
   if(course==3)grade=.062f+.045f*std::sin(s*.017f)+.022f*std::sin(s*.053f);
   if(course==4)grade=.032f+.022f*std::sin(s*.012f)+.012f*std::sin(s*.037f);
   if(course==5)grade=.070f+.085f*std::sin(s*.016f)+.036f*std::sin(s*.039f);
   if(course==6){
     grade=.105f+.065f*std::sin(s*.014f);
     auto plunge=[&](float begin,float length,float depth){
       if(s>begin&&s<begin+length){float q=std::sin((s-begin)/length*pi);grade+=depth*q*q;}
     };
     plunge(140,220,.48f);plunge(920,240,.55f);plunge(1900,270,.62f);plunge(2990,270,.60f);
     plunge(400,140,-.22f);plunge(1220,150,-.22f);plunge(2220,160,-.22f);plunge(3420,150,-.22f);
   }
   if(course==7){
     grade=.075f+.05f*std::sin(s*.015f)+.018f*std::sin(s*.041f);
     for(float start:{270.f,1460.f,2920.f})if(s>start&&s<start+180){float q=std::sin((s-start)/180*pi);grade+=.23f*q*q;}
   }
   if(s>3900)grade*=clamp((finishDistance-s)/100,0,1);
   h+=k*step; p=p+V3{std::sin(h)*step,-grade*step,std::cos(h)*step};
 }}
 float size()const{return (points.size()-1)*step;}
 RoadPoint at(float s)const{float f=clamp(s/step,0,float(points.size()-1)-.001f);int i=int(f);return {mix(points[i].p,points[i+1].p,f-i),mix(points[i].heading,points[i+1].heading,f-i),mix(points[i].bank,points[i+1].bank,f-i)};}
 V3 surface(float s,float d)const{auto r=at(s);return r.p+V3{std::cos(r.heading)*d,r.bank*d,-std::sin(r.heading)*d};}
 float project(V3 p,float previous)const{
   int mid=int(previous/step),start=std::max(0,mid-25),end=std::min(int(points.size())-2,mid+25);float best=1e20f,result=previous;
   for(int i=start;i<=end;i++){V3 a=points[i].p,b=points[i+1].p,ab=b-a;ab.y=0;V3 ap=p-a;ap.y=0;float t=clamp(dot(ap,ab)/dot(ab,ab),0,1);float ds=dot(ap-ab*t,ap-ab*t);if(ds<best){best=ds;result=(i+t)*step;}}
   return result;
 }
 RoadLocation locate(V3 p)const{
   float best=1e20f,at=0;
   for(size_t i=0;i+1<points.size();i++){
     V3 a=points[i].p,v=points[i+1].p-a,q=p-a;v.y=q.y=0;
     float t=clamp(dot(q,v)/dot(v,v),0,1),ds=dot(q-v*t,q-v*t);
     if(ds<best){best=ds;at=(float(i)+t)*step;}
   }
   auto r=this->at(at);V3 right{std::cos(r.heading),0,-std::sin(r.heading)};
   return {at,dot(p-r.p,right),std::sqrt(best)};
 }
};
struct Input {float steer=0,brake=0;bool tuck=false,slide=false;};
// Arcade board yaw and travel direction are intentionally independent.
inline float terrainOffset(float d){float x=std::abs(d);return x<=5.3f?0:(d<0?(x-5.3f)*.72f:-(std::min(x-5.3f,24.7f)*1.05f+std::max(0.f,x-30.f)*.48f));}
inline bool stoneWallAt(float s){return s<Road::finishDistance&&std::fmod(s,145.f)<64&&!(s>=410&&s<=466);}
inline bool hayBalesAt(float s){return s>=410&&s<=466;}
inline float clearPlantOffset(float s,int side,float halfWidth,float desired){
 // Check the full longitudinal footprint too, including wall endpoints.
 float barrier=side>0?5.15f:5.05f;
 if(side<0)for(float delta:{-halfWidth,0.f,halfWidth}){
   float at=clamp(s+delta,0,Road::finishDistance);
   barrier=std::max(barrier,stoneWallAt(at)?5.98f:(hayBalesAt(at)?5.36f:5.05f));
 }
 return side*std::max(std::abs(desired),barrier+halfWidth+.22f);
}
inline float cliffWeight(float s){
 // Deliberately different sectors, with a 30m transition into grassy slopes.
 auto window=[&](float a,float b){return clamp((s-a)/30,0,1)*clamp((b-s)/30,0,1);};
 return std::max({window(340,580),window(900,1120),window(1630,1840),window(2450,2650)});
}
inline float terrainElevation(float s,float d){
 if(s>=Road::finishDistance)return 0;
 float height=terrainOffset(d),edge=clamp((std::abs(d)-5.3f)/5,0,1);
 if(d<0)height*=.28f+.72f*cliffWeight(s);
 float rolling=1.25f*std::sin(s*.095f+d*.24f)+.60f*std::sin(s*.23f-d*.38f)+.24f*std::sin(s*.67f+d*.8f);
 return height+rolling*edge*(1-cliffWeight(s)*.8f);
}
inline V3 terrainPoint(const Road& road,float s,float d){return road.surface(s,d)+V3{0,terrainElevation(s,d),0};}
inline V3 groundPoint(const Road&road,V3 p,float hint,bool roadOnly=false){
 float s=clamp(hint,0,road.size()-.05f);auto r=road.at(s);V3 side{std::cos(r.heading),0,-std::sin(r.heading)};
 float d=dot(p-r.p,side);
 // Invert the actual curved road frame; nearest centre-line projection is not
 // sufficient on banked hairpins or for terrain far from the centre line.
 for(int iteration=0;iteration<8;iteration++){
  V3 q=road.surface(s,d),e=p-q;
  if(e.x*e.x+e.z*e.z<1e-8f)break;
  V3 ds=(road.surface(s+.02f,d)-road.surface(std::max(0.f,s-.02f),d))*(1.f/.04f);
  V3 dd=road.surface(s,d+1)-q;float determinant=ds.x*dd.z-ds.z*dd.x;
  if(std::abs(determinant)<.04f)break;
  s=clamp(s+clamp((e.x*dd.z-e.z*dd.x)/determinant,-3,3),0,road.size()-.05f);
  d+=clamp((ds.x*e.z-ds.z*e.x)/determinant,-3,3);
 }
 p.y=road.surface(s,d).y+(roadOnly?0:terrainElevation(s,d));return p;
}
inline V3 roadGroundPoint(const Road&road,V3 p,float hint){
 int mid=int(hint/Road::step);
 for(int index=std::max(0,mid-5);index<=std::min(int(road.points.size())-2,mid+5);index++){
  float s=index*Road::step;V3 a=road.surface(s,-Road::halfWidth),b=road.surface(s+Road::step,-Road::halfWidth),c=road.surface(s+Road::step,Road::halfWidth),d=road.surface(s,Road::halfWidth);
  for(int tri=0;tri<2;tri++){
   V3 e=(tri?c:b)-a,f=(tri?d:c)-a,q=p-a;float det=e.x*f.z-e.z*f.x;
   if(std::abs(det)<1e-6f)continue;
   float u=(q.x*f.z-q.z*f.x)/det,v=(e.x*q.z-e.z*q.x)/det;
   if(u>=-.0001f&&v>=-.0001f&&u+v<=1.0001f){p.y=a.y+e.y*u+f.y*v;return p;}
  }
 }
 return groundPoint(road,p,hint,true);
}
// Cache exact road triangles once per rendered frame. Shadows and splash rings
// query thousands of points; rebuilding the same trigonometry per point stalls.
struct RoadSurfaceQuery {
 struct Face {V3 a,e,f;float invDet,minX,maxX,minZ,maxZ;};
 std::vector<Face> faces;
 RoadSurfaceQuery(const Road&road,float hint,int radius=28){
   int mid=int(hint/Road::step);faces.reserve(size_t(radius*4+2));
   for(int i=std::max(0,mid-radius);i<=std::min(int(road.points.size())-2,mid+radius);i++){
     float s=i*Road::step;
     V3 a=road.surface(s,-Road::halfWidth),b=road.surface(s+2,-Road::halfWidth),c=road.surface(s+2,Road::halfWidth),d=road.surface(s,Road::halfWidth);
     for(int tri=0;tri<2;tri++){
       V3 v=tri?c:b,w=tri?d:c,e=v-a,f=w-a;float det=e.x*f.z-e.z*f.x;
       if(std::abs(det)<1e-6f)continue;
       faces.push_back({a,e,f,1/det,std::min({a.x,v.x,w.x})-.001f,std::max({a.x,v.x,w.x})+.001f,std::min({a.z,v.z,w.z})-.001f,std::max({a.z,v.z,w.z})+.001f});
     }
   }
 }
 bool project(V3& p)const{
   for(const auto& f:faces){
     if(p.x<f.minX||p.x>f.maxX||p.z<f.minZ||p.z>f.maxZ)continue;
     V3 q=p-f.a;float u=(q.x*f.f.z-q.z*f.f.x)*f.invDet,v=(f.e.x*q.z-f.e.z*q.x)*f.invDet;
     if(u>=-.0001f&&v>=-.0001f&&u+v<=1.0001f){p.y=f.a.y+f.e.y*u+f.f.y*v;return true;}
   }
   return false;
 }
};
struct Rider {
 V3 p{},velocity{};float s=15,d=0,heading=0,yawRate=0,lean=0,slip=0;
 float lateralSpeed=0,drift=0,boardAngle=0,pace=28,driftCharge=0,boost=0;
 float edgeImpact=0,cleanSlideTime=0,runTime=0,crouch=0,windPhase=0;
 float slideAngle=0,slideAngularSpeed=0,carveCommit=0,footBrake=0,overRotation=0,wheelTravel=0;
 bool spinOut=false;
 float crashTime=0;int crashSide=1,crashes=0;bool crashed=false;
 int driftDirection=1;bool finished=false;
 int pose=0,previousPose=0;float poseBlend=1,poseLean=0,turnIntentTime=0;
 int pilot=0;
 static constexpr float startSpeed=28.f;
 void reset(const Road& road){*this=Rider{};auto r=road.at(s);p=r.p;heading=r.heading;velocity={std::sin(heading)*startSpeed,0,std::cos(heading)*startSpeed};}
 float speed()const{return length(velocity);}
 void animatePose(float dt){
   if(dt<=0)return;
   poseBlend=std::min(1.f,poseBlend+dt*(drift>.48f?24.f:16.f));
   poseLean=approach(poseLean,lean,9,dt);
   int oldLevel=poseLevel(pose),oldSide=poseDirection(pose);
   float amount=std::abs(poseLean);
   // Visual dead zone and hysteresis let small corrections stay relaxed.
   int level=amount>(oldLevel>=3?.72f:.80f)?3:amount>(oldLevel>=2?.43f:.50f)?2:amount>(oldLevel>=1?.19f:.27f)?1:0;
   int side=poseLean<0?-1:1;
   if(oldLevel==0&&level&&drift<=.48f){turnIntentTime+=dt;if(turnIntentTime<.06f)level=0;}
   else turnIntentTime=0;
   if(drift>.48f){level=4;side=driftDirection;}
   if(slideAngle>1.35f&&drift>.48f){level=5;side=driftDirection;}
   int desired=footBrake>.2f&&drift<.2f?6:level?turningPose(side,level):crouch>.65f?5:0;
   // Step through distinct painted cels, never mirror a rider's stance or
   // squash the body to fake intermediate steering angles.
   if(oldLevel&&oldSide!=side)desired=oldLevel>1?turningPose(oldSide,oldLevel-1):0;
   else if(level>oldLevel+1)desired=turningPose(side,oldLevel+1);
   else if(oldLevel>level+1)desired=turningPose(oldSide,oldLevel-1);
   if(desired!=pose&&poseBlend>=.75f){previousPose=pose;pose=desired;poseBlend=0;}
 }
 void tick(const Road& road,Input in,float wet,float dt){
   if(finished||dt<=0)return;
   in.steer=clamp(in.steer,-1,1);in.brake=clamp(in.brake,0,1);wet=clamp(wet,0,1);
   float previousS=s,previousD=d;auto r=road.at(s);float curvature=wrap(road.at(s+2).heading-r.heading)/2;
   crouch=approach(crouch,in.tuck&&in.brake<.05f?1.f:0.f,9,dt);runTime+=dt;edgeImpact=std::max(0.f,edgeImpact-dt*4);
   if(crashed){
     crashTime+=dt;pace*=std::exp(-dt*3.7f);
     s=std::min(road.size()-4,s+pace*dt);d=approach(d,crashSide*3.45f,3,dt);
     p=road.surface(s,d);auto rr=road.at(s);velocity=V3{std::sin(rr.heading),0,std::cos(rr.heading)}*pace;
     if(crashTime>=1.75f){
       float resume=s,time=runTime;int count=crashes,selected=pilot;
       *this=Rider{};pilot=selected;s=resume;runTime=time;crashes=count;pace=18;
       p=road.surface(s,0);heading=road.at(s).heading;
       velocity=V3{std::sin(heading),0,std::cos(heading)}*pace;
     }
     return;
   }
   windPhase+=dt*windRate(pace);
   float steering=in.steer*(.65f+.35f*std::abs(in.steer));
   lean=approach(lean,steering,26,dt);
   // Sustained weight transfer can break the rear wheels loose without a
   // dedicated button. A short steering tap remains a carve.
   carveCommit=clamp(carveCommit+dt*(std::abs(in.steer)>.86f&&pace>13?1.f:-3.f),0,.5f);
   bool requestSlide=(in.slide||carveCommit>.24f)&&pace>9&&(std::abs(in.steer)>.12f||drift>.2f);
   bool sliding=requestSlide||slideAngle>.20f;
   // Commit to a slide side; countersteering corrects its angle before reversing it.
   if(requestSlide&&slideAngle<.12f)driftDirection=in.steer>0?1:-1;
   const auto& traits=pilots[std::clamp(pilot,0,3)];
   float balance=(curvature*pace*pace*(.25f-drift*.105f)*(1+wet*.35f)-road.bankAssist(s))/((7.2f-drift*.5f)*traits.steering);
   float excess=(steering-clamp(balance,-1,1))*driftDirection;
   float rotation=requestSlide?( .62f+in.brake*.32f-slideAngle)*2.8f+excess*(4.5f+wet): -slideAngle*12.f;
   slideAngularSpeed=approach(slideAngularSpeed,rotation,10.f,dt);
   slideAngle=clamp(slideAngle+slideAngularSpeed*dt,0,2.6f);
   if(slideAngle==0)slideAngularSpeed=0;
   float oldDrift=drift;
   drift=approach(drift,clamp((slideAngle-.10f)/.45f,0,1),20,dt);
   overRotation=clamp((slideAngle-1.66f)/.45f,0,1);
   footBrake=approach(footBrake,!sliding?in.brake:0.f,12,dt);
   if(!requestSlide&&oldDrift>.2f&&drift<=.2f&&cleanSlideTime>.45f&&driftCharge>.28f&&edgeImpact==0){boost=std::min(1.f,driftCharge);driftCharge=0;cleanSlideTime=0;}
   if(sliding&&slideAngle>.35f&&slideAngle<1.02f&&edgeImpact==0){cleanSlideTime+=dt;driftCharge=std::min(1.f,driftCharge+dt*.7f);}else if(overRotation>.1f){driftCharge=0;cleanSlideTime=0;}else if(drift<.1f&&!requestSlide){driftCharge=0;cleanSlideTime=0;}
   boost=std::max(0.f,boost-dt*1.2f);
   float grade=-(road.at(s+1).p.y-r.p.y);
   // Gravity, tuck and aerodynamic drag determine speed; no target-speed servo.
   float gravityGrade=road.course>=6?grade/std::sqrt(1+grade*grade):grade;
   // Tucking only reduces aerodynamic drag: it is not a throttle. Braking
   // comes from the sole on the road or the wheels scrubbing sideways.
   float acceleration=gravityGrade*(road.course>=6?24.f:38.f)-.35f-pace*pace*mix(.0032f,.00065f,crouch)*traits.drag;
   // Arcade rolling assistance restores momentum on gentle sections, but
   // never fights a deliberate brake/slide and fades out above 86 km/h.
   if(in.brake<.05f&&slideAngle<.20f&&!requestSlide)
     acceleration+=4.5f*clamp((24.f-pace)/12.f,0,1)*clamp((.08f-grade)/.08f,0,1);
   if(in.tuck&&in.brake<.05f&&pace<8&&slideAngle<.2f&&std::fmod(runTime,.75f)<.22f)acceleration+=35.f;
   float soleDrag=footBrake*8.5f*traits.brake*(1-wet*.25f);
   float transverse=clamp((slideAngle-1.05f)/.50f,0,1);
   float scrub=drift*(1.5f+5.f*std::pow(std::sin(slideAngle),2)+8.f*transverse*transverse)*(1+in.brake*2)*(1-wet*.22f);
   pace=clamp(pace+(acceleration-soleDrag-scrub)*dt,0,58);
   // Outside pressure makes fast hairpins demand braking or a committed slide.
   // Countersteer corrects the trajectory while retaining the hand-down side.
   float outside=(.25f-drift*.105f)*(1+wet*.35f);
   float control=(7.2f-drift*.5f)*traits.steering*(1-footBrake*.12f)*(1-overRotation*.5f)*(1-transverse*.75f);
   float targetLateral=lean*control-curvature*pace*pace*outside+road.bankAssist(s);
   lateralSpeed=approach(lateralSpeed,targetLateral,sliding?10.f:20.f,dt);
   lateralSpeed=clamp(lateralSpeed,-pace*.65f,pace*.65f);
   float forwardSpeed=std::sqrt(std::max(0.f,pace*pace-lateralSpeed*lateralSpeed));
   V3 previousPosition=road.surface(s,d);
   float advance=forwardSpeed*dt/std::max(.65f,1-curvature*d),sideAdvance=lateralSpeed*dt;
   if(road.course>=6){
     // On a steep, banked deck the route's horizontal coordinate is not arc
     // length. Preserve speed in world metres rather than accelerating twice.
     float travelled=length(road.surface(s+advance,d+sideAdvance)-previousPosition);
     float metric=pace*dt/std::max(travelled,.000001f);
     advance*=metric;sideAdvance*=metric;
   }
   s=std::min(road.size()-3,s+advance);d+=sideAdvance;
   float edge=Road::halfWidth-.35f;
   // Crossing 90 degrees is controllable braking. Only a further rotation
   // loses balance; speed and outward angular momentum narrow the margin.
   float fallAngle=2.20f-clamp((pace-12.f)/35.f,0,1)*.22f-clamp(std::max(0.f,slideAngularSpeed)/5.f,0,1)*.12f;
   bool lostBalance=slideAngle>fallAngle&&pace>7;
   if(std::abs(d)>edge||lostBalance||(road.course==7&&hitsRoadHazard(previousS,previousD,s,d,runTime))){
     spinOut=lostBalance;carveCommit=0;footBrake=0;
     d=clamp(d,-edge,edge);crashed=true;crashTime=0;crashSide=spinOut?driftDirection:(d>0?1:-1);crashes++;
     lateralSpeed=0;pace*=.65f;boost=0;drift=0;driftCharge=0;cleanSlideTime=0;edgeImpact=1;
     p=road.surface(s,d);velocity=norm(velocity)*pace;return;
   }
   r=road.at(s);float travelAngle=std::atan2(lateralSpeed,std::max(forwardSpeed,.1f));
   float wantedAngle=travelAngle+lean*.12f*(1-drift)+slideAngle*driftDirection;
   float oldHeading=heading;boardAngle=wantedAngle;
   heading=wrap(r.heading+boardAngle);yawRate=wrap(heading-oldHeading)/dt;slip=wrap(boardAngle-travelAngle);
   wheelTravel+=pace*dt;animatePose(dt);
   V3 forward{std::sin(r.heading),0,std::cos(r.heading)},right{forward.z,0,-forward.x};
   velocity=forward*forwardSpeed+right*lateralSpeed;p=road.surface(s,d);
   if(road.course>=6)velocity=norm(p-previousPosition)*pace;
   if(s>=Road::finishDistance){finished=true;}
 }
};
// A preview driver must brake before an apex, just like the player. The
// speed limit comes from the same steering/outward-pressure budget as tick().
inline float cornerSpeed(const Road& road,float s,float wet,bool slide=true,int pilot=0){
 if(road.course>=5){
   float limit=58,outside=(slide?.145f:.25f)*(1+wet*.35f),control=(slide?6.7f:7.2f)*pilots[std::clamp(pilot,0,3)].steering;
   for(float ahead=4;ahead<=(road.course==6?76:50);ahead+=4){float k=wrap(road.at(s+ahead+2).heading-road.at(s+ahead).heading)/2;
     float help=road.bankAssist(s+ahead)*(k<0?-1.f:1.f);
     limit=std::min(limit,std::sqrt(std::max(.1f,control+help)/(std::max(.001f,std::abs(k))*outside))*.87f);
   }return limit;
 }
 float peak=.001f;
 for(float ahead=4;ahead<=50;ahead+=4)
   peak=std::max(peak,std::abs(wrap(road.at(s+ahead+2).heading-road.at(s+ahead).heading)/2));
 float outside=(slide?.145f:.25f)*(1+wet*.35f),control=(slide?6.7f:7.2f)*pilots[std::clamp(pilot,0,3)].steering;
 return std::min(58.f,std::sqrt(control/(peak*outside))*.87f);
}
inline Input previewInput(const Road& road,const Rider& r,float wet,float lane,int level,int personality){
 const auto& traits=pilots[std::clamp(r.pilot,0,3)];
 float k=wrap(road.at(r.s+2).heading-road.at(r.s).heading)/2;
 float preview=wrap(road.at(r.s+10).heading-road.at(r.s+8).heading)/2;
 float margin=.85f+level*.055f+personality*.009f;
 float ceiling=48+level*5.f;
 // Brake to the speed needed AT each apex, not to the slowest speed anywhere
 // in a fixed preview window. Distance and available deceleration matter.
 float distance=0;V3 previous=road.surface(r.s,lane);
 for(float ahead=0;ahead<=140;ahead+=4){
   float at=r.s+ahead;V3 p=road.surface(at,lane);
   distance+=length(p-previous);previous=p;
   float turn=wrap(road.at(at+2).heading-road.at(at).heading)/2;
   if(std::abs(turn)<.0002f)continue;
   float assist=road.bankAssist(at)*(turn<0?-1.f:1.f);
   float apex=std::sqrt(std::max(.2f,6.7f*traits.steering+assist)/(std::abs(turn)*.145f*(1+wet*.35f)))*margin;
   float deceleration=(10.f+level*1.6f)*traits.brake;
   ceiling=std::min(ceiling,std::sqrt(apex*apex+2*deceleration*std::max(0.f,distance-r.pace*.15f)));
 }
 float demand=std::max(std::abs(k*r.pace*r.pace*.25f*(1+wet*.35f)-road.bankAssist(r.s)),
   std::abs(preview*r.pace*r.pace*.25f*(1+wet*.35f)-road.bankAssist(r.s+8)));
 bool slide=demand>(r.drift>.25f?5.3f:6.5f)*traits.steering;
 float brake=clamp((r.pace-ceiling)*.65f,0,1);
 float outside=(.25f-r.drift*.105f)*(1+wet*.35f);
 float desiredLane=road.course==7?mix(lane,hazardLane(r.s),hazardInfluence(r.s)):lane;
 float target=clamp((k*r.pace*r.pace*outside-road.bankAssist(r.s)-(r.d-desiredLane)*1.8f-r.lateralSpeed*.32f)/((7.2f-r.drift*.5f)*traits.steering),-1,1);
 if(slide&&r.drift<.2f&&std::abs(target)<.30f)target=preview<0?-.30f:.30f;
 // Recover yaw before the slide reaches the spin-out threshold. This uses
 // ordinary steering, so recovery trades some apex precision for stability.
 if(r.slideAngle>1.f)target=clamp(target-r.driftDirection*clamp((r.slideAngle-1.f)*.85f+std::max(0.f,r.slideAngularSpeed)*.08f,0,.5f),-1,1);
 float axis=(std::sqrt(.65f*.65f+1.4f*std::abs(target))-.65f)/.7f;
 return {std::copysign(axis,target),brake,brake<.03f,slide};
}
inline Input demoInput(const Road&road,const Rider&r,float wet){return previewInput(road,r,wet,0,1,0);}
inline Rider interpolate(const Rider&a,const Rider&b,float t){if(a.crashed!=b.crashed)return b;Rider r=b;r.p=mix(a.p,b.p,t);r.s=mix(a.s,b.s,t);r.d=mix(a.d,b.d,t);r.heading=a.heading+wrap(b.heading-a.heading)*t;r.lean=mix(a.lean,b.lean,t);r.slip=mix(a.slip,b.slip,t);r.drift=mix(a.drift,b.drift,t);r.boardAngle=mix(a.boardAngle,b.boardAngle,t);r.crouch=mix(a.crouch,b.crouch,t);r.slideAngle=mix(a.slideAngle,b.slideAngle,t);r.footBrake=mix(a.footBrake,b.footBrake,t);r.wheelTravel=mix(a.wheelTravel,b.wheelTravel,t);r.windPhase=mix(a.windPhase,b.windPhase,t);return r;}
}

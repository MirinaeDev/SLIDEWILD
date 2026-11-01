#include "simulation.hpp"
#include "effects.hpp"
#include <iostream>
#include <stdexcept>
using namespace dh;
void check(bool v,const char*m){if(!v)throw std::runtime_error(m);}
void ticks(Rider&r,const Road&road,Input in,int n,float wet=0){for(int i=0;i<n;i++)r.tick(road,in,wet,1.f/120);}
int main(){try{
 Road level;
 for(size_t j=0;j<level.points.size();j++)level.points[j]={{0,0,float(j)*2},0,0};
 Rider tucked;tucked.reset(level);ticks(tucked,level,{0,0,true,false},120);
 check(tucked.pace<Rider::startSpeed,"Tuck must not propel the board on level ground");
 Rider rolling;rolling.reset(level);rolling.pace=12;ticks(rolling,level,{},600);
 check(rolling.pace>19&&rolling.pace<24,"Flat sections need bounded automatic momentum recovery");
 Rider lowBrake;lowBrake.reset(level);lowBrake.pace=12;ticks(lowBrake,level,{0,1,false,false},120);
 check(lowBrake.pace<6,"Rolling assistance must not cancel a deliberate brake");
 Rider sole,coast;sole.reset(level);coast=sole;
 ticks(sole,level,{0,1,false,false},120);ticks(coast,level,{},120);
 check(sole.pace<coast.pace-5&&sole.pose==6,"Footbrake needs real speed loss and its own pose");
 for(int side:{-1,1}){
   Rider gentle;gentle.reset(level);ticks(gentle,level,{side*.35f,0,true,false},60);
   check(gentle.slideAngle==0&&!gentle.crashed,"Gentle carving must retain grip");
   Rider held;held.reset(level);ticks(held,level,{float(side),0,true,false},120);
   check(held.crashed&&held.slideAngle>pi*.5f,"Held strong steering must pass transverse braking before losing the line");
   Rider spin;spin.reset(level);ticks(spin,level,{float(side),0,true,true},120);
   check(spin.spinOut&&std::abs(spin.d)<Road::halfWidth-.5f,"Excess slide rotation must cause a fall away from walls");
   Rider cross;cross.reset(level);cross.slideAngle=pi*.5f;cross.drift=1;cross.driftDirection=side;
   float target=(cross.slideAngle-.62f)*2.8f/4.5f;
   float hold=(std::sqrt(.65f*.65f+1.4f*target)-.65f)/.7f;
   ticks(cross,level,{side*hold,0,false,true},60);
   check(!cross.crashed&&std::abs(cross.slideAngle-pi*.5f)<.05f&&cross.pace<21&&poseLevel(cross.pose)==5,"90 degree slide must be sustainable, strongly brake and use its own pose");
   Rider balanced;balanced.reset(level);ticks(balanced,level,{side*.2f,0,true,true},120);
   check(!balanced.crashed&&balanced.slideAngle>.5f&&balanced.slideAngle<1.1f,"A balanced slide must remain controllable");
   Rider recover;recover.reset(level);
   while(recover.slideAngle<1.75f&&!recover.crashed)recover.tick(level,{float(side),0,true,true},0,1.f/120);
   Rider late=recover;
   ticks(recover,level,{-side*.6f,0,true,true},48);
   ticks(late,level,{float(side),0,true,true},48);
   std::cout<<"Recovery "<<side<<" angle "<<recover.slideAngle<<" crash "<<recover.crashed<<" late "<<late.slideAngle<<" spin "<<late.spinOut<<'\n';
   check(!recover.crashed&&recover.slideAngle<1.1f&&late.spinOut,"Countersteer must save a slide that continued steering loses");
   Rider poses;
   for(int degree=1;degree<=3;degree++){
     poses.lean=side*(degree==1?.35f:degree==2?.65f:.95f);
     for(int j=0;j<60;j++)poses.animatePose(1.f/120);
     check(poseDirection(poses.pose)==side&&poseLevel(poses.pose)==degree,"Three distinct turning levels on each side");
   }
 }
 std::cout<<"PASS longboard balance: tuck, sole brake, three turn stages, automatic slide, bilateral spin-out and countersteer recovery\n";
 Road park(5),flatPark=park;float largestBank=0,minGrade=1,maxGrade=-1;
 for(auto&p:flatPark.points)p.bank=0;
 for(float s=5;s<3900;s+=1){
   auto a=park.at(s),b=park.at(s+1);float curvature=wrap(b.heading-a.heading);
   largestBank=std::max(largestBank,std::abs(a.bank));
   float grade=a.p.y-b.p.y;minGrade=std::min(minGrade,grade);maxGrade=std::max(maxGrade,grade);
   check(curvature*a.bank<=.0001f,"Bank rises on the inside of a corner");
   check(length(park.surface(s+1,4.5f)-park.surface(s,4.5f))<1.7f,"Abrupt bank edge transition");
 }
 check(largestBank>.7f&&minGrade<-.02f&&maxGrade>.16f,"Gravity Park lacks steep banks and compression relief");
 Rider assisted,unassisted;assisted.reset(park);assisted.s=270;assisted.p=park.surface(270,0);assisted.heading=park.at(270).heading;assisted.pace=35;assisted.velocity={0,0,35};unassisted=assisted;
 ticks(assisted,park,{},12);ticks(unassisted,flatPark,{},12);
 check(std::abs(assisted.d)<std::abs(unassisted.d),"Bank does not help resist outward pressure");
 check(cornerSpeed(park,250,0)>cornerSpeed(flatPark,250,0),"Corner speed ignores useful banking");
 std::cout<<"PASS Gravity Park: outward bank, smooth edges, compressions and effective lateral support\n";
 Road future(6);float futureBank=0,futureGrade=0,minFutureGrade=1;
 for(float s=2;s<3890;s+=1){
   auto a=future.at(s),b=future.at(s+1);float k=wrap(b.heading-a.heading),grade=a.p.y-b.p.y;
   futureBank=std::max(futureBank,std::abs(a.bank));futureGrade=std::max(futureGrade,grade);minFutureGrade=std::min(minFutureGrade,grade);
   check(k*a.bank<=.0001f,"Future banking faces the wrong side");
   check(length(future.surface(s+1,4.5f)-future.surface(s,4.5f))<1.8f,"Abrupt future bank or plunge");
 }
 check(futureBank>1.1f&&futureGrade>.68f&&minFutureGrade<-.14f,"Future circuit lacks extreme banks, plunges or compressions");
 Rider plunging;plunging.reset(future);plunging.s=2030;plunging.p=future.surface(plunging.s,0);plunging.pace=45;plunging.velocity={0,0,45};
 for(int j=0;j<70;j++){
   V3 before=plunging.p;plunging.tick(future,demoInput(future,plunging,0),0,1.f/120);
   check(std::abs(length(plunging.p-before)*120-plunging.pace)<.15f,"Slope adds uncommanded world-space speed");
 }
 Rider descentBrake=plunging,descentCoast=plunging;
 ticks(descentBrake,future,{0,1,false,false},30);
 ticks(descentCoast,future,{0,0,false,false},30);
 check(descentBrake.pace<descentCoast.pace-.5f,"Footbrake must resist the plunge relative to coasting");
 std::cout<<"PASS Neon Descent: steep deck metric, braking, outward banks and smooth compression transitions\n";
 Road road;
 for(const auto& h:caldeiraHazards){
   float live=h.kind==HazardKind::Basalt?0:4-h.phase+8;
   check(hitsRoadHazard(h.s-7,h.d,h.s+7,h.d,live),"Fast rider tunnels through a road hazard");
   check(!hitsRoadHazard(h.s-7,-h.d,h.s+7,-h.d,live),"Hazard blocks the advertised escape lane");
   if(h.kind==HazardKind::LavaVent){
     check(!hazardActive(h,2.5f-h.phase+8),"Lava hits during its warning phase");
     check(!hazardActive(h,6-h.phase+8),"Lava hits after its eruption");
     check(lavaBurst(live,h.phase)>.9f,"Active collision is not visibly erupting");
   }
   check(std::abs(hazardLane(h.s)+std::copysign(1.75f,h.d))<.01f,"Avoidance points into the obstacle");
 }
 {
   Road caldeira(7);Rider hit;hit.reset(caldeira);const auto& h=caldeiraHazards[0];
   hit.s=h.s-4;hit.d=h.d;hit.p=caldeira.surface(hit.s,hit.d);hit.pace=58;
   for(int j=0;j<30&&!hit.crashed;j++)hit.tick(caldeira,{0,0,true,false},0,1.f/120);
   check(hit.crashed&&hit.crashes==1,"Hazards are visible but never trigger a fall");
 }
 std::cout<<"PASS volcanic hazards: swept collision, advance warning, active phase, free escape lane and real rider impact\n";
 for(float s=0;s<road.size()-2;s+=.7f){check(length(road.surface(s+.1f,0)-road.surface(s,0))<.11f,"Road continuity");check(std::abs(road.project(road.surface(s,0),s)-s)<.01f,"Projection");}
 Rider straight,braked;straight.reset(road);braked=straight;ticks(straight,road,{},240);ticks(braked,road,{0,1,false,false},240);
 check(straight.speed()>23&&straight.speed()<36,"Controlled cruise");check(braked.speed()<straight.speed()-12,"Strong brake");
 Rider response;response.reset(road);ticks(response,road,{1,0,false,false},12);
 check(response.lean>.9f&&response.lateralSpeed>2.7f,"Steering response under 100ms");
 Rider carve,slide;carve.reset(road);slide=carve;ticks(carve,road,{.5f,0,true,false},42);ticks(slide,road,{.5f,0,true,true},42);
 check(slide.slip>.5f,"Progressive slide reaches a visible angle within 350ms");check(slide.speed()>carve.speed()*.86f,"Drift scrubs speed without stopping the board");
 check(poseDirection(carve.pose)==1&&poseLevel(carve.pose)<4&&slide.pose==4,"Carving and hand-down slide use distinct poses");
 float entrySlip=slide.slip;ticks(slide,road,{-.6f,0,true,true},20);check(slide.slip>0&&slide.slip<entrySlip,"Countersteer reduces slip without reversing committed side");
 check(slide.pose==4,"Countersteer retains hand-down side");
 ticks(slide,road,{},36);check(std::abs(slide.slip)<.12f&&slide.drift<.03f,"Recovery within 300ms");
 Rider boost;boost.reset(road);ticks(boost,road,{.2f,0,true,true},96);float rewardPeak=0;
 for(int j=0;j<60;j++){boost.tick(road,{},0,1.f/120);rewardPeak=std::max(rewardPeak,boost.boost);if(boost.boost>0)check(boost.slideAngle<.3f,"Reward before the board is aligned");}
 check(rewardPeak>.2f,"Clean slide is rewarded only after realignment");
 Rider wall;wall.reset(road);wall.d=Road::halfWidth-.36f;ticks(wall,road,{1,0,true,true},100);wall.tick(road,{},0,1.f/120);check(wall.boost==0&&wall.driftCharge==0,"No reward for wall slide");
 check(wall.crashed&&wall.crashes==1,"Edge contact starts one crash rather than wall riding");
 check(wall.drift==0&&wall.pace<3,"Crash cancels slide and decelerates independently of input");
 Rider recovery=wall;ticks(recovery,road,{},240);
 check(!recovery.crashed&&recovery.crashes==1&&std::abs(recovery.d)<1&&recovery.speed()>15,"Automatic grounded recovery with reduced speed");
 Road shallow=road,steep=road;
 for(size_t i=0;i<road.points.size();i++){shallow.points[i].p.y=-float(i)*Road::step*.02f;steep.points[i].p.y=-float(i)*Road::step*.16f;}
 Rider slowSlope,fastSlope;slowSlope.reset(shallow);fastSlope.reset(steep);
 ticks(slowSlope,shallow,{},180);ticks(fastSlope,steep,{},180);
 check(fastSlope.speed()>slowSlope.speed()+3,"Grade changes pace without steering or tuck");
 Rider tuck,cruise;tuck.reset(road);cruise=tuck;ticks(tuck,road,{0,0,true,false},360);ticks(cruise,road,{},360);check(tuck.speed()>cruise.speed()+3,"Tuck gives speed control");
 Rider dry,wet;dry.reset(road);wet=dry;dry.s=wet.s=480;dry.p=wet.p=road.surface(480,0);
 ticks(dry,road,{},20);ticks(wet,road,{},20,1);check(std::abs(wet.d)>std::abs(dry.d),"Wet corner drift");
 Rider a,b;a.reset(road);b=a;for(int f=0;f<120;f++)ticks(a,road,{},4);ticks(b,road,{},480);check(length(a.p-b.p)<1e-5,"Fixed step determinism");
 Rider stress;stress.reset(road);for(int i=0;i<24000&&!stress.finished;i++){stress.tick(road,{std::sin(i*.004f),i%900<100?1.f:0.f,true,i%600<130},.7f,1.f/120);check(std::isfinite(stress.speed()),"Finite");check(std::abs(stress.d)<=Road::halfWidth,"Road bounds");check(std::abs(stress.p.y-road.surface(stress.s,stress.d).y)<.001f,"Grounded");}
 // Holding steering into a barrier may stop the rider; the recovery must work.
 ticks(wall,road,{-1,0,true,false},60);check(wall.d<Road::halfWidth-1&&wall.speed()>0,"Recover from guardrail");
 Rider tour;tour.reset(road);for(int i=0;i<18000&&!tour.finished;i++){
   tour.tick(road,demoInput(road,tour,0),0,1.f/120);
 }check(tour.finished&&tour.crashes==0,"Full course including hairpins without demo crashes");
 Rider wetTour;wetTour.reset(road);float topSpeed=0;
 for(int i=0;i<22000&&!wetTour.finished;i++){
   wetTour.tick(road,demoInput(road,wetTour,1),1,1.f/120);topSpeed=std::max(topSpeed,wetTour.speed());
 }
 check(wetTour.finished&&wetTour.crashes==0,"Wet course can be completed with anticipatory braking");
  check(topSpeed>43,"Wet long straights retain speeds above 154 km/h with anticipatory braking");
 for(float s=0;s<1400;s+=13)for(float d:{-12.f,-8.f,6.5f,12.f,19.f})check(std::abs(terrainPoint(road,s,d).y-road.surface(s,d).y-terrainElevation(s,d))<.0001f,"Terrain anchor");
 for(auto range: {std::pair<float,float>{440,530},{690,790},{1000,1090}}){
   float angle=std::abs(road.at(range.second).heading-road.at(range.first).heading);
   check(angle>2.9f&&angle<3.25f,"Hairpins turn approximately 175 degrees");
 }
 Rider hairpin,noSlide;hairpin.reset(road);hairpin.s=425;hairpin.p=road.surface(425,0);hairpin.pace=36;noSlide=hairpin;
 float maxSlide=0,maxCarve=0;int slideWalls=0;
 for(int i=0;i<500;i++){
    Input a=demoInput(road,hairpin,0),b=demoInput(road,noSlide,0);b.slide=false;b.steer=clamp(b.steer,-.85f,.85f);
   hairpin.tick(road,a,0,1.f/120);noSlide.tick(road,b,0,1.f/120);
   maxSlide=std::max(maxSlide,std::abs(hairpin.d));maxCarve=std::max(maxCarve,std::abs(noSlide.d));
   if(hairpin.edgeImpact>.99f)slideWalls++;
 }
 std::cout<<"Hairpin comparison: slide deviation "<<maxSlide<<", carve "<<maxCarve<<", slide walls "<<slideWalls<<"\n";
 check(slideWalls==0&&maxSlide<3.5f,"Slide crosses hairpin without riding barrier");
 check(maxCarve>maxSlide+1,"Slide materially improves tight corner trajectory");
 // Identical entry and steering policy: acceleration without braking must
 // overshoot, whereas the preview driver can slow down and complete the turn.
 Rider controlled,reckless;controlled.reset(road);controlled.s=410;controlled.p=road.surface(410,0);controlled.pace=50;reckless=controlled;
 bool didBrake=false;
 for(int i=0;i<900&&controlled.s<550;i++){
   Input safe=demoInput(road,controlled,0),fast=demoInput(road,reckless,0);
   didBrake|=safe.brake>.1f;fast.brake=0;fast.tuck=true;
   controlled.tick(road,safe,0,1.f/120);reckless.tick(road,fast,0,1.f/120);
 }
 std::cout<<"Braking challenge: safe crashes "<<controlled.crashes<<", full throttle crashes "<<reckless.crashes<<"\n";
 check(didBrake&&controlled.crashes==0&&controlled.s>=550,"Brake before the apex to survive a fast entry");
 check(reckless.crashes>0,"Excessive speed overruns steering even with drift");
 for(float s=0;s<1500;s+=.5f)for(int side:{-1,1})for(float radius:{.5f,1.05f,1.2f}){
   float offset=clearPlantOffset(s,side,radius,side*5.3f);
   float outer=side>0?5.15f:(stoneWallAt(s)?5.98f:(hayBalesAt(s)?5.36f:5.05f));
   check(std::abs(offset)-radius>outer+.2f,"Shrub bounds clear barrier including billboard width");
 }
 check(cliffWeight(480)>.99f&&cliffWeight(730)==0&&cliffWeight(1200)==0,"Cliff and open meadow sectors");
 check(std::abs(clearPlantOffset(65,-1,2,-6))-2>6.18f,"Plant footprint checks wall end caps");
 for(float at:{50.f,448.f,490.f,710.f,1040.f}){
   auto loc=road.locate(road.surface(at,0));
   check(std::abs(loc.s-at)<.02f&&loc.distance<.01f,"Global road lookup across hairpins");
 }
 RideEffects effects;Rider effectRider;effectRider.reset(road);
 for(int i=0;i<90;i++){effectRider.tick(road,{.25f,0,true,true},0,1.f/120);effects.tick(effectRider,road,0,1.f/120);}
 check(!effects.puffs.empty()&&!effects.tracks.empty(),"Slide produces smoke and persistent tracks");
 V3 original=effects.tracks.front().a;
 effectRider.tick(road,{},0,1.f/120);effects.tick(effectRider,road,0,1.f/120);
 check(length(effects.tracks.front().a-original)<1e-6,"Tracks stay fixed in world space");
 effectRider.drift=0;
 for(int i=0;i<1000;i++)effects.tick(effectRider,road,0,1.f/120);
 check(effects.puffs.empty()&&effects.tracks.empty(),"Effects expire");
 for(int i=0;i<120;i++)effects.tick(effectRider,road,1,1.f/120);
 check(!effects.puffs.empty()&&effects.puffs.front().spray&&effects.tracks.empty(),"Wet spray without smoke or skidmarks");
 check(!effects.rain.empty()&&!effects.splashes.empty(),"World rain collides and emits splashes");
 for(const auto&drop:effects.rain)check(drop.p.y>drop.floor,"Rain stays above impact surface");
 RideEffects single;float rs=500;V3 impact=road.surface(rs,1);
 single.rain.push_back({impact+V3{0,.05f,0},impact.y,rs});
 Rider observer=effectRider;observer.s=rs;observer.p=road.surface(rs,0);observer.velocity={};
 single.tick(observer,road,0,.01f);
 check(single.rain.empty()&&single.splashes.size()==1,"One collision makes exactly one splash");
 check(length(single.splashes.front().p-(impact+V3{0,.012f,0}))<.001f,"Splash at actual collision location");
 for(int i=0;i<40;i++)single.tick(observer,road,0,.01f);
 check(single.splashes.empty(),"Splash expires after impact");
 for(float s=0;s<1400;s+=7)for(float d:{-20.f,-8.f,0.f,8.f,19.f}){
   V3 p=terrainPoint(road,s,d);V3 q=groundPoint(road,p,s);
   check(std::abs(q.y-p.y)<.04f,"Ground queries follow rolling terrain");
 }
 for(float at:{448.f,482.f,520.f,700.f,740.f}){
   V3 a=road.surface(at,-Road::halfWidth),b=road.surface(at+2,-Road::halfWidth),c=road.surface(at+2,Road::halfWidth);
   V3 sample=a*.2f+b*.35f+c*.45f;
   check(std::abs(roadGroundPoint(road,sample+V3{0,5,0},at).y-sample.y)<.0001f,"Shadow ground matches rendered triangle, including bank");
   RoadSurfaceQuery cache(road,at);V3 fast=sample+V3{0,5,0};
   check(cache.project(fast)&&std::abs(fast.y-sample.y)<.0001f,"Cached surface query preserves exact triangle height");
 }
 RideEffects anchored;anchored.rain.push_back({{10,50,20},-100,500});
 observer.s=500;observer.p={80,0,70};anchored.tick(observer,road,0,1.f/120);
 check(anchored.rain.front().p.x==10&&anchored.rain.front().p.z==20,"Rain does not translate with player");
 effects.reset();check(effects.puffs.empty()&&effects.rain.empty()&&effects.splashes.empty()&&effects.debris.empty()&&!effects.contact,"Reset clears effects");
 ImmersionState immersion;Rider actor;actor.reset(road);
 actor.drift=.4f;immersion.tick(actor,0,1.f/120);check(immersion.slideEntry==1,"One impulse at slide engagement");
 for(int i=0;i<120;i++)immersion.tick(actor,0,1.f/120);
 check(immersion.slideEntry<.001f,"Held drift does not repeatedly retrigger entry impulse");
 actor.drift=.9f;immersion.tick(actor,0,.01f);actor.drift=.6f;immersion.tick(actor,0,.01f);
 check(immersion.gripReturn==1,"Grip recovery impulse on slide release");
 float frozen=immersion.gripReturn;immersion.tick(actor,1,0);check(immersion.gripReturn==frozen,"Pause freezes immersion envelopes");
 float poolS=0,dryS=0,roughS=0;
 for(float at=20;at<400;at+=.1f){
   check(waterMask(at,0,0)==0,"Dry weather cannot produce puddles");
   check(waterMask(at,4.6f,1)==0,"Puddles stay inside road margins");
   if(waterMask(at,0,1)>.9f)poolS=at;
   if(waterMask(at,0,1)==0)dryS=at;
   if(repairMask(at,0)>.95f)roughS=at;
 }
 check(poolS>0&&dryS>0&&roughS>0,"Distinct puddles and repaired surfaces exist");
 RideEffects poolEffects,filmEffects;actor.velocity={0,0,35};actor.drift=0;
 actor.s=poolS;actor.p=road.surface(poolS,0);
 for(int i=0;i<45;i++)poolEffects.tick(actor,road,1,1.f/120);
 actor.s=dryS;actor.p=road.surface(dryS,0);
 for(int i=0;i<45;i++)filmEffects.tick(actor,road,1,1.f/120);
 check(poolEffects.puffs.size()>filmEffects.puffs.size()*1.6f,"A puddle emits more wheel spray than the wet film");
 RideEffects gravel;actor.s=roughS;actor.p=road.surface(roughS,0);
 for(int i=0;i<120;i++)gravel.tick(actor,road,0,1.f/120);
 check(!gravel.debris.empty()&&gravel.debris.size()<=96,"Rough asphalt emits bounded debris");
 actor.velocity={};for(int i=0;i<240;i++)gravel.tick(actor,road,0,1.f/120);
 check(gravel.debris.empty(),"Debris expires after stopping");
 Rider animation;animation.drift=1;animation.driftDirection=-1;animation.lean=-.7f;
 animation.animatePose(1.f/120);
 check(animation.pose==1,"Drift loads the inside carving pose before the hand goes down");
 for(int i=0;i<24;i++)animation.animatePose(1.f/120);
 check(animation.pose==3,"Carve transitions to the left hand-down pose");
 float blend=animation.poseBlend;animation.animatePose(0);
 check(animation.poseBlend==blend,"Pose transition freezes in pause");
 animation.drift=0;animation.lean=0;
 for(int i=0;i<40;i++)animation.animatePose(1.f/120);
 check(animation.pose==0&&animation.poseBlend==1,"Leaving slide rises through carve and settles");
 for(int selected=0;selected<4;selected++)for(float rain:{0.f,1.f}){
   Rider trial;trial.reset(road);trial.pilot=selected;trial.pace=22;trial.velocity=norm(trial.velocity)*22;
   for(int step=0;step<18000&&!trial.finished;step++)trial.tick(road,demoInput(road,trial,rain),rain,1.f/120);
   check(trial.finished&&trial.crashes==0,"Every pilot can finish the dry and wet reference course");
   check(trial.pilot==selected,"Pilot traits persist throughout the run");
   std::cout<<"Pilot "<<pilots[selected].name<<" wet "<<rain<<" reference time "<<trial.runTime<<" s\n";
 }
 std::cout<<"PASS immersion: pause, drift events, puddle response, dry surface, bounded debris, pose staging and expiry\n";
 check(courseRain(3,0)==1&&courseRain(3,4000)==1,"Highlands remains rainy");
 check(courseRain(4,500)==0&&courseRain(4,2200)==1&&courseRain(4,3900)>.7f,"Tropical sun to storm progression");
 for(int course=1;course<courseCount;course++){
   Road route(course);
   check(length(route.at(2000).p-road.at(2000).p)>60,"New course must have its own physical layout");
   Rider reckless;reckless.reset(route);
   for(int step=0;step<12000&&reckless.crashes==0;step++){
     auto driving=demoInput(route,reckless,0);driving.brake=0;driving.tuck=true;driving.slide=false;
     driving.steer=clamp(driving.steer,-.85f,.85f); // Stay below automatic slide commitment.
     reckless.tick(route,driving,0,1.f/120);
   }
   check(reckless.crashes>0,"Even with path steering, ignoring brakes must miss a turn in each new world");
   for(int selected=0;selected<4;selected++)for(float rain:{0.f,1.f,-1.f}){
     if(rain<0&&course!=4)continue;
     Rider trial;trial.reset(route);trial.pilot=selected;trial.pace=22;trial.velocity=norm(trial.velocity)*22;
     float remaining=float(courses[course].initialTime);int gate=0;
     for(int step=0;step<22000&&!trial.finished;step++){
       float actualRain=rain<0?courseRain(course,trial.s):rain;
       trial.tick(route,demoInput(route,trial,actualRain),actualRain,1.f/120);remaining-=1.f/120;
       while(gate<3&&trial.s>=(gate+1)*1000){gate++;remaining+=courses[course].checkpointTime;}
       check(remaining>0,"New course time budget expires before a clean reference finish");
     }
     std::cout<<"Course "<<course<<" pilot "<<pilots[selected].name<<" wet "<<rain<<" time "<<trial.runTime<<" crashes "<<trial.crashes<<"\n";
     check(trial.finished&&trial.crashes==0,"Every pilot must finish each new course in dry and wet conditions");
   }
 }
 std::cout<<"PASS arcade and effects: response, speed, brake, drift, countersteer, recovery, boost, weather, fixed step, course, terrain, stationary trails, particle expiry, wet spray, reset\n";
 }catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}

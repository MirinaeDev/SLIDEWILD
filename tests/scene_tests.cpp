#include "world.hpp"
#include "ground_coverage.hpp"
#include <iostream>
#include <stdexcept>
#include <map>
#include <array>
#include "camera_effects.hpp"
#include "immersion.hpp"
#include "frontend.hpp"
#include "pause_menu.hpp"
#include "vertex_capacity.hpp"
#include "rig_data.hpp"
#include "arcade.hpp"
#include "arrival_sprite_tests.hpp"
using namespace dh;
int main(){try{
 PauseMenu pause;
 if(pause.back()!=PauseAction::Resume)throw std::runtime_error("Escape must resume, never abandon");
 pause.choose(-1);if(pause.selection!=2||pause.confirm()!=PauseAction::None||!pause.confirming||pause.accept)throw std::runtime_error("Abandon must default to cancel");
 if(pause.back()!=PauseAction::None||pause.confirming)throw std::runtime_error("Escape must cancel confirmation");
 pause.confirm();pause.choose(1);if(pause.confirm()!=PauseAction::Abandon)throw std::runtime_error("Confirmed abandon unavailable");
 pause.reset();pause.choose(1);pause.confirm();pause.choose(1);if(pause.confirm()!=PauseAction::Restart)throw std::runtime_error("Confirmed retry unavailable");
 pause.reset();pause.click(480,365,960);if(!pause.confirming||pause.selection!=2)throw std::runtime_error("Mouse pause selection mismatch");
 if(growVertexCapacity(400000,400095,sizeof(Vertex))!=800000||growVertexCapacity(800000,400095,sizeof(Vertex))!=800000)throw std::runtime_error("Vertex growth/reuse regression");
 bool overflow=false;try{growVertexCapacity(400000,size_t(UINT32_MAX)/sizeof(Vertex)+1,sizeof(Vertex));}catch(const std::runtime_error&){overflow=true;}
 if(!overflow)throw std::runtime_error("Vertex byte overflow unchecked");
 for(int choice=0;choice<4;choice++){
   Frontend result;result.page=MenuPage::Results;result.country=courseCount-1;result.choose(choice);result.confirm();result.tick(.6f);
   MenuPage expected[]={MenuPage::Route,MenuPage::Briefing,MenuPage::Globe,MenuPage::Title};
   if(result.page!=expected[choice]||result.startRequested||(choice==1&&result.country!=0))throw std::runtime_error("Results action routes incorrectly");
 }
 StageClock abandoned;if(abandoned.won||abandoned.failed)throw std::runtime_error("Empty session has a result");
 Road finishRoad;Rider finisher;finisher.reset(finishRoad);StageClock outcome;outcome.begin(0,true);finisher.finished=true;outcome.tick(finisher,.01f);if(!outcome.won||outcome.failed)throw std::runtime_error("Finish outcome missing");
 finisher.finished=false;outcome.begin(0,false);outcome.remaining=.01f;outcome.tick(finisher,.02f);if(!outcome.failed||outcome.won)throw std::runtime_error("Timeout outcome missing");
 std::cout<<"PASS session: safe pause, confirmation, retry, abandon, mouse, results navigation, win/timeout and GPU capacity overflow\n";

 Frontend wipe;wipe.confirm();wipe.confirm();wipe.choose(1);
 if(wipe.page!=MenuPage::Title||wipe.pilot!=0)throw std::runtime_error("Input skips an active transition");
 wipe.tick(.25f);
 if(wipe.page!=MenuPage::Title)throw std::runtime_error("Menu switches before cover");
 for(int width:{960,1260}){
   Mesh cover;wipe.drawTransition(cover,width);
   for(int y=0;y<540;y+=19)for(int x=0;x<width;x+=19){
     V3 p{(x+.5f)/width*2-1,1-(y+.5f)/270,0};bool covered=false;
     for(size_t i=0;i<cover.size();i+=3){
       if(cover[i].material!=-1)continue;
       V3 a=cover[i].p,e=cover[i+1].p-a,f=cover[i+2].p-a,q=p-a;
       float det=e.x*f.y-e.y*f.x;if(std::abs(det)<1e-8f)continue;
       float u=(q.x*f.y-q.y*f.x)/det,v=(e.x*q.y-e.y*q.x)/det;
       if(u>=0&&v>=0&&u+v<=1){covered=true;break;}
     }
     if(!covered)throw std::runtime_error("Transition exposes loading behind its wipe");
   }
 }
 wipe.tick(.32f);if(wipe.page!=MenuPage::Pilots||wipe.transitioning)throw std::runtime_error("Menu does not reopen after wipe");
 Frontend cancel;cancel.page=MenuPage::Route;cancel.back();for(int k=0;k<40;k++)cancel.tick(1.f/60);
 if(cancel.page!=MenuPage::Briefing||cancel.startRequested)throw std::runtime_error("Cancelling route preview starts race");
 std::cout<<"PASS transitions: input lock, full 16:9/21:9 loading cover, reopen and route cancellation\n";
 Frontend raceMenu;if(!raceMenu.raceMode)throw std::runtime_error("Rivals must be the default mode");raceMenu.page=MenuPage::Briefing;
 raceMenu.choose(1);if(raceMenu.raceMode)throw std::runtime_error("Chrono selection unavailable");raceMenu.choose(1);
 if(!raceMenu.raceMode)throw std::runtime_error("Race selection unavailable");
 raceMenu.rotate(1);if(raceMenu.difficulty!=2)throw std::runtime_error("Race difficulty control unavailable");
 raceMenu.confirm();for(int k=0;k<260;k++)raceMenu.tick(1.f/60);if(!raceMenu.startRequested||!raceMenu.raceMode)throw std::runtime_error("Race selection lost on launch");
 StageClock raceClock;raceClock.begin(0,true);Rider parked;
 raceClock.tick(parked,300);if(raceClock.failed||!raceClock.active)throw std::runtime_error("Time trial timeout leaks into race mode");
 Road road;auto world=buildWorld(road);checkGroundSeams(world,road);checkArrivalSprites(road);size_t checked=0;float distantSlope=0;
 Frontend menu;auto confirm=[&](){menu.confirm();for(int k=0;k<36;k++)menu.tick(1.f/60);};confirm();menu.choose(1);confirm();menu.choose(4);confirm();
 if(menu.page!=MenuPage::Briefing||menu.startRequested)throw std::runtime_error("Tropical selection skipped briefing");
 menu.back();for(int k=0;k<36;k++)menu.tick(1.f/60);menu.choose(courseCount-4);confirm();confirm();
 if(menu.page!=MenuPage::Route||menu.startRequested)throw std::runtime_error("Route presentation skipped");
 menu.confirm();menu.choose(1);if(menu.country!=0)throw std::runtime_error("Input changed destination during presentation");
 for(int k=0;k<240;k++)menu.tick(1.f/60);
 if(menu.page!=MenuPage::Ride||!menu.startRequested||menu.pilot!=1)throw std::runtime_error("Menu selection lost before start");
 Frontend carousel;carousel.page=MenuPage::Pilots;carousel.carousel=3.9f;carousel.pilot=0;
 carousel.tick(1.f/60);if(carousel.carousel<3.9f)throw std::runtime_error("Carousel takes the long path at wrap");
 carousel.choose(-1);for(int i=0;i<90;i++)carousel.tick(1.f/60);
 if(carousel.pilot!=3||std::abs(carousel.carousel-3)>.001f)throw std::runtime_error("Carousel does not settle on selected pilot");
 carousel.click(20,335,960);if(carousel.pilot!=2)throw std::runtime_error("Carousel arrow hit test");
 StageClock timer;Rider timerRider;timer.begin();timer.tick(timerRider,0);
 if(timer.remaining!=courses[0].initialTime)throw std::runtime_error("Paused clock changed");
 timerRider.s=1000;timer.tick(timerRider,10);timer.tick(timerRider,.5f);
 if(timer.checkpoint!=1||std::abs(timer.remaining-(courses[0].initialTime+courses[0].checkpointTime-10.5f))>.001f)throw std::runtime_error("Checkpoint time award duplicated");
 timerRider.s=3000;timer.tick(timerRider,10);timerRider.s=4000;timerRider.finished=true;timer.tick(timerRider,1);
 if(!timer.won||timer.failed||timer.checkpoint!=3)throw std::runtime_error("Valid finish rejected");
 timer.begin();timerRider.finished=false;timerRider.s=0;timer.tick(timerRider,float(courses[0].initialTime+1));
 if(!timer.failed||timer.won)throw std::runtime_error("Time trial continues after time-out");
 std::cout<<"PASS frontend: navigation, locked destinations, pilot, checkpoint awards, pause, victory and timeout\n";
 // A held brake/steering key must not crash the automatic lead-in. Control and
 // clock start once, with continuous position and speed at the handover.
 RollingStart launch;launch.begin();StageClock launchClock;Rider starter;
 starter.reset(road);starter.s=15;starter.p=road.surface(15,0);starter.heading=road.at(15).heading;
 starter.pace=22;starter.velocity={std::sin(starter.heading)*22,0,std::cos(starter.heading)*22};
 int handovers=0;float launchOrigin=starter.s;
 for(int k=0;k<360;k++){
   auto controlled=launch.tick(road,starter,0,{1,1,false,true},1.f/120);
   starter.tick(road,controlled,0,1.f/120);
   if(launch.crossed){launchClock.begin();handovers++;}else launchClock.tick(starter,1.f/120);
   if(launch.active()&&launchClock.elapsed!=0)throw std::runtime_error("Clock consumed during rolling countdown");
 }
 if(starter.s-launchOrigin<55||starter.speed()<18||starter.crashes)throw std::runtime_error("Rolling countdown stopped or crashed");
 float lastSpeed=starter.speed(),lastS=starter.s;
 for(int k=0;k<6;k++){
   auto controlled=launch.tick(road,starter,0,{},1.f/120);starter.tick(road,controlled,0,1.f/120);
   if(launch.crossed){launchClock.begin();handovers++;}else launchClock.tick(starter,1.f/120);
 }
 if(handovers!=1||!launchClock.active||launchClock.elapsed<=0||starter.s<=lastS||std::abs(starter.speed()-lastSpeed)>1)
   throw std::runtime_error("Discontinuous rolling-start handover");
 std::cout<<"PASS rolling start: moves during countdown, ignores premature input, starts clock once, continuous handover\n";
 for(auto size:std::initializer_list<std::pair<int,int>>{{1920,1080},{2560,1440},{3440,1440},{2560,1080},{1440,810},{2048,1152}}){
   auto grid=fitPixelGrid(size.first,size.second);
   if(grid.width*grid.scale>size.first||grid.height*grid.scale>size.second||size.first-grid.width*grid.scale>=grid.scale||size.second-grid.height*grid.scale>=grid.scale)
     throw std::runtime_error("Pixel presentation stretches or leaves unnecessary borders");
 }
 for(int who=0;who<4;who++)for(int pose=0;pose<riderPoseCount;pose++){
   Rider actor;actor.reset(road);actor.pilot=who;actor.pose=pose;actor.previousPose=5;actor.poseBlend=.2f;
   auto mesh=buildRider(actor,road,actor.p+V3{0,2,-4},false,false);Mesh sprite;
   for(auto v:mesh)if(v.material>=5&&v.material<5.5f)sprite.push_back(v);
   if(sprite.size()!=6)throw std::runtime_error("Rider is sliced into deforming bands");
    auto b=pose==0||pose==5?windRects[who][pose==5?3:0]:rigRects[who][pose];float expected=(b.x1-b.x0)*1536/((b.y1-b.y0)*1024);
   float actual=length(sprite[1].p-sprite[0].p)/length(sprite[2].p-sprite[1].p);
   if(std::abs(expected-actual)>.001f)throw std::runtime_error("Rider sprite aspect ratio changed");
   actor.poseBlend=.9f;auto other=buildRider(actor,road,actor.p+V3{0,2,-4},false,false);size_t v=0;
   for(auto p:other)if(p.material>=5&&p.material<5.5f){if(length(p.p-sprite[v++].p)>.00001f)throw std::runtime_error("Pose interpolation squashes rider");}
 }
  std::cout<<"PASS pixel grid and all 52 rider poses: integer presentation, one billboard, preserved proportions, no squash interpolation\n";
  for(int who=0;who<4;who++)for(int pose:{0,5}){
    Rider actor;actor.reset(road);actor.pilot=who;actor.pose=pose;
    auto camera=actor.p+V3{0,2,-4};auto reference=buildRider(actor,road,camera,false,false);
    for(float time:{.18f,.35f,.52f}){
      actor.runTime=time;actor.windPhase=time*windRate(actor.pace);auto next=buildRider(actor,road,camera,false,false);
      if(next.size()!=reference.size())throw std::runtime_error("Wind changes rider topology");
      for(size_t j=0;j<next.size();j++)if(length(next[j].p-reference[j].p)>1e-7f||next[j].u!=reference[j].u||next[j].v!=reference[j].v)
        throw std::runtime_error("Wind moves the board, body geometry or shadow UVs");
    }
  }
  std::cout<<"PASS wind registration: four riders, neutral/tuck, all three frames keep identical geometry, UVs and shadow contacts\n";
 for(int who=0;who<4;who++)for(float phase:{.1f,.4f,.8f}){
   Rider fallen;fallen.reset(road);fallen.crashed=true;fallen.crashTime=phase;fallen.pilot=who;
   V3 camera=fallen.p+V3{2,2,-4},toward=norm(camera-(fallen.p+V3{0,.6f,0}));
   auto mesh=buildRider(fallen,road,camera,false,false);Mesh sprite;
   for(auto v:mesh)if(v.material>=22&&v.material<22.5f)sprite.push_back(v);
   if(sprite.size()!=6)throw std::runtime_error("Missing full fall sprite");
   V3 right=norm(sprite[1].p-sprite[0].p),up=norm(sprite[1].p-sprite[2].p);
   if(std::abs(dot(right,up))>.001f||std::abs(dot(up,toward))>.001f)
     throw std::runtime_error("Fall sprite is tilted twice and crushed in projection");
 }
 std::cout<<"PASS all 12 fall poses: orthonormal camera-facing basis, no second foreshortening\n";
  for(int who=0;who<4;who++)for(int side:{-1,1})for(float phase:{.05f,.18f,.3f,.42f,.8f}){
    Rider r;r.reset(road);r.pilot=who;r.crashed=true;r.crashSide=float(side);r.crashTime=phase;
   V3 camera=r.p+V3{2,2,-4};auto mesh=buildRider(r,road,camera,false,false);Mesh board;
   for(auto v:mesh)if(v.material==25)board.push_back(v);
   if(board.size()!=6)throw std::runtime_error("Detached board drawing missing");
    const V3 wheelColor[]={{1,.40f,.04f},{1,.19f,.26f},{1,.74f,.04f},{.58f,.32f,.86f}};
    for(auto& v:board)if(length(v.color-wheelColor[who])>.001f)throw std::runtime_error("Fallen board loses rider wheel color");
   V3 across=board[1].p-board[0].p,up=board[1].p-board[2].p;
   float sourceAspect=std::abs(board[1].u-board[0].u)*1536/(std::abs(board[1].v-board[2].v)*1024);
   if(std::abs(length(across)/length(up)-sourceAspect)>.001f||std::abs(dot(norm(across),norm(up)))>.001f||norm(up).y<.7f)
     throw std::runtime_error("Detached board flattened or its drawing stretched");
   V3 foot=(board[2].p+board[5].p)*.5f;
   if(length(foot-r.p)<.9f)throw std::runtime_error("Detached board hidden inside rider");
 }
 std::cout<<"PASS detached board: ten roll/settle poses, original atlas aspect, upright billboard, separated from rider\n";
 ArcadeFeedback feedback;Rider reward;reward.velocity={0,0,46};
 for(int k=0;k<120;k++)feedback.tick(reward,1.f/120);
 if(feedback.speedAwards!=1)throw std::runtime_error("Speed award not earned after sustained 160km/h");
 for(int k=0;k<120;k++)feedback.tick(reward,1.f/120);
 if(feedback.speedAwards!=1)throw std::runtime_error("Speed award spams while held");
 reward.boost=.7f;feedback.tick(reward,.01f);feedback.tick(reward,.01f);
 if(feedback.driftAwards!=1)throw std::runtime_error("Drift award missing or duplicated");
 reward.crashed=true;reward.boost=1;feedback.tick(reward,.01f);
 if(feedback.driftAwards!=1)throw std::runtime_error("Crash awarded a drift");
 FinishRunout coast;Rider end;end.s=Road::finishDistance;end.finished=true;end.velocity={0,0,48};end.p=road.surface(end.s,0);coast.begin(end);
 coast.tick(end,road,1.f/120);
 if(end.speed()<47||end.s<=Road::finishDistance)throw std::runtime_error("Finish destroys momentum");
 for(int k=0;k<730;k++)coast.tick(end,road,1.f/120);
 if(!coast.done||end.speed()!=0||end.s<=Road::finishDistance+90||end.s>Road::runoutEnd)throw std::runtime_error("Runout does not stop inside parking");
 std::cout<<"PASS arcade awards: earned speed, cooldown, clean release, no crash reward; continuous finish and parking stop\n";
 using Point=std::array<int,3>;using Edge=std::pair<Point,Point>;
 std::map<Edge,std::pair<int,int>> rockEdges;size_t rockTriangles=0;
 auto key=[](V3 p){return Point{int(std::round(p.x*10000)),int(std::round(p.y*10000)),int(std::round(p.z*10000))};};
 for(size_t i=0;i<world.vertices.size();i+=3){
   if(world.vertices[i].material==18){
     rockTriangles++;
     for(int j=0;j<3;j++){
       auto a=key(world.vertices[i+j].p),b=key(world.vertices[i+(j+1)%3].p);
       if(a==b)throw std::runtime_error("Degenerate cliff edge");
       int direction=a<b?1:-1;if(b<a)std::swap(a,b);
       auto& edge=rockEdges[{a,b}];edge.first++;edge.second+=direction;
     }
     V3 center=(world.vertices[i].p+world.vertices[i+1].p+world.vertices[i+2].p)*(1.f/3);
     if(sceneryPlantClear(center,0))throw std::runtime_error("Rock footprint allows a plant inside the cliff");
     auto loc=road.locate(center);
     if(loc.distance<4.7f&&center.y>roadGroundPoint(road,center,loc.s).y-.05f)
       throw std::runtime_error("Cliff crosses another hairpin branch");
   }
   if(world.vertices[i].material!=3)continue;
   V3 center=(world.vertices[i].p+world.vertices[i+1].p+world.vertices[i+2].p)*(1.f/3);
   auto loc=road.locate(center);
   // Distant ground used to acquire near-vertical cliffs at the arbitrary
   // boundary of the height interpolation support. These are terrain faces,
   // not the deliberately steep, closed rock meshes (material 18).
   if(loc.distance>100){
     float ny=std::abs(world.vertices[i].n.y);
     distantSlope=std::max(distantSlope,std::sqrt(std::max(0.f,1-ny*ny))/std::max(ny,.00001f));
     if(ny<.555f)throw std::runtime_error("Discontinuous distant terrain slope at "+std::to_string(loc.s));
   }
   if(loc.distance<4.6f&&center.y>roadGroundPoint(road,center,loc.s).y-.01f)
     throw std::runtime_error("Terrain triangle crosses the drivable road at "+std::to_string(loc.s));
   checked++;
 }
 for(const auto& [edge,count]:rockEdges){
   (void)edge;
   if(count.first!=2||count.second!=0)throw std::runtime_error("Open or incorrectly oriented cliff surface");
 }
 if(rockTriangles<1000)throw std::runtime_error("Missing cliff geometry");
 if(!sceneryPlantClear({10000,0,10000},5))throw std::runtime_error("Rock exclusion consumes open terrain");
 size_t plantChecks=0;
 for(float at:{360.f,470.f,640.f,965.f,1070.f})for(float angle:{-.7f,.7f}){
   Rider viewer;viewer.reset(road);viewer.s=at;viewer.p=road.at(at).p;
   float yaw=road.at(at).heading+angle;
   V3 camera=viewer.p-V3{std::sin(yaw)*4,-2,std::cos(yaw)*4};
   Mesh plants;appendTrees(plants,road,viewer,camera);
   for(const auto& v:plants){
     if((v.material>=13&&v.material<13.5)||(v.material>=20&&v.material<21.5)){
       if(!sceneryPlantClear(v.p,0))throw std::runtime_error("Vegetation quad enters a rock footprint");
       plantChecks++;
     }
   }
 }
 if(plantChecks<10000)throw std::runtime_error("Missing vegetation coverage");
 std::cout<<"PASS vegetation: "<<plantChecks<<" billboard vertices clear rocks at ten course/camera positions\n";
 // Adjacent branches of a hairpin share world space. A change of route
 // coordinate alone must not make grass bands appear in front of the camera.
 Rider branch;branch.reset(road);branch.s=640;branch.p=road.at(640).p;
 V3 fixedCamera=branch.p-V3{0,-2,4};Mesh before,after;
 appendTrees(before,road,branch,fixedCamera);branch.s+=60;appendTrees(after,road,branch,fixedCamera);
 if(before.size()!=after.size())throw std::runtime_error("Vegetation depends on route-distance LOD thresholds");
 for(size_t i=0;i<before.size();i++)if(length(before[i].p-after[i].p)>.0001f||before[i].material!=after[i].material)
   throw std::runtime_error("Grass changes at fixed world camera position");
 // The shadow frustum must cover the nearby road at every course heading.
 for(float at=20;at<1450;at+=25){
   V3 center=road.at(at).p;Mat sun=sunlightMatrix(center);
   float clip[3]{};for(int row=0;row<3;row++)clip[row]=sun.m[row*4]*center.x+sun.m[row*4+1]*center.y+sun.m[row*4+2]*center.z+sun.m[row*4+3];
   if(std::abs(clip[0])>.002f||std::abs(clip[1])>.002f||clip[2]<.1f||clip[2]>.9f)throw std::runtime_error("Sun shadow frustum loses its focus");
 }
 Rider r;r.velocity={0,0,10};r.runTime=3;
 auto still=cameraReaction(r);
 if(still.vertical!=0||still.lateral!=0||still.roll!=0)throw std::runtime_error("Shake at low speed");
 float peak=0;
 for(int frame=0;frame<1200;frame++){
   r.velocity={0,0,47};r.drift=1;r.runTime=frame/120.f;
   auto c=cameraReaction(r);peak=std::max(peak,std::abs(c.vertical));
   if(std::abs(c.vertical)>.027f||std::abs(c.lateral)>.018f||std::abs(c.roll)>.005f)
     throw std::runtime_error("Excessive sustained camera shake");
 }
 if(peak<.015f)throw std::runtime_error("Missing speed and drift camera reaction");
 r.crashed=true;r.crashTime=0;r.crashSide=1;
 auto impact=cameraReaction(r);r.crashTime=1.5f;auto settled=cameraReaction(r);
 if(impact.vertical<.08f||std::abs(settled.vertical)>.014f)throw std::runtime_error("Crash impulse does not settle");
 std::cout<<"PASS closed cliffs: "<<rockTriangles<<" triangles, paired oriented edges, road clearance; bounded camera effects\n";
 std::cout<<"PASS scene: "<<checked<<" terrain triangles checked against the whole road corridor; maximum distant slope "<<distantSlope<<"\n";
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}

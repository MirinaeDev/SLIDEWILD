#include "world.hpp"
#include "runtime_paths.hpp"
#include "sound.hpp"
#include "camera_effects.hpp"
#include "immersion.hpp"
#include "frontend.hpp"
#include "pause_menu.hpp"
#include "arcade.hpp"
#include "rig_data.hpp"
#include "wind_registration.hpp"
#include <SDL3/SDL_main.h>
#include <fstream>
#include <filesystem>
#include <cstdio>
#include <iostream>
#include <string>
#include <chrono>
using namespace dh;
int main(int argc,char**argv){
 bool vertexProbe=false,sessionProbe=false;int resultProbe=-1;
 bool raceFlag=true;int raceDifficulty=1;
 int poseProbe=-1;bool balanceProbe=false,brakeProbe=false;
 bool tourProbe=false,finishProbe=false,frontendProbe=false,fastProbe=false,crashProbe=false,record=false,carveProbe=false,pauseCapture=false,audioCheck=false,driftProbe=false,smoke=false,capture=false,demo=false,wide=false,rain=false;int menuPage=-1,pilotOverride=0,courseOverride=0,frontendDestination=-1;float menuTime=0,sceneTime=0;int limit=180;float start=365,startSpeed=Rider::startSpeed;
 for(int i=1;i<argc;i++){std::string a=argv[i];if(a=="--vertex-test")vertexProbe=true;if(a=="--session-test"){sessionProbe=true;smoke=true;}if(a=="--result-test"&&i+1<argc){resultProbe=std::clamp(std::stoi(argv[++i]),0,2);smoke=true;menuPage=5;}if(a=="--pose-test"&&i+1<argc)poseProbe=std::clamp(std::stoi(argv[++i]),0,riderPoseCount-1);if(a=="--balance-test")balanceProbe=true;if(a=="--brake-test")brakeProbe=true;if(a=="--scene-time"&&i+1<argc)sceneTime=std::max(0.f,std::stof(argv[++i]));if(a=="--race")raceFlag=true;if(a=="--chrono")raceFlag=false;if(a=="--difficulty"&&i+1<argc)raceDifficulty=std::clamp(std::stoi(argv[++i]),0,2);if(a=="--frontend-destination"&&i+1<argc)frontendDestination=std::clamp(std::stoi(argv[++i]),0,courseCount-1);if(a=="--course"&&i+1<argc)courseOverride=std::clamp(std::stoi(argv[++i]),0,courseCount-1);if(a=="--tour-test"){tourProbe=true;smoke=true;demo=true;start=15;startSpeed=22;}if(a=="--finish-test"){finishProbe=true;demo=true;start=3970;startSpeed=45;}if(a=="--pilot"&&i+1<argc)pilotOverride=std::clamp(std::stoi(argv[++i]),0,3);if(a=="--menu-time"&&i+1<argc)menuTime=std::stof(argv[++i]);if(a=="--frontend-test"){frontendProbe=true;menuPage=0;}if(a=="--menu-page"&&i+1<argc)menuPage=std::clamp(std::stoi(argv[++i]),0,6);if(a=="--fast-test")fastProbe=true;if(a=="--speed"&&i+1<argc)startSpeed=clamp(std::stof(argv[++i]),0,58);if(a=="--crash-test")crashProbe=true;if(a=="--record"){record=true;smoke=true;}if(a=="--carve-test"){carveProbe=true;driftProbe=true;}if(a=="--paused")pauseCapture=true;if(a=="--audio-check")audioCheck=true;if(a=="--drift-test")driftProbe=true;if(a=="--smoke")smoke=true;if(a=="--capture")capture=true;if(a=="--demo")demo=true;if(a=="--wide")wide=true;if(a=="--rain")rain=true;if(a=="--frames"&&i+1<argc)limit=std::max(1,std::stoi(argv[++i]));if(a=="--start"&&i+1<argc)start=std::stof(argv[++i]);}
 try{
 initializeOutputPaths(smoke);
 PauseMenu pauseMenu;Frontend menu;Race race;menu.raceMode=raceFlag;menu.difficulty=raceDifficulty;StageClock stageClock;RollingStart launch;FinishRunout runout;ArcadeFeedback feedback;menu.pilot=pilotOverride;menu.country=courseOverride;menu.time=menuTime;menu.orbit=menuTime;menu.pageTime=menuTime;
 if(smoke&&menuPage<0)menu.page=MenuPage::Ride;else if(menuPage>=0)menu.page=MenuPage(menuPage);
 Renderer renderer;
 if(!smoke)SDL_SetWindowFullscreen(renderer.window,true);
 RideSound sound(!smoke||audioCheck);bool muted=false;if(wide)SDL_SetWindowSize(renderer.window,1680,720);Road road(courseOverride);auto scenery=buildWorld(road);renderer.uploadWorld(scenery.vertices);Rider rider;RideEffects effects;ImmersionState immersion;auto resetRider=[&](){effects.reset();immersion={};feedback={};runout={};rider.reset(road);rider.pilot=menu.pilot;rider.s=clamp(start,1,road.size()-5);auto initial=road.at(rider.s);rider.p=initial.p;rider.heading=initial.heading;rider.runTime=sceneTime;rider.windPhase=sceneTime*windRate(startSpeed);rider.pace=startSpeed;rider.velocity={std::sin(initial.heading)*startSpeed,0,std::cos(initial.heading)*startSpeed};race.reset(road,rider,menu.raceMode,menu.difficulty);};resetRider();Rider previous=rider;if(finishProbe){
   rider.reset(road);rider.pilot=menu.pilot;rider.pace=22;rider.velocity=norm(rider.velocity)*22;launch.begin();
   for(int step=0;step<19000&&rider.s<3970;step++){
     auto driving=launch.tick(road,rider,std::max(rain?1.f:0.f,courseRain(road.course,rider.s)),demoInput(road,rider,std::max(rain?1.f:0.f,courseRain(road.course,rider.s))),1.f/120);
     rider.tick(road,driving,std::max(rain?1.f:0.f,courseRain(road.course,rider.s)),1.f/120);
     if(launch.crossed)stageClock.begin(road.course,menu.raceMode);else stageClock.tick(rider,1.f/120);
   }
   if(stageClock.failed||rider.crashes||rider.s<3970)throw std::runtime_error("Complete pre-finish simulation failed");
   previous=rider;
 }
 if(tourProbe||sessionProbe)launch.begin();
 if(resultProbe>=0){
   stageClock.begin(road.course,resultProbe!=0);menu.raceMode=resultProbe!=0;stageClock.elapsed=132.56f;stageClock.active=false;stageClock.won=resultProbe!=0;stageClock.failed=resultProbe==0;
   race.playerFinish=132.56f;for(int j=0;j<3;j++)race.rivals[j].finish=135.f+j*2-(resultProbe==2?9:0);
 }
 std::ofstream log(outputPath("run.log"));log<<"Resources "<<pathUtf8(resourcePath(""))<<"\nOutput "<<pathUtf8(outputDirectory)<<"\n";log<<"Fullscreen "<<bool(SDL_GetWindowFlags(renderer.window)&SDL_WINDOW_FULLSCREEN)<<"\n";log<<"GPU "<<renderer.driver<<"\nWorld vertices "<<scenery.vertices.size()<<"\nAudio "<<sound.available()<<'\n'<<"Music "<<sound.musicAvailable()<<'\n';log.flush();
 int countdownTone=-1;
 unsigned heardTransition=0;
 bool running=true,paused=menu.visible()||!smoke||pauseCapture,help=false,fullscreen=!smoke;float wet=std::max(rain?1.f:0.f,courseRain(road.course,rider.s)),wetTarget=rain?1.f:0.f;int frames=0;double accumulator=0;constexpr double dt=1.0/120;
 Uint64 last=SDL_GetPerformanceCounter();double frequency=double(SDL_GetPerformanceFrequency());double buildSeconds=0,drawSeconds=0,scenerySeconds=0,effectsSeconds=0,riderSeconds=0;double measuredSeconds=0;float fps=60;V3 cam{},look{};float cameraHeading=rider.heading,cameraRoll=0,cameraFov=69;bool cameraReset=true;SDL_Gamepad*pad=nullptr;
 int n=0;auto ids=SDL_GetGamepads(&n);if(n>0)pad=SDL_OpenGamepad(ids[0]);SDL_free(ids);
 while(running){
   bool shot=false;SDL_Event event;
   auto pauseAction=[&](PauseAction action){
     if(action==PauseAction::None)return;
     pauseMenu.reset();
     if(action==PauseAction::Resume){paused=false;return;}
     // Aborting is never a finish. A fresh rolling start resets the session.
     stageClock={};launch={};runout={};
     menu.travel(action==PauseAction::Restart?MenuPage::Route:MenuPage::Globe);
     paused=true;
   };
   if(sessionProbe){
     SDL_Scancode key=SDL_SCANCODE_UNKNOWN;
     if(frames==10||frames==20||frames==30||frames==70)key=SDL_SCANCODE_ESCAPE;
     if(frames==40||frames==50)key=SDL_SCANCODE_DOWN;
     if(frames==60||frames==100||frames==120)key=SDL_SCANCODE_RETURN;
     if(frames==110)key=SDL_SCANCODE_DOWN;
     if(frames==25&&(paused||menu.visible()||stageClock.won))throw std::runtime_error("Escape did not resume the race safely");
     if(frames==75&&(!paused||pauseMenu.confirming||menu.visible()))throw std::runtime_error("Escape did not cancel abandon confirmation");
     if(frames==160&&(menu.page!=MenuPage::Globe||stageClock.won||stageClock.failed))throw std::runtime_error("Abandon produced a race result");
     if(key!=SDL_SCANCODE_UNKNOWN){SDL_Event test{};test.type=SDL_EVENT_KEY_DOWN;test.key.scancode=key;SDL_PushEvent(&test);}
   }
   bool transitionShot=frontendProbe&&capture&&(frames==102||frames==107||frames==750||frames==810||frames==850);
   shot=transitionShot||(sessionProbe&&frames==65);
   if(frontendProbe){
     SDL_Scancode key=SDL_SCANCODE_UNKNOWN;
     if(frames==90||frames==560||frames==650)key=SDL_SCANCODE_RETURN;
     if(frames==140||frames==300||frames==460)key=SDL_SCANCODE_RIGHT;
     if(frames==350||frames==510)key=SDL_SCANCODE_LEFT;
     if(frames==510&&frontendDestination>0)key=frontendDestination==1?SDL_SCANCODE_UNKNOWN:SDL_SCANCODE_RIGHT;
     if((frames==530&&frontendDestination>=3)||(frames==540&&frontendDestination>=4))key=SDL_SCANCODE_RIGHT;
     if(frames==550&&frontendDestination>=5)key=SDL_SCANCODE_RIGHT;
     if(frames==553&&frontendDestination>=6)key=SDL_SCANCODE_RIGHT;
     if(frames==556&&frontendDestination==7)key=SDL_SCANCODE_RIGHT;
     if(frames==250)key=SDL_SCANCODE_E;
     if(key!=SDL_SCANCODE_UNKNOWN){SDL_Event scripted{};scripted.type=SDL_EVENT_KEY_DOWN;scripted.key.scancode=key;scripted.key.down=true;SDL_PushEvent(&scripted);}
     if(frames==200||frames==205||frames==420){
       int ww,hh;SDL_GetWindowSize(renderer.window,&ww,&hh);
       float x=frames==200?renderer.width*.5f:(frames==205?60.f:renderer.width-151.f);
       float y=frames==200?295.f:491.f;
       SDL_Event pointer{};pointer.type=SDL_EVENT_MOUSE_BUTTON_DOWN;pointer.button.button=SDL_BUTTON_LEFT;pointer.button.down=true;
       pointer.button.x=x*ww/renderer.width;pointer.button.y=y*hh/540;
       SDL_PushEvent(&pointer);pointer.type=SDL_EVENT_MOUSE_BUTTON_UP;pointer.button.down=false;SDL_PushEvent(&pointer);
     }
   }
   while(SDL_PollEvent(&event)){
    if(event.type==SDL_EVENT_QUIT)running=false;
    if(!menu.visible()&&event.type==SDL_EVENT_GAMEPAD_BUTTON_DOWN){
      SDL_Scancode key=SDL_SCANCODE_UNKNOWN;
      if(event.gbutton.button==SDL_GAMEPAD_BUTTON_START)key=SDL_SCANCODE_ESCAPE;
      if(paused){
        if(event.gbutton.button==SDL_GAMEPAD_BUTTON_EAST)key=SDL_SCANCODE_ESCAPE;
        if(event.gbutton.button==SDL_GAMEPAD_BUTTON_SOUTH)key=SDL_SCANCODE_RETURN;
        if(event.gbutton.button==SDL_GAMEPAD_BUTTON_DPAD_UP)key=SDL_SCANCODE_UP;
        if(event.gbutton.button==SDL_GAMEPAD_BUTTON_DPAD_DOWN)key=SDL_SCANCODE_DOWN;
      }
      if(key!=SDL_SCANCODE_UNKNOWN){event={};event.type=SDL_EVENT_KEY_DOWN;event.key.scancode=key;}
    }
    // The page changes underneath an opaque wipe. Keep input locked through
    // its reveal too, so a second confirm cannot pause the rolling start.
    if(menu.transitioning){
      if(event.type==SDL_EVENT_KEY_DOWN&&event.key.scancode!=SDL_SCANCODE_F11&&event.key.scancode!=SDL_SCANCODE_F12&&event.key.scancode!=SDL_SCANCODE_N)continue;
      if(event.type==SDL_EVENT_GAMEPAD_BUTTON_DOWN||event.type==SDL_EVENT_MOUSE_BUTTON_DOWN||event.type==SDL_EVENT_MOUSE_WHEEL)continue;
    }
    if(menu.visible()){
      if(event.type==SDL_EVENT_KEY_DOWN&&!event.key.repeat){
        auto key=event.key.scancode;
        if(key==SDL_SCANCODE_Q||key==SDL_SCANCODE_E){menu.rotate(key==SDL_SCANCODE_Q?-.7854f:.7854f);continue;}
        if(key==SDL_SCANCODE_LEFT||key==SDL_SCANCODE_UP){menu.choose(-1);sound.cue();continue;}
        if(key==SDL_SCANCODE_RIGHT||key==SDL_SCANCODE_DOWN){menu.choose(1);sound.cue();continue;}
        if(key==SDL_SCANCODE_RETURN||key==SDL_SCANCODE_SPACE){menu.confirm();sound.cue(2);continue;}
        if(key==SDL_SCANCODE_ESCAPE){if(menu.page==MenuPage::Title)running=false;else menu.back();continue;}
        if(key!=SDL_SCANCODE_F11&&key!=SDL_SCANCODE_F12&&key!=SDL_SCANCODE_N)continue;
      }
      if(event.type==SDL_EVENT_GAMEPAD_BUTTON_DOWN){
        if(event.gbutton.button==SDL_GAMEPAD_BUTTON_SOUTH){menu.confirm();sound.cue(2);}
        if(event.gbutton.button==SDL_GAMEPAD_BUTTON_LEFT_SHOULDER)menu.rotate(-.7854f);
        if(event.gbutton.button==SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER)menu.rotate(.7854f);
        if(event.gbutton.button==SDL_GAMEPAD_BUTTON_EAST)menu.back();
        if(event.gbutton.button==SDL_GAMEPAD_BUTTON_DPAD_LEFT||event.gbutton.button==SDL_GAMEPAD_BUTTON_DPAD_UP)menu.choose(-1);
        if(event.gbutton.button==SDL_GAMEPAD_BUTTON_DPAD_RIGHT||event.gbutton.button==SDL_GAMEPAD_BUTTON_DPAD_DOWN)menu.choose(1);
      }
      if(event.type==SDL_EVENT_MOUSE_BUTTON_DOWN&&event.button.button==SDL_BUTTON_LEFT){
        int ww,hh;SDL_GetWindowSize(renderer.window,&ww,&hh);menu.click(event.button.x*renderer.width/ww,event.button.y*540/hh,renderer.width);
      }
      if(event.type==SDL_EVENT_MOUSE_BUTTON_UP)menu.dragging=false;
      if(event.type==SDL_EVENT_MOUSE_MOTION&&menu.dragging){menu.rotate(-event.motion.xrel*.009f);}
      if(event.type==SDL_EVENT_MOUSE_WHEEL)menu.choose(event.wheel.y>0?-1:1);
    }
    if(event.type==SDL_EVENT_WINDOW_FOCUS_LOST&&!smoke&&!menu.visible()){if(!paused)pauseMenu.reset();paused=true;}
    if(!menu.visible()&&!menu.transitioning&&paused&&event.type==SDL_EVENT_MOUSE_BUTTON_DOWN&&event.button.button==SDL_BUTTON_LEFT){
      int ww,hh;SDL_GetWindowSize(renderer.window,&ww,&hh);pauseAction(pauseMenu.click(event.button.x*renderer.width/ww,event.button.y*540/hh,renderer.width));continue;
    }
    if(!menu.visible()&&!menu.transitioning&&paused&&event.type==SDL_EVENT_KEY_DOWN&&!event.key.repeat){
      auto key=event.key.scancode;
      if(key==SDL_SCANCODE_ESCAPE||key==SDL_SCANCODE_P){pauseAction(pauseMenu.back());continue;}
      if(key==SDL_SCANCODE_RETURN||key==SDL_SCANCODE_SPACE){pauseAction(pauseMenu.confirm());sound.cue(2);continue;}
      if(key==SDL_SCANCODE_UP||key==SDL_SCANCODE_LEFT){pauseMenu.choose(-1);sound.cue();continue;}
      if(key==SDL_SCANCODE_DOWN||key==SDL_SCANCODE_RIGHT){pauseMenu.choose(1);sound.cue();continue;}
      if(key==SDL_SCANCODE_R){pauseMenu.selection=1;pauseMenu.confirming=true;pauseMenu.accept=false;continue;}
    }
    if(event.type==SDL_EVENT_GAMEPAD_ADDED&&!pad)pad=SDL_OpenGamepad(event.gdevice.which);
    if(event.type==SDL_EVENT_GAMEPAD_REMOVED&&pad&&SDL_GetGamepadID(pad)==event.gdevice.which){SDL_CloseGamepad(pad);pad=nullptr;}
    if(event.type==SDL_EVENT_KEY_DOWN&&!event.key.repeat){switch(event.key.scancode){
      case SDL_SCANCODE_ESCAPE:case SDL_SCANCODE_P:if(!menu.visible()){pauseMenu.reset();paused=true;}break;case SDL_SCANCODE_RETURN:break;case SDL_SCANCODE_H:help=!help;break;
      case SDL_SCANCODE_R:if(!menu.visible()){paused=true;pauseMenu.reset();pauseMenu.selection=1;pauseMenu.confirming=true;}break;
      case SDL_SCANCODE_N:muted=!muted;break;case SDL_SCANCODE_M:wetTarget=1-wetTarget;break;case SDL_SCANCODE_F2:if(!stageClock.active&&!launch.active())demo=!demo;break;
      case SDL_SCANCODE_F11:fullscreen=!fullscreen;SDL_SetWindowFullscreen(renderer.window,fullscreen);break;
      case SDL_SCANCODE_F12:shot=true;break;default:break;
      case SDL_SCANCODE_G:renderer.integerPixels=!renderer.integerPixels;break;
    }}
   }
   if(menu.startRequested){if(road.course!=menu.country){road=Road(menu.country);scenery=buildWorld(road);renderer.uploadWorld(scenery.vertices);}start=15;startSpeed=22;resetRider();previous=rider;cameraReset=true;wet=std::max(wetTarget,courseRain(road.course,rider.s));stageClock={};launch.begin();paused=false;demo=false;menu.startRequested=false;}
   if(menu.visible())paused=true;
   Uint64 now=SDL_GetPerformanceCounter();double realElapsed=double(now-last)/frequency;measuredSeconds+=realElapsed;double elapsed=std::min(.10,realElapsed);last=now;fps=approach(fps,float(1/std::max(.0001,realElapsed)),2,float(elapsed));if(smoke)elapsed=tourProbe?1.0/15:1.0/60;
   menu.tick(float(elapsed));
   if(heardTransition!=menu.transitionSerial){heardTransition=menu.transitionSerial;if(!muted)sound.cue(4);}
   Input input;auto keys=SDL_GetKeyboardState(nullptr);input.steer=float(keys[SDL_SCANCODE_RIGHT]||keys[SDL_SCANCODE_D])-float(keys[SDL_SCANCODE_LEFT]||keys[SDL_SCANCODE_A]);input.brake=keys[SDL_SCANCODE_DOWN]||keys[SDL_SCANCODE_S];input.tuck=keys[SDL_SCANCODE_UP]||keys[SDL_SCANCODE_W];input.slide=keys[SDL_SCANCODE_SPACE];
   if(pad){float axis=SDL_GetGamepadAxis(pad,SDL_GAMEPAD_AXIS_LEFTX)/32767.f;if(std::abs(axis)>.12f)input.steer=clamp((std::abs(axis)-.12f)/.88f,0,1)*(axis>0?1:-1);input.brake=std::max(input.brake,SDL_GetGamepadAxis(pad,SDL_GAMEPAD_AXIS_LEFT_TRIGGER)/32767.f);input.tuck|=SDL_GetGamepadAxis(pad,SDL_GAMEPAD_AXIS_RIGHT_TRIGGER)>6000;input.slide|=SDL_GetGamepadButton(pad,SDL_GAMEPAD_BUTTON_SOUTH);}
   if(!paused&&!menu.transitioning){accumulator+=elapsed;while(accumulator>=dt){previous=rider;wet=approach(wet,std::max(wetTarget,courseRain(road.course,rider.s)),.45f,float(dt));
      if(demo||fastProbe)input=demoInput(road,rider,wet);
      if(fastProbe){input.brake=0;input.tuck=true;}
      if(driftProbe){input={frames<45?0.f:(frames<85?.6f:-.6f),0,true,!carveProbe&&frames>=45&&frames<110};}
      if(crashProbe)input={1,0,false,false};
      if(balanceProbe)input={1,0,true,true};
      if(brakeProbe)input={0,1,false,false};
      if(runout.active){runout.tick(rider,road,float(dt));race.tick(road,rider,wetTarget,false,float(dt));accumulator-=dt;
        if(runout.done){race.complete(road,rider,wetTarget);menu.travel(MenuPage::Results);paused=true;accumulator=0;break;}
        continue;
      }
      Input driving=launch.tick(road,rider,wet,input,float(dt));
      rider.tick(road,driving,wet,float(dt));race.tick(road,rider,wetTarget,launch.active(),float(dt),previous.s);effects.tick(rider,road,wet,float(dt),sceneryHeight);immersion.tick(rider,wet,float(dt));feedback.tick(rider,float(dt));if(launch.crossed)stageClock.begin(road.course,menu.raceMode);else stageClock.tick(rider,float(dt));accumulator-=dt;if(stageClock.won&&!runout.active&&!runout.done){runout.begin(rider);}
      if(stageClock.failed){menu.travel(MenuPage::Results);paused=true;accumulator=0;break;}
   }}else{accumulator=0;previous=rider;}
   int tone=launch.active()?int(std::ceil(launch.remaining)):(launch.go>0?0:-1);
   if(tone!=countdownTone&&tone>=0)sound.cue(tone==0?3:2);
   countdownTone=tone;
   sound.menu(menu.visible());
   sound.update(rider.speed(),rider.drift,rider.edgeImpact,paused,immersion.rough,immersion.water,muted);
   float stormPhase=std::fmod(rider.runTime,23.7f);
   float rumble=road.course==4&&wet>.8f?std::exp(-std::abs(stormPhase-18.8f)*1.6f):0;
   if(road.course==7){
     float index=std::round((rider.s-180)/126),distance=std::abs(rider.s-(180+index*126));
     rumble=.32f*lavaBurst(rider.runTime,index*.93f)*clamp(1-distance/80,0,1);
   }
   sound.storm(!paused&&!muted?rumble:0);
   Rider visible=interpolate(previous,rider,float(accumulator/dt));if(poseProbe>=0)visible.pose=poseProbe;auto rp=road.at(visible.s);
   // Follow translation immediately; smooth only orientation. Old position damping
   // added speed-dependent metres of camera lag, shrinking the rider and killing speed.
   if(cameraReset){cameraHeading=rp.heading;cameraRoll=0;cameraReset=false;}
   float bend=wrap(road.at(visible.s+10).heading-rp.heading);
   cameraHeading+=wrap(rp.heading+bend*.23f+visible.boardAngle*.08f-cameraHeading)*(1-std::exp(-float(elapsed)*10));
   auto reaction=cameraReaction(visible);
   if(road.course==7){reaction.vertical+=rumble*.055f*std::sin(visible.runTime*33);reaction.roll+=rumble*.004f*std::cos(visible.runTime*23); }
   float motionAmount=clamp(visible.speed()/25,0,1);
   reaction.vertical+=immersion.rough*motionAmount*std::sin(visible.s*8)*.018f;
   reaction.roll+=(immersion.slideEntry*.009f-immersion.gripReturn*.007f)*visible.driftDirection;
   reaction.pitch+=immersion.slideEntry*.009f-immersion.gripReturn*.006f;
   float banking=road.course>=5?-std::atan(rp.bank)*(road.course==6?.40f:.26f):0;
   cameraRoll=approach(cameraRoll,clamp(-bend*.145f-visible.lean*.026f,-.082f,.082f)+banking,7,float(elapsed));
   float targetFov=66+clamp((visible.speed()-20)*.88f,0,29)+visible.boost*3.2f;
   if(visible.crashed)targetFov=65;
   cameraFov=approach(cameraFov,targetFov,4,float(elapsed));
   float intro=launch.remaining/3;
   V3 cameraForward{std::sin(cameraHeading),0,std::cos(cameraHeading)};
   V3 camRight{cameraForward.z,0,-cameraForward.x};
   cam=visible.p-cameraForward*(3.8f+intro*.8f-clamp((cameraFov-69)*.035f,0,.88f))-camRight*(visible.lean*.18f-reaction.lateral+intro*.6f)+V3{0,2.15f-clamp((visible.speed()-24)/30,0,1)*.14f+intro*.45f-visible.crouch*.045f-visible.boost*.06f+reaction.vertical,0};
   auto ahead=road.at(visible.s+20);
   look=mix(visible.p+cameraForward*15,ahead.p,.28f)+V3{0,.40f-reaction.pitch*15,0};
   if(road.course>=6){
     // Follow the actual banked surface behind the board: a horizontal chase
     // offset buries the camera inside the track during the steepest plunges.
     float back=3.8f+intro*.8f-clamp((cameraFov-69)*.035f,0,.88f);
     cam.y=road.surface(visible.s-back,visible.d).y+2.1f+intro*.45f+reaction.vertical;
     V3 target=road.surface(visible.s+15,visible.d*.35f)+V3{0,.55f-reaction.pitch*10,0};
     look=mix(visible.p+cameraForward*15,target,.82f);
     look.y=target.y;
   }
   int w,h;SDL_GetWindowSizeInPixels(renderer.window,&w,&h);renderer.configurePixels(w,h,smoke);
   Uniforms u{};u.raster[0]=float(renderer.pixelWidth);u.raster[1]=float(renderer.pixelHeight);u.raster[2]=float(rider.pilot);u.raster[3]=float(road.course);float aspect=float(renderer.width)/540;float effectiveRoll=cameraRoll+reaction.roll;
   auto projection=cameraProjection(cam,look,cameraFov*pi/180,aspect,effectiveRoll);
   auto animateRider=[&](const Rider& actor){int who=std::clamp(actor.pilot,0,3);auto& a=u.riderState[who];a[0]=float(actor.pose);a[1]=actor.runTime;a[2]=actor.wheelTravel;a[3]=actor.pace;
     if(actor.pose==0||actor.pose==5){int base=actor.pose==5?3:0,frame=base+windFrame(actor.windPhase,actor.pace);auto b=windRects[who][base];
       u.cloth[who][0]=windRegistration[who][frame][0];u.cloth[who][1]=windRegistration[who][frame][1];
       u.cloth[who][2]=mix(b.y0,b.y1,.18f);u.cloth[who][3]=mix(b.y0,b.y1,.48f);
     }
   };
   animateRider(visible);if(race.enabled)for(const auto& opponent:race.rivals)animateRider(opponent.rider);
   const V3 rays[]={projection.rayX,projection.rayY,projection.forward};float* rayUniforms[]={u.rayX,u.rayY,u.rayZ};
   for(int k=0;k<3;k++){rayUniforms[k][0]=rays[k].x;rayUniforms[k][1]=rays[k].y;rayUniforms[k][2]=rays[k].z;}
   u.sunMvp=sunlightMatrix(visible.p+cameraForward*23);
   u.dynamics[0]=clamp(visible.speed()/58,0,1);u.dynamics[1]=visible.edgeImpact;u.dynamics[2]=immersion.exposure;u.dynamics[3]=wet;
   u.mvp=projection.matrix;u.eye[3]=road.course==7?rumble/.32f:0;u.eye[0]=cam.x;u.eye[1]=cam.y;u.eye[2]=cam.z;u.weather[0]=wet;u.weather[1]=rider.runTime;u.weather[2]=float(renderer.width)/540;u.weather[3]=effectiveRoll;u.camera[0]=std::atan2(look.x-cam.x,look.z-cam.z);u.camera[1]=std::atan2(look.y-cam.y,length(V3{look.x-cam.x,0,look.z-cam.z}));u.camera[2]=cameraFov*pi/180;u.camera[3]=aspect;
   Uint64 buildStart=SDL_GetPerformanceCounter();
   auto sky=buildSky(renderer.width,wet,std::atan2(look.x-cam.x,look.z-cam.z));Mesh moving;
   if(!menu.visible()){
     moving=buildRider(visible,road,cam,input.tuck,input.slide);riderSeconds+=double(SDL_GetPerformanceCounter()-buildStart)/frequency;
     if(race.enabled)for(const auto&opponent:race.rivals){
       Rider other=interpolate(opponent.previous,opponent.rider,float(accumulator/dt));
       if(length(other.p-cam)>220)continue;
       auto body=buildRider(other,road,cam,false,false);
       for(auto&v:body){
         if(v.material>=5&&v.material<5.5f)v.material=36.f+other.pilot;
         else if(v.material>=22&&v.material<22.5f)v.material=40.f+other.pilot;
         else if(v.material>=14&&v.material<14.5f)v.material=44.f+other.pilot;
       }
       moving.insert(moving.end(),body.begin(),body.end());
     }
     Uint64 sceneryStart=SDL_GetPerformanceCounter();appendTrees(moving,road,visible,cam);scenerySeconds+=double(SDL_GetPerformanceCounter()-sceneryStart)/frequency;
     Uint64 effectsStart=SDL_GetPerformanceCounter();appendEffects(moving,effects,cam,visible,road);effectsSeconds+=double(SDL_GetPerformanceCounter()-effectsStart)/frequency;
   }
   auto hud=menu.visible()?menu.draw(renderer.width,stageClock,&race):buildHud(renderer.width,rider,road,wet,false,demo,fps,help);
   if(!menu.visible()){
     auto clockHud=menu.draw(renderer.width,stageClock,&race);hud.insert(hud.end(),clockHud.begin(),clockHud.end());
     if(race.enabled){
       int place=race.place(rider);float cx=renderer.width*.5f;char timeText[40];
       uiCentered(hud,renderer.width,"POSITION",cx,13,0,{.85f,.89f,.92f});
       uiCentered(hud,renderer.width,std::to_string(place)+" / 4",cx,31,2,{1,.72f,.34f});
       std::snprintf(timeText,sizeof(timeText),"VS RIVALS  /  %05.2f S",race.elapsed);
       uiText(hud,renderer.width,timeText,24,24,0,{.9f,.92f,.88f});
       uiText(hud,renderer.width,Race::difficultyNames[race.difficulty],24,43,0,{.42f,.86f,.78f});
       float nearest=10000;for(const auto&b:race.rivals)if(b.rider.s>rider.s)nearest=std::min(nearest,b.rider.s-rider.s);
       if(place>1&&nearest<10000){std::snprintf(timeText,sizeof(timeText),"NEXT RIDER  +%.0f M",nearest);uiCentered(hud,renderer.width,timeText,cx,72,0,{.9f,.92f,.88f});}
     }
     menu.countdown(hud,renderer.width,launch);feedback.draw(hud,renderer.width,runout.active);
   }

   if(!menu.visible()&&paused&&!menu.transitioning)pauseMenu.draw(hud,renderer.width);
   menu.drawTransition(hud,renderer.width);
   if(vertexProbe&&frames==0){Vertex empty{};empty.p={-2,-2,0};empty.material=-1;hud.insert(hud.end(),420000,empty);}

   // Static geometry is resident for the entire descent, including the heightfield.
   // Distant bends are no longer cut off by a route-distance window.
   renderer.worldFirst=renderer.worldCount=0;
   buildSeconds+=double(SDL_GetPerformanceCounter()-buildStart)/frequency;Uint64 drawStart=SDL_GetPerformanceCounter();
   renderer.draw(sky,moving,hud,u,record||shot||(smoke&&capture&&(frames==limit-1||(tourProbe&&(runout.done||stageClock.failed)))));
   if(transitionShot){char name[100];std::snprintf(name,sizeof(name),"captures/pass21-transition-%04d.bmp",frames);std::filesystem::copy_file(outputPath("captures/latest.bmp"),outputPath(name),std::filesystem::copy_options::overwrite_existing);}
   drawSeconds+=double(SDL_GetPerformanceCounter()-drawStart)/frequency;
   if(record){std::filesystem::create_directories(outputPath("captures/pass18-motion"));char name[100];std::snprintf(name,sizeof(name),"captures/pass18-motion/frame-%04d.bmp",frames);std::filesystem::copy_file(outputPath("captures/latest.bmp"),outputPath(name),std::filesystem::copy_options::overwrite_existing);}
   frames++;
   if(!smoke){double work=double(SDL_GetPerformanceCounter()-now)/frequency;double budget=1.0/120-work;if(budget>0)SDL_DelayNS(Uint64(budget*1e9));}
   if(smoke&&frames>=limit)running=false;
   if(tourProbe&&(runout.done||stageClock.failed)&&menu.page==MenuPage::Results&&!menu.transitioning)running=false;
 }
 if(vertexProbe&&renderer.dynamicVertexCapacity()<=400000)throw std::runtime_error("Vertex growth was not exercised");
 if((finishProbe||tourProbe)&&(!runout.done||menu.page!=MenuPage::Results))throw std::runtime_error("Finish integration flow failed");
 if(frontendProbe&&(menu.page!=MenuPage::Ride||rider.pilot!=1||!stageClock.active||road.course!=(frontendDestination>=0?frontendDestination:courseOverride)))
   throw std::runtime_error("Frontend integration flow failed: frames="+std::to_string(frames)+" page="+std::to_string(int(menu.page))+" pilot="+std::to_string(rider.pilot)+" clock="+std::to_string(stageClock.active)+" course="+std::to_string(road.course));
 log<<"Race "<<race.enabled<<" place "<<race.place(rider)<<" difficulty "<<race.difficulty<<" player finish "<<race.playerFinish<<"\n";for(const auto&b:race.rivals)log<<"Rival "<<b.rider.pilot<<" finish "<<b.finish<<" progress "<<b.rider.s<<" crashes "<<b.rider.crashes<<"\n";
 log<<"Course "<<road.course<<"\n";log<<"Awards speed "<<feedback.speedAwards<<" drift "<<feedback.driftAwards<<"\nFinish runout "<<runout.done<<"\n";
 log<<"Dynamic capacity "<<renderer.dynamicVertexCapacity()<<" vertices\n";
 log<<"Pixel grid "<<renderer.pixelWidth<<" x "<<renderer.pixelHeight<<" integer "<<renderer.integerPixels<<"\n";
 log<<"Countdown remaining "<<launch.remaining<<"\n";
 log<<"Menu page "<<int(menu.page)<<"\nPilot "<<rider.pilot<<"\nTrial seconds "<<stageClock.elapsed<<"\n";
 if(pad){SDL_CloseGamepad(pad);}
 log<<"Rider ms/frame "<<riderSeconds*1000/frames<<"\nScenery ms/frame "<<scenerySeconds*1000/frames<<"\nEffects ms/frame "<<effectsSeconds*1000/frames<<"\n";
 log<<"Build ms/frame "<<buildSeconds*1000/frames<<"\nDraw ms/frame "<<drawSeconds*1000/frames<<"\n";
 log<<"Frames "<<frames<<"\nMeasured seconds "<<measuredSeconds<<"\nDistance "<<rider.s<<"\nSpeed "<<rider.speed()<<"\nCrashes "<<rider.crashes<<"\nExit clean\n";std::cout<<"Rendered "<<frames<<" frames on "<<renderer.driver<<"\n";
 }catch(const std::exception&e){std::ofstream log(outputPath("error.log"));log<<e.what();std::cerr<<e.what()<<'\n';if(!smoke)SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR,"SLIDEWILD",e.what(),nullptr);return 1;}
 return 0;
}

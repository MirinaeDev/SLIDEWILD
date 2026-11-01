#pragma once
#include "renderer.hpp"
#include "simulation.hpp"
#include "ui.hpp"
#include "sprite_data.hpp"
#include "nameplate_data.hpp"
#include "launch.hpp"
#include "race.hpp"
#include "route_presentation.hpp"
#include <cstdio>
#include <array>
namespace dh {
struct StageClock {
 float remaining=43,elapsed=0,flash=0;int checkpoint=0,course=0;bool active=false,failed=false,won=false,raceMode=false;
 void begin(int selected=0,bool race=false){*this=StageClock{};course=std::clamp(selected,0,courseCount-1);remaining=float(courses[course].initialTime);active=true;raceMode=race;}
 void tick(const Rider& r,float dt){
   if(!active||failed||won||dt<=0)return;
   elapsed+=dt;if(!raceMode)remaining-=dt;flash=std::max(0.f,flash-dt);
   if(!raceMode&&remaining<=0){remaining=0;failed=true;active=false;return;}
   while(checkpoint<3&&r.s>=float(checkpoint+1)*1000){checkpoint++;remaining+=courses[course].checkpointTime;flash=1.2f;}
   if(r.finished){won=true;active=false;}
 }
};
enum class MenuPage {Title,Pilots,Globe,Briefing,Ride,Results,Route};
struct Frontend {
 int resultChoice=0;
 MenuPage page=MenuPage::Title;int pilot=0,country=0,difficulty=1;bool raceMode=true;
 float time=0,yaw=0,manual=0,orbit=0,pageTime=0,carousel=0;bool dragging=false,startRequested=false;
 RoutePresentation presentation;
 MenuPage destination=MenuPage::Title;float transitionTime=0;bool transitioning=false,switched=false;
 unsigned transitionSerial=0;
 bool busy()const{return transitioning||page==MenuPage::Route;}
 void travel(MenuPage next){
   if(transitioning)return;
   if(next==MenuPage::Results)resultChoice=0;
   destination=next;transitionTime=0;transitioning=true;switched=false;dragging=false;transitionSerial++;
 }
 void drawTransition(Mesh&m,int width)const{
   if(!transitioning)return;
   const V3 ink{.025f,.045f,.075f},teal{.30f,.85f,.78f},gold{1,.65f,.3f};
   for(int row=0;row<9;row++){
     float delay=row*.006f;
     float closing=clamp((transitionTime-delay)/.18f,0,1);
     float opening=clamp((transitionTime-.34f-delay)/.16f,0,1);
     float x=-160-(width+340)*(1-closing*closing*(3-2*closing))+(width+340)*opening*opening*(3-2*opening);
     auto p=[&](float a,float b){return V3{std::round(a)/width*2-1,1-std::round(b)/270,0};};
     float y=row*60.f;
     quad(m,p(x,y),p(x+width+320,y),p(x+width+290,y+61),p(x-30,y+61),ink,-1);
     screenLine(m,width,{x+width+307,y,0},{x+width+277,y+61,0},8,row%2?teal:gold);
     screenLine(m,width,{x+12,y,0},{x-18,y+61,0},3,teal);
   }
   float enter=clamp(transitionTime/.20f,0,1),leave=clamp((transitionTime-.34f)/.20f,0,1);
   float fly=-(1-enter)*(1-enter)*(width+340)+leave*leave*(width+340);
   float center=width*.5f+fly;
   screenLine(m,width,{center-260,310,0},{center+255,230,0},3,teal);
   screenLine(m,width,{center-225,322,0},{center+275,245,0},1,gold);
   uiImage(m,width,center-160,179,320,160,-7);
   uiCentered(m,width,"WORLD TOUR",center,347,0,{1,.94f,.80f});
 }
 static constexpr const char* countries[courseCount]={"VESPERA","SOLMARE","KESTREL","BOREALIS","AZURA","ALTERRA","NOVA","IGNARA"};
 static constexpr const char* biomes[courseCount]={"PASSES / LAKES / HAIRPINS","CANYONS / MESAS / DESERT","CITY / TERRACES / HARBOR","MOORS / CASTLES / RAIN","PALMS / OCEAN / STORMS","BANKING / DIPS / SPEED","SKY TRACK / DIVES / NEON","VOLCANO / GEYSERS / HAZARDS"};
 static constexpr float longitude[courseCount]={0,2.10f,-3.08f,1.26f,-1.15f,-.52f,2.7f,-2.25f};
 static constexpr float latitude[courseCount]={.42f,-.47f,.50f,1.16f,-.20f,-.85f,-.98f,.05f};
 static constexpr V3 accent[4]={{1,.65f,.30f},{.32f,.84f,.80f},{.91f,.62f,.95f},{1,.48f,.62f}};
 bool visible()const{return page!=MenuPage::Ride;}
 void tick(float dt){time+=dt;pageTime+=dt;manual=std::max(0.f,manual-dt);
   if(transitioning){
     transitionTime+=dt;
     if(!switched&&transitionTime>=.28f){page=destination;pageTime=0;switched=true;if(page==MenuPage::Ride)startRequested=true;}
     if(transitionTime>=.56f)transitioning=false;
   }else if(page==MenuPage::Route&&pageTime>=2.7f)travel(MenuPage::Ride);
   float delta=float(pilot)-carousel;while(delta>2)delta-=4;while(delta<-2)delta+=4;
   carousel+=delta*(1-std::exp(-dt*9));
   if(carousel>4)carousel-=4;
   if(carousel<0)carousel+=4;
   if(page==MenuPage::Globe&&!dragging&&manual==0)yaw+=wrap(longitude[country]-yaw)*(1-std::exp(-dt*5));
   if(!dragging&&manual==0)orbit+=dt*.95f;
 }
 void choose(int direction){
   if(busy())return;
   if(page==MenuPage::Pilots){pilot=(pilot+direction+4)%4;orbit=0;pageTime=0;}
   if(page==MenuPage::Globe){country=(country+direction+courseCount)%courseCount;manual=0;}
   if(page==MenuPage::Briefing)raceMode=!raceMode;
   if(page==MenuPage::Results)resultChoice=(resultChoice+direction+4)%4;
 }
 void rotate(float amount){if(busy())return;if(page==MenuPage::Pilots)orbit+=amount;else if(page==MenuPage::Globe)yaw+=amount;else if(page==MenuPage::Briefing)difficulty=(difficulty+(amount>0?1:2))%3;manual=2.5f;}
 void confirm(){
   if(busy())return;
   if(page==MenuPage::Title){travel(MenuPage::Pilots);orbit=0;}
   else if(page==MenuPage::Pilots)travel(MenuPage::Globe);
   else if(page==MenuPage::Globe&&country<courseCount)travel(MenuPage::Briefing);
   else if(page==MenuPage::Briefing)travel(MenuPage::Route);
   else if(page==MenuPage::Results){
     if(resultChoice==0)travel(MenuPage::Route);
     else if(resultChoice==1){country=(country+1)%courseCount;travel(MenuPage::Briefing);}
     else travel(resultChoice==2?MenuPage::Globe:MenuPage::Title);
   }
 }
 void back(){
   if(transitioning)return;
   if(page==MenuPage::Pilots)travel(MenuPage::Title);
   else if(page==MenuPage::Globe)travel(MenuPage::Pilots);
   else if(page==MenuPage::Route)travel(MenuPage::Briefing);
   else if(page==MenuPage::Briefing||page==MenuPage::Results)travel(MenuPage::Globe);
 }
 void click(float x,float y,int width){
   if(busy())return;
   if(page==MenuPage::Results){if(y>=453&&y<501&&x>=32&&x<width-32){resultChoice=std::clamp(int((x-32)/((width-64)/4.f)),0,3);confirm();}return;}
   if(page==MenuPage::Briefing&&x>=40&&x<420&&y>=172&&y<=205){raceMode=x>=220;return;}
   if(page==MenuPage::Briefing&&raceMode&&x>=40&&x<420&&y>=230&&y<278){difficulty=(difficulty+1)%3;return;}
   if(page==MenuPage::Title){if(y>=390&&y<500)confirm();return;}
   if(page==MenuPage::Pilots){
     if(y>=474&&y<518&&x>=width-281&&x<=width-28){confirm();return;}
     float cx=width*.5f;
     if(y>=320&&y<=355){
       if(x<65){choose(-1);return;}
       if(x>width-65){choose(1);return;}
     }
     if(y>150&&y<469){
       if(std::abs(x-cx)<width*.13f){dragging=true;manual=3;return;}
       choose(x<cx?-1:1);return;
     }return;
   }
   if(page==MenuPage::Globe&&x>width*.40f&&y>105&&y<440){
     for(int j=0;j<courseCount;j++){
       float delta=longitude[j]-yaw;if(std::cos(latitude[j])*std::cos(delta)<.04f)continue;
       float px=width*.69f+std::cos(latitude[j])*std::sin(delta)*151,py=274-std::sin(latitude[j])*151;
       if((x-px)*(x-px)+(y-py)*(y-py)<18*18){country=j;manual=0;return;}
     }
     dragging=true;manual=4;return;
   }

   if(page==MenuPage::Globe&&x>=40&&x<=334&&y>=166&&y<446){country=int((y-166)/35);manual=0;return;}
   if(y>=454&&y<500&&x>=40&&x<350)confirm();
 }
 void character(Mesh&m,int width,int who,int frame,float center,float baseline,float height,bool shadow=true)const{
   auto b=selectRects[who][frame%8];auto first=selectRects[who][0];
   float scale=height/((first.y1-first.y0)*1024);
   float w=(b.x1-b.x0)*1536*scale,h=(b.y1-b.y0)*1024*scale;
   if(shadow)uiImage(m,width,center-w*.7f,baseline-8,w*1.4f,22,-12);
   uiImage(m,width,center-w*.5f,baseline-h,w,h,who==3?-14.f:-8.f-who,b.x0,b.y0,b.x1,b.y1);
 }
 void countdown(Mesh&m,int width,const RollingStart& launch)const{
   if(!launch.active()&&launch.go<=0)return;
   const V3 cream{1,.95f,.78f},ink{.035f,.045f,.08f},gold{1,.63f,.29f};
   bool go=!launch.active();std::string word=go?"GO!":std::to_string(int(std::ceil(launch.remaining)));
   float y=go?142:176;
   uiCentered(m,width,word,width*.5f+3,y+4,3,ink);uiCentered(m,width,word,width*.5f,y,3,go?V3{.4f,1,.83f}:cream);
   uiPanel(m,width,width*.5f-152,y+77,304,27,ink,.8f);
   uiCentered(m,width,go?"YOU'RE IN CONTROL!":"ROLLING START - GET READY",width*.5f,y+86,0,cream);
   if(!go)for(int k=0;k<3;k++){
     float x=width*.5f-38+k*28;rect(m,x,y-25,20,6,int(std::ceil(launch.remaining))<=3-k?gold:ink,width);
   }
 }
 Mesh draw(int width,const StageClock& clock,const Race* race=nullptr)const{
   Mesh m;const V3 ink{.03f,.045f,.075f},white{.98f,.95f,.87f},dim{.71f,.77f,.80f},gold{1,.67f,.34f},teal{.33f,.87f,.80f};
   auto txt=[&](const std::string&s,float x,float y,int font,V3 c){uiText(m,width,s,x,y,font,c);};
   if(page==MenuPage::Ride){
     if(raceMode&&race&&race->enabled)return m;
     if(clock.active){
       float cx=width*.5f;V3 tone=clock.remaining<8?V3{1,.35f,.29f}:gold;
       char b[30];std::snprintf(b,sizeof(b),"%02d",int(std::ceil(clock.remaining)));
       uiCentered(m,width,b,cx,15,2,clock.remaining<8?tone:white);
       txt("TIME",cx-72,22,0,dim);txt("SEC",cx+46,22,0,dim);
       for(int j=0;j<3;j++)rect(m,cx-20+j*15,48,10,2,j<clock.checkpoint?teal:dim*.3f,width);
       if(clock.flash>0){
         uiCut(m,width,cx-118,64,236,29,ink,.84f,4);
         uiCentered(m,width,"CHECKPOINT  +"+std::to_string(courses[clock.course].checkpointTime)+" SEC",cx,70,1,teal);
       }
     }return m;
   }
   // Four independently authored plates. Movement is snapped to the final
   // render grid; foreground displacement is much larger than distant peaks.
   float sway=std::sin(time*.085f),rise=std::sin(time*.057f);
   for(int layer=0;layer<4;layer++){
     float depth[4]={1,7,17,34};float drift=depth[layer]*sway;
     float w=std::max(float(width)+88,588.f*1672/941),h=w*941/1672;
     uiImage(m,width,(width-w)*.5f+drift,(540-h)*.5f+rise*depth[layer]*.15f,w,h,-15.f-layer);
   }
   // The painted paddock has a real floor for the rider on both the staging
   // and result screens; never leave a full-body sprite hovering over the lake.
   if(page==MenuPage::Route){presentation.draw(m,width,country,pageTime,raceMode);return m;}
   if(page==MenuPage::Pilots||page==MenuPage::Briefing||page==MenuPage::Results){
     float aw=1672.f/941,ar=float(width)/540,uw=std::min(1.f,ar/aw),vh=std::min(1.f,aw/ar);
     uiImage(m,width,0,0,float(width),540,-6,(1-uw)*.5f,(1-vh)*.5f,(1+uw)*.5f,(1+vh)*.5f);
   }
   if(page==MenuPage::Title){
     float arrive=1-std::pow(1-clamp(pageTime/1.1f,0,1),3);
     float logoW=std::min(580.f,width*.67f);
     uiImage(m,width,(width-logoW)*.5f,58-(1-arrive)*45,logoW,logoW*.5f,-7);
     uiCentered(m,width,"WORLD TOUR",width*.5f,337,1,gold);
     V3 pulse=std::fmod(time,1.4f)<.95f?white:gold;
     uiCentered(m,width,"PRESS START",width*.5f,415,2,pulse);
     uiCentered(m,width,"ENTER / A",width*.5f,451,0,white);
     uiCentered(m,width,"DAMIEN MIRI",width*.5f,518,0,dim*.7f);
     return m;
   }
   if(page==MenuPage::Pilots){
     // Elliptical turntable: real depth order and continuous shortest rotation.
     // Every drawing uses the measured native sprite aspect, never a fixed box.
     uiImage(m,width,24,15,154,77,-7);
     uiCentered(m,width,"SELECT YOUR RIDER",width*.5f,24,1,white);
     uiSteps(m,width,0,width-276,22,accent[pilot]);
     const char* style[4]={"THE ALL-ROUNDER","THE TECHNICIAN","THE SPEED SPECIALIST","THE FLOW RIDER"};
     const int bars[4][3]={{6,6,6},{5,8,8},{8,5,5},{6,7,6}};
     const char* labels[3]={"SPEED","CARVING","BRAKING"};
     for(int j=0;j<80;j++)uiPanel(m,width,0,460+j,float(width),1,ink,.10f+j*.0081f);
     for(int j=0;j<3;j++){
       txt(labels[j],37,468+j*13,0,white);
       for(int k=0;k<9;k++){
         for(int line=0;line<5;line++)rect(m,111+k*13-line,471+j*13+line,8,1,k<bars[pilot][j]?accent[pilot]:dim*.18f,width);
       }
     }
     float cx=width*.5f,rx=std::min(340.f,width*.34f);
     for(int j=0;j<144;j++){
       float a=j*2*pi/144,depth=(std::cos(a)+1)*.5f;
       if(j%3==0)continue;
       rect(m,std::round(cx+std::sin(a)*rx),std::round(368+std::cos(a)*76),3,depth>.5f?2.f:1.f,accent[pilot]*(.22f+depth*.4f),width);
     }
     struct Slot{int who;float x,base,height,depth,angle;};std::array<Slot,4> slots;
     for(int who=0;who<4;who++){
       float a=(who-carousel)*pi*.5f,depth=(std::cos(a)+1)*.5f;
       slots[who]={who,cx+std::sin(a)*rx+140*std::pow(1-depth,3.f),310+134*depth,180+132*depth,depth,a};
     }
     std::sort(slots.begin(),slots.end(),[](const Slot&a,const Slot&b){return a.depth<b.depth;});
     int frame=(int(std::floor(orbit/(pi*.25f)))%8+8)%8;
     for(const auto& slot:slots){
       int facing=slot.who==pilot?frame:(std::sin(slot.angle)<0?1:7);
       character(m,width,slot.who,facing,slot.x,slot.base,slot.height);
       if(slot.who!=pilot&&slot.depth>.25f)uiCentered(m,width,pilots[slot.who].name,slot.x,slot.base+12,1,accent[slot.who]);
     }
     auto nb=nameplateRects[pilot];float nw=48*(nb.x1-nb.x0)/(nb.y1-nb.y0)*1.5f;
     uiImage(m,width,cx-nw*.5f,70,nw,48,-20,nb.x0,nb.y0,nb.x1,nb.y1);
     uiCentered(m,width,style[pilot],cx,120,0,accent[pilot]);
     for(int d:{-1,1})for(int n=0;n<3;n++)uiChevron(m,width,d<0?35.f-n*9:width-35.f+n*9,332,d,accent[pilot]*(1-n*.18f));
     for(int j=0;j<4;j++){
       float x=cx-30+j*20;rect(m,x,455,11,3,j==pilot?accent[pilot]:dim*.35f,width);
       if(j==pilot)rect(m,x+3,452,5,3,white,width);
     }
     uiCentered(m,width,"CHOOSE YOUR STYLE",cx,479,1,white);
     uiCentered(m,width,"4 RIDERS / WORLD TOUR",cx,500,0,dim);
     uiButton(m,width,"ENTER / A   SELECT",width-281,475,253,accent[pilot]);
     float hint=28;hint+=uiHint(m,width,"< >","RIDER",hint,519,dim);
     hint+=uiHint(m,width,"Q E","ROTATE",hint,519,dim);
     uiHint(m,width,"ESC","BACK",hint,519,dim);
     txt("Q / E : 360 VIEW",width-169,522,0,dim);
     return m;
   }
   // Small backlit motes, separate from the painting, use the logical pixel grid.
   for(int j=0;j<22;j++){
     float px=std::fmod(j*137.3f+time*(5+j%3),float(width)),py=110+std::fmod(j*61.f-time*3+10000.f,330.f);
     uiPanel(m,width,std::round(px),std::round(py),j%4==0?2.f:1.f,1,gold,.28f+.25f*std::sin(time+j));
   }
   for(int j=0;j<72;j++)uiPanel(m,width,0,j,float(width),1,ink,.88f-j*.0094f);
   uiPanel(m,width,0,511,float(width),29,ink,.84f);
   uiImage(m,width,26,5,112,55,-7);
   txt("WORLD TOUR",154,18,1,white);txt("ASTER / LAND OF RIDERS",155,42,0,dim);
   if(page==MenuPage::Results)txt("SESSION / COMPLETE",width-200,27,0,clock.won?teal:gold);
   else uiSteps(m,width,page==MenuPage::Globe?1:2,width-276,22,gold);
   if(page==MenuPage::Globe){
     txt("DESTINATION",47,105,2,white);txt("CHOOSE YOUR PLAYGROUND",48,144,0,gold);
     for(int j=0;j<courseCount;j++){
       float y=166+j*35;
       if(j==country){
         float pulse=.8f+.2f*std::sin(time*4);
         rect(m,58,y+34,229,2,gold*pulse,width);
         rect(m,58+std::fmod(time*75,220.f),y+33,9,3,white,width);
         uiChevron(m,width,39+std::round(std::sin(time*5)*3),y+15,1,gold);
       }
       else rect(m,57,y+34,261,1,dim*.17f,width);
       txt(countries[j],58,y+3,1,j==country?white:dim);
       txt(j<courseCount?courses[j].name:"COMING SOON / EXPLORE",58,y+21,0,j<courseCount?teal:dim*.72f);
       if(j==country)uiChevron(m,width,314,y+15,1,gold);
     }
     float cx=width*.69f,cy=274,radius=151;
     uiImage(m,width,cx-radius-8,cy-radius-8,radius*2+16,radius*2+16,-12);
     uiImage(m,width,cx-radius,cy-radius,radius*2,radius*2,-4,0,0,1,1,{yaw,0,0});
     for(int j=0;j<courseCount;j++){
       float delta=longitude[j]-yaw,front=std::cos(latitude[j])*std::cos(delta);if(front<.04f)continue;
       float x=cx+std::cos(latitude[j])*std::sin(delta)*radius,y=cy-std::sin(latitude[j])*radius;
       rect(m,std::round(x)-5,std::round(y)-5,11,11,ink,width);rect(m,std::round(x)-3,std::round(y)-3,7,7,j==country?gold:teal,width);
       uiPanel(m,width,x+11,y-8,uiTextWidth(countries[j])+12,23,ink,.86f);txt(countries[j],x+17,y-2,0,j==country?white:dim);
     }
     uiCentered(m,width,biomes[country],cx,453,0,white);
     uiCentered(m,width,"SPIN THE GLOBE",cx,474,0,dim);
     uiButton(m,width,country<courseCount?"ENTER  /  VIEW COURSE":"DESTINATION LOCKED",44,462,290,country<courseCount?teal:dim);
   }else if(page==MenuPage::Briefing){
     txt(courses[country].name,48,90,2,white);txt(std::string(courses[country].country)+" / 4.0 KM / "+std::to_string(courses[country].turns)+" TURNS",49,139,0,gold);
     txt("TIME ATTACK",59,181,1,raceMode?dim:gold);txt("VS RIVALS",235,181,1,raceMode?gold:dim);
     rect(m,raceMode?232.f:56.f,205,raceMode?135.f:117.f,3,gold,width);
     if(raceMode){
       txt(Race::difficultyNames[difficulty],59,244,2,gold);
       txt("3 RIVALS / AIM FOR FIRST PLACE",59,286,0,teal);
       txt("Q / E  OR LB / RB : DIFFICULTY",59,309,0,dim);
     }else{
       txt(std::to_string(courses[country].initialTime)+" S",59,245,2,gold);txt("START TIME",149,254,0,white);
       txt("+"+std::to_string(courses[country].checkpointTime)+" S / CHECKPOINT - EVERY 1000 M",59,286,0,teal);
     }
     txt("ROLLING START",49,330,1,white);txt("3, 2, 1... TAKE CONTROL ON THE MOVE.",49,359,0,dim);
     txt(std::string(country==0?"ABOUT 2 MIN / ":"SPRINT / ")+courses[country].finish,49,389,0,dim);
     // Route milestones expose the actual three time gates and finish.
     rect(m,62,422,294,1,dim*.36f,width);
     for(int j=0;j<4;j++){
       rect(m,60+j*98,419,5,5,j==3?gold:teal,width);
       if(j<3)rect(m,60+j*98,418,5,1,white,width);
     }
     float cx=width*.72f;character(m,width,pilot,1,cx,439,295);
     auto nb=nameplateRects[pilot];float nw=48*(nb.x1-nb.x0)/(nb.y1-nb.y0)*1.5f;
     uiImage(m,width,cx-nw*.5f,81,nw,48,-20,nb.x0,nb.y0,nb.x1,nb.y1);
     uiButton(m,width,"ENTER  /  LET'S RIDE!",44,462,290,gold);
   }else if(page==MenuPage::Results){
     int place=1;if(race)for(const auto& rival:race->rivals)if(rival.finish>=0&&(race->playerFinish<0||rival.finish<race->playerFinish))place++;
     bool victory=clock.won&&(!raceMode||(race&&race->playerFinish>=0&&place==1));
     auto panelPoint=[&](float x,float y){return V3{x/width*2-1,1-y/270,0};};
     uiPanel(m,width,32,91,530,333,ink,.65f);
     quad(m,panelPoint(32,91),panelPoint(610,91),panelPoint(580,144),panelPoint(32,144),ink,-1);
     rect(m,32,91,4,53,victory?teal:gold,width);
     txt(victory?"VICTORY!":clock.failed?"TIME UP":"REMATCH?",51,99,3,victory?teal:gold);
     txt(victory?"THE DESCENT IS YOURS.":clock.failed?"GAME OVER / OUT OF TIME.":"RACE COMPLETE / CHASE THE TOP SPOT.",53,153,0,white);
     txt(std::string(courses[country].country)+" / "+courses[country].name,53,175,0,dim);
     char b[50];int centiseconds=int(std::round(clock.elapsed*100));
     std::snprintf(b,sizeof(b),"%02d:%02d.%02d",centiseconds/6000,(centiseconds/100)%60,centiseconds%100);
     if(raceMode&&race){
       struct Row{int pilot;float time,s;bool player;};std::array<Row,4> rows;
       rows[0]={pilot,race->playerFinish,Road::finishDistance,true};
       for(int j=0;j<3;j++)rows[j+1]={race->rivals[j].rider.pilot,race->rivals[j].finish,race->rivals[j].rider.s,false};
       std::sort(rows.begin(),rows.end(),[](const Row&a,const Row&b){if(a.time>=0&&b.time>=0)return a.time<b.time;if((a.time>=0)!=(b.time>=0))return a.time>=0;return a.s>b.s;});
       for(int j=0;j<4;j++){
         char line[64];if(rows[j].time>=0)std::snprintf(line,sizeof(line),"%d  %s%s  %05.2f S",j+1,pilots[rows[j].pilot].name,rows[j].player?" / YOU":"",rows[j].time);
         else std::snprintf(line,sizeof(line),"%d  %s  DNF",j+1,pilots[rows[j].pilot].name);
         if(rows[j].player){rect(m,43,219+j*39,3,26,gold,width);rect(m,53,249+j*39,480,1,gold*.5f,width);}
         txt(line,53,224+j*39,1,rows[j].player?gold:white);
       }
       txt(Race::difficultyNames[difficulty],53,403,0,teal);
     }else{
       txt(b,50,217,3,white);txt(clock.won?"YOUR TIME / 4.0 KM":"ELAPSED TIME / DID NOT FINISH",53,292,0,gold);
       txt(clock.won?"KEEP YOUR FLOW. BEAT YOUR BEST TIME.":"BRAKE EARLY. KEEP THE CLOCK ALIVE.",53,342,0,white);
       txt("EVERY FALL COUNTS. EVERY CORNER TOO.",53,379,0,dim);
     }
     character(m,width,pilot,0,width*.79f,449,306);
     const char* actions[]={"RACE AGAIN","NEXT COURSE","SELECT COURSE","TITLE SCREEN"};
     float slot=(width-64)/4.f;
     for(int j=0;j<4;j++){
       float x=32+j*slot;bool selected=j==resultChoice;
       uiPanel(m,width,x,453,slot-10,47,ink,.93f);
       rect(m,x,498,slot-10,selected?3:1,selected?gold:teal*.35f,width);
       uiText(m,width,selected?">":"",x+8,469,0,gold);
       uiText(m,width,actions[j],x+24,469,0,selected?white:dim);
     }
   }
   float hint=32;if(page==MenuPage::Globe||page==MenuPage::Briefing||page==MenuPage::Results)hint+=uiHint(m,width,"< >",page==MenuPage::Briefing?"MODE":"SELECT",hint,518,dim);
   hint+=uiHint(m,width,"ENTER / A","CONFIRM",hint,518,gold);
   uiHint(m,width,"ESC / B","BACK",hint,518,dim);
   if(width>900)txt("FREE PLAY / WORLD TOUR",width-209,522,0,gold);
   return m;
 }
};
}

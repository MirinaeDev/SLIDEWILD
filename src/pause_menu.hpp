#pragma once
#include "ui.hpp"
namespace dh {
enum class PauseAction {None,Resume,Restart,Abandon};
struct PauseMenu {
 int selection=0;bool confirming=false,accept=false;
 void reset(){selection=0;confirming=false;accept=false;}
 void choose(int direction){if(confirming)accept=!accept;else selection=(selection+direction+3)%3;}
 PauseAction back(){if(confirming){confirming=false;accept=false;return PauseAction::None;}reset();return PauseAction::Resume;}
 PauseAction confirm(){
   if(!confirming){if(selection==0)return PauseAction::Resume;confirming=true;accept=false;return PauseAction::None;}
   if(!accept){confirming=false;return PauseAction::None;}
   return selection==1?PauseAction::Restart:PauseAction::Abandon;
 }
 PauseAction click(float x,float y,int width){
   float left=width*.5f-235;
   if(x<left||x>left+470)return PauseAction::None;
   int row=int((y-252)/48);
   if(y<252||row>=(confirming?2:3))return PauseAction::None;
   if(confirming)accept=row==1;else selection=row;
   return confirm();
 }
 void draw(Mesh&m,int width)const{
   const V3 gold{1,.72f,.35f},teal{.35f,.86f,.80f},white{.96f,.95f,.88f},ink{.035f,.05f,.08f};
   float cx=width*.5f,left=cx-235;
   uiPanel(m,width,0,0,float(width),540,ink,.67f);
   auto p=[&](float x,float y){return V3{x/width*2-1,1-y/270,0};};
   quad(m,p(left-40,134),p(left+512,134),p(left+470,422),p(left-82,422),ink,-1);
   for(int i=0;i<5;i++)rect(m,left-33+i*18,131,11,3,teal,width);
   rect(m,left,420,470,2,teal,width);
   uiText(m,width,"SESSION PAUSED",left,158,0,teal);
   uiText(m,width,confirming?(selection==1?"RESTART?":"LEAVE RACE?"):"TAKE A BREATHER.",left,181,2,white);
   uiText(m,width,confirming?"YOUR CURRENT RACE PROGRESS WILL BE LOST.":"THE CLOCK AND YOUR RIVALS CAN WAIT.",left,230,0,gold);
   const char* labels[]={"RESUME DESCENT","RESTART RACE","SELECT ANOTHER COURSE"};
   for(int i=0;i<(confirming?2:3);i++){
     bool selected=confirming?(int(accept)==i):selection==i;float y=260+i*48.f;
     if(selected){rect(m,left-12,y-3,3,29,gold,width);rect(m,left,y+30,438,1,gold*.45f,width);}
     uiText(m,width,selected?">":" ",left,y,1,gold);
     uiText(m,width,confirming?(i==0?"NO, STAY HERE":"YES, CONFIRM"):labels[i],left+27,y,1,selected?white:teal*.7f);
   }
   uiCentered(m,width,"UP / DOWN : SELECT    ENTER / A : CONFIRM",cx,451,0,white);
   uiCentered(m,width,"ESC / B : BACK",cx,475,0,teal);
 }
};
}

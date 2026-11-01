#include "simulation.hpp"
#include "music.hpp"
#include <iostream>
#include <stdexcept>
using namespace dh;
void check(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
int main(){try{
 Rider rider;rider.lean=.20f;
 for(int i=0;i<120;i++)rider.animatePose(1.f/120);
 check(rider.pose==0,"Small sustained corrections should keep the neutral pose");
 rider.lean=.9f;rider.animatePose(1.f/120);
 check(rider.pose==0,"A steering tap must not immediately snap to a turning pose");
 for(int i=0;i<80;i++)rider.animatePose(1.f/120);
 check(poseLevel(rider.pose)==3&&poseDirection(rider.pose)==1,"Committed turns still reach all three painted levels");
 rider.lean=0;for(int i=0;i<120;i++)rider.animatePose(1.f/120);
 check(rider.pose==0,"Releasing steering returns to neutral without visual chatter");
 check(windRate(0)==0&&windRate(12)<windRate(30)&&windRate(30)<windRate(58),"Shirt frequency must increase with speed and stop at rest");
 MusicMixer music;music.menu={.5f,-.5f,.25f,-.25f};music.course={-.4f,.4f};
 for(int i=0;i<100000;i++)music.next(true,false,false);
 check(music.menuFrame<2&&music.next(true,false,false)[0]>0,"Menu music loops and preserves stereo");
 auto before=music.next(true,false,false);auto transition=music.next(false,false,false);
 check(std::abs(before[0]-transition[0])<.3f,"Track switch must crossfade rather than cut");
 for(int i=0;i<100000;i++)music.next(false,false,false);
 check(music.next(false,false,false)[0]<-.12f,"Race mode selects the course track");
 for(int i=0;i<100000;i++)music.next(false,false,true);
 check(std::abs(music.next(false,false,true)[0])<.04f,"Pause ducks race music");
 for(int i=0;i<150000;i++)music.next(false,true,false);
 check(std::abs(music.next(false,true,false)[0])<.00001f,"Mute fades both tracks to silence");
 std::cout<<"PASS gentle steering poses, speed-driven shirt animation, stereo music looping, crossfade, pause and mute\n";
 }catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}}

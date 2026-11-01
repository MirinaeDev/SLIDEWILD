#include "race.hpp"
#include <iostream>
#include <stdexcept>
using namespace dh;
void check(bool b,const char*message){if(!b)throw std::runtime_error(message);}
int main(){try{
 std::array<std::array<float,3>,courseCount> fastest{};
 for(int course=0;course<courseCount;course++)for(int level=0;level<3;level++){
   Road road(course);Rider player;player.reset(road);player.pace=22;Race race;race.reset(road,player,true,level);
   float first=race.rivals[0].rider.s;race.tick(road,player,0,false,0);check(race.rivals[0].rider.s==first,"Pause advances opponents");
   float brakeTime=0,slideTime=0,slowTime=0;
   for(int frame=0;frame<30000;frame++){
     float wet=courseRain(course,player.s);auto input=demoInput(road,player,wet);
     if(input.brake>.1f)brakeTime+=1.f/120;
     if(input.slide)slideTime+=1.f/120;
     float oldPace=player.pace;player.tick(road,input,wet,1.f/120);race.tick(road,player,0,false,1.f/120);
     if(player.pace<oldPace&&!player.crashed&&(input.brake>.1f||player.drift>.2f))slowTime+=1.f/120;
     bool done=player.finished;for(const auto&b:race.rivals)done&=b.finish>=0;
     if(done)break;
   }
   check(player.finished&&race.playerFinish>0,"Player cannot complete race");
   for(const auto&b:race.rivals){check(b.finish>0,"Rival cannot finish");check(b.rider.pilot!=player.pilot,"Duplicate player avatar");check(b.rider.crashes<5,"Opponent repeatedly crashes");}
   check(race.place(player)>=1&&race.place(player)<=4,"Invalid rank");
   std::cout<<"PASS course "<<course<<" level "<<level<<" player "<<race.playerFinish<<" rank "<<race.place(player)<<" brake "<<brakeTime<<" slide "<<slideTime;
   for(const auto&b:race.rivals)std::cout<<" rival "<<b.finish<<" crashes "<<b.rider.crashes;
   fastest[course][level]=std::min({race.rivals[0].finish,race.rivals[1].finish,race.rivals[2].finish});
   std::cout<<'\n';
   if(course>=2&&course<=4)check(slowTime>2&&slideTime>12,"Technical course lacks actual speed scrubbing and slide demands");
   if(course==5)check(brakeTime>3&&slideTime>12,"Gravity Park lacks braking zones between its banked sweepers");

 }
 for(int course=0;course<courseCount;course++){
   check(fastest[course][1]<fastest[course][0]-.5f&&fastest[course][2]<fastest[course][1]-.5f,"Race difficulties do not increase the competitive pace");
   // A clean, assertive driver using only legal player inputs can stay in
   // contention with Expert. Faster rivals must not become uncatchable bots.
   Road track(course);Rider contender;contender.reset(track);contender.pace=22;
   Race expert;expert.reset(track,contender,true,2);float lane=0;
   for(int step=0;step<30000&&!contender.finished;step++){
     float k=wrap(track.at(contender.s+12).heading-track.at(contender.s+10).heading)/2;
     float apex=std::copysign(1.8f*clamp(std::abs(k)/.028f,0,1),k);
     lane=approach(lane,mix(-.75f,apex,clamp(std::abs(k)*75,0,1)),2,1.f/120);
     float wet=courseRain(course,contender.s);
     auto input=racingInput(track,contender,wet,lane,2,0);
     contender.tick(track,input,wet,1.f/120);expert.tick(track,contender,0,false,1.f/120);
   }
   check(contender.finished&&contender.crashes==0,"Assertive legal player cannot drive the course cleanly");
   expert.complete(track,contender,0);
   float fastestExpert=std::min({expert.rivals[0].finish,expert.rivals[1].finish,expert.rivals[2].finish});
   std::cout<<"Competitive check "<<course<<" player "<<expert.playerFinish<<" leader "<<fastestExpert<<std::endl;
   check(expert.playerFinish<=fastestExpert*1.06f,"Expert pace is beyond a clean legal reference driver");
   std::cout<<"PASS competitive pace course "<<course<<": Amateur/Arcade/Expert "<<fastest[course][0]<<'/'<<fastest[course][1]<<'/'<<fastest[course][2]<<", assertive player "<<expert.playerFinish<<", Expert leader "<<fastestExpert<<'\n';
 }
 Road road;Rider player;player.reset(road);Race race;race.reset(road,player,true,1);
 race.playerFinish=100;race.rivals[0].finish=99;race.rivals[1].finish=101;race.rivals[2].finish=-1;
 check(race.place(player)==2,"Finish order changes after finish");
 player.finished=true;race.rivals[2].rider.s=3990;race.rivals[2].rider.p=road.surface(3990,0);race.elapsed=101;
 race.complete(road,player,0);check(race.rivals[2].finish>0&&race.playerFinish==100&&race.place(player)==2,"Result completion corrupts finish order");
 race.reset(road,player,false,1);auto before=race.rivals[0].rider.s;race.tick(road,player,0,false,1);check(before==race.rivals[0].rider.s,"Chrono mode simulates opponents");
 std::cout<<"PASS pause, disabled mode, restart and frozen finish order\n";
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}

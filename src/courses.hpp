#pragma once
#include <algorithm>
namespace dh {
struct CourseInfo {const char* name;const char* country;const char* finish;int turns;int initialTime;int checkpointTime;};
inline constexpr int courseCount=8;
inline constexpr CourseInfo courses[courseCount]={
 {"LAKESIDE RIDGE","VESPERA","LAKESIDE PARKING",10,47,29},
 {"RED CANYON","SOLMARE","MESA OUTPOST",12,32,22},
 {"THE DOCKS","KESTREL","HARBOR PLAZA",20,40,33},
 {"NORTHERN MOORS","BOREALIS","LIGHTHOUSE HARBOR",18,40,33},
 {"AZURA COAST","AZURA","TRADEWIND BEACH",18,40,43},
 {"GRAVITY PARK","ALTERRA","GRAVITY PARK BASE",16,36,27},
 {"NEON DESCENT","NOVA","CLOUD TERMINAL",16,38,26},
 {"CALDERA RUN","IGNARA","EMBER HARBOR",18,40,28}
};
inline float courseRain(int course,float distance){
 if(course==3)return 1;
 if(course!=4)return 0;
 // Clouds gather inland, then a squall crosses the coast. The wet finale
 // keeps reduced grip after the rain starts to ease.
 float arrival=std::clamp((distance-1000)/1050,0.f,1.f);
 float departure=std::clamp((distance-3400)/600,0.f,1.f);
 return arrival*arrival*(3-2*arrival)*(1-.28f*departure);
}
}

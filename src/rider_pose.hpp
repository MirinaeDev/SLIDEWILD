#pragma once
#include <algorithm>
namespace dh {
inline constexpr int riderPoseCount=13;
inline int poseDirection(int pose){return pose==1||pose==3||pose==7||pose==9||pose==11?-1:pose==2||pose==4||pose==8||pose==10||pose==12?1:0;}
inline int poseLevel(int pose){return pose>=11?5:pose==3||pose==4?4:pose==9||pose==10?3:pose==7||pose==8?2:pose==1||pose==2?1:0;}
inline int turningPose(int side,int level){return level==5?(side<0?11:12):level==4?(side<0?3:4):level==3?(side<0?9:10):level==2?(side<0?7:8):(side<0?1:2);}
inline bool usesTurnAtlas(int pose){return pose==1||pose==2||pose>=7;}
// Accumulate this rate in simulation time: changing speed must never jump cels.
inline float windRate(float speed){return std::clamp((speed-3.f)*.16f,0.f,9.f);}
inline int windFrame(float phase,float speed){int cycle=speed>3?int(phase)%4:0;return cycle==3?1:cycle;}
}

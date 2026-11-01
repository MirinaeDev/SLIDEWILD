#pragma once
#include "simulation.hpp"
namespace dh {
struct CameraReaction {float lateral=0,vertical=0,roll=0,pitch=0;};
// Absolute simulation time makes paused frames stable and playback independent
// of render rate. Amplitudes are metres/radians, with separate event envelopes.
inline CameraReaction cameraReaction(const Rider& r){
 float t=r.runTime;
 float fast=clamp((r.speed()-29)/14,0,1);fast*=fast;
 float slide=r.crashed?0:r.drift;
 float hit=r.crashed?std::exp(-r.crashTime*7):0;
 float contact=r.edgeImpact;
 CameraReaction c;
 c.lateral=fast*(std::sin(t*39)+.4f*std::sin(t*67))*.006f
           +slide*std::sin(t*48)*.008f+hit*std::sin(r.crashTime*57)*.095f;
 c.vertical=fast*(std::sin(t*51)+.35f*std::sin(t*83))*.009f
            +slide*(std::sin(t*63)*.009f+.005f*std::sin(t*29))
            +hit*std::cos(r.crashTime*49)*.10f;
 c.roll=fast*std::sin(t*31)*.0018f+slide*std::sin(t*37)*.0028f
        +hit*r.crashSide*std::cos(r.crashTime*34)*.034f;
 c.pitch=hit*.06f+contact*.008f;
 return c;
}
}

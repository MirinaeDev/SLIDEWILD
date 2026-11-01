#pragma once
#include <array>
#include <vector>
#include <cstddef>
namespace dh {
// Immutable decoded stereo tracks. All cursors and envelopes belong to the audio thread.
struct MusicMixer {
 std::vector<float> menu,course;
 std::size_t menuFrame=0,courseFrame=0;
 float menuGain=0,courseGain=0;
 std::array<float,2> next(bool inMenu,bool muted,bool paused){
   float targetMenu=!muted&&inMenu?.38f:0.f;
   float targetCourse=!muted&&!inMenu?(paused?.09f:.32f):0.f;
   // Roughly 0.65 seconds for a smooth menu/course crossfade at 48 kHz.
   menuGain+=(targetMenu-menuGain)*.00010f;
   courseGain+=(targetCourse-courseGain)*.00010f;
   std::array<float,2> sample{};
   auto add=[&](const std::vector<float>& track,std::size_t& frame,float volume,bool active){
     if(track.size()<2||(!active&&volume<.00001f))return;
     auto count=track.size()/2;
     if(frame>=count)frame=0;
     for(int channel=0;channel<2;channel++)sample[channel]+=track[frame*2+channel]*volume;
     frame=(frame+1)%count;
   };
   add(menu,menuFrame,menuGain,inMenu&&!muted);
   add(course,courseFrame,courseGain,!inMenu&&!muted);
   return sample;
 }
};
}

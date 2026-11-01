#pragma once
#include <SDL3/SDL.h>
#include <atomic>
#include <algorithm>
#include <cmath>
#include "runtime_paths.hpp"
#include "music.hpp"
namespace dh {
// Authored stereo music, mixed with procedural wheel / wind / slide layers.
class RideSound {
 SDL_AudioStream* stream=nullptr;
 std::atomic<float> speed{0},slide{0},scrape{0},roughness{0},water{0},thunder{0};
 std::atomic<bool> menuMode{false},muted{false},paused{false};std::atomic<int> cueRequest{0};
 MusicMixer music;
 double musicTime=0;float cueAge=1;int cueKind=0;
 float filter=0,rumble=0,phase=0,gain=0;unsigned rng=0x1234abcd;
 static void SDLCALL callback(void*user,SDL_AudioStream*stream,int amount,int){
  auto& s=*static_cast<RideSound*>(user);float samples[2048];
  int cue=s.cueRequest.exchange(0);if(cue){s.cueKind=cue;s.cueAge=0;}
  bool menu=s.menuMode.load(),mute=s.muted.load(),pause=s.paused.load();
  float v=s.speed.load(),drift=s.slide.load(),edge=s.scrape.load(),rough=s.roughness.load(),water=s.water.load();
  while(amount>0){int n=std::min(1024,amount/int(2*sizeof(float)));if(n<=0)break;
   for(int i=0;i<n;i++){
    s.rng^=s.rng<<13;s.rng^=s.rng>>17;s.rng^=s.rng<<5;
    float noise=float(s.rng&65535)/32768.f-1;
    s.filter+=(noise-s.filter)*.065f;s.gain+=(std::min(v/32.f,1.f)-s.gain)*.002f;
    s.phase+=.0008f+v*.00013f;if(s.phase>1)s.phase-=1;
    float wheel=std::sin(s.phase*6.2831853f)*.007f;
    float effect=s.gain*(s.filter*.16f+wheel+noise*(.025f+drift*.045f+edge*.07f+rough*.045f+water*.05f));
    s.rumble+=(noise-s.rumble)*.008f;
    effect+=s.thunder.load()*(s.rumble*.65f+std::sin(float(s.musicTime*241))*.009f);
    s.musicTime+=1./48000;
    if(s.cueAge<.18f){float f=s.cueKind==3?1320.f:s.cueKind==2?880.f:660.f;
      if(s.cueKind==4)effect+=(noise-s.filter)*.075f*std::sin(s.cueAge/.18f*3.1415927f);
      else effect+=std::sin(s.cueAge*f*6.2831853f)*.10f*std::exp(-s.cueAge*22);
      s.cueAge+=1.f/48000;}
    auto song=s.music.next(menu,mute,pause);
    for(int channel=0;channel<2;channel++)samples[i*2+channel]=std::clamp(song[channel]+(mute?0.f:effect),-1.f,1.f);
   }
   SDL_PutAudioStreamData(stream,samples,n*2*sizeof(float));amount-=n*2*sizeof(float);
  }
 }
public:
 static std::vector<float> loadMusic(const char* name){
   SDL_AudioSpec original{};Uint8* bytes=nullptr;Uint32 size=0;
   auto path=pathUtf8(resourcePath(std::string("assets/music/")+name));
   if(!SDL_LoadWAV(path.c_str(),&original,&bytes,&size)){SDL_Log("Music %s: %s",name,SDL_GetError());return {};}
   SDL_AudioSpec target{};target.format=SDL_AUDIO_F32;target.channels=2;target.freq=48000;
   Uint8* converted=nullptr;int length=0;
   bool ok=SDL_ConvertAudioSamples(&original,bytes,int(size),&target,&converted,&length);SDL_free(bytes);
   if(!ok){SDL_Log("Music conversion %s: %s",name,SDL_GetError());return {};}
   const float* first=reinterpret_cast<const float*>(converted);
   std::vector<float> result(first,first+length/sizeof(float));SDL_free(converted);return result;
 }
 explicit RideSound(bool enabled){if(!enabled||!SDL_InitSubSystem(SDL_INIT_AUDIO))return;
   music.menu=loadMusic("menu.wav");music.course=loadMusic("course.wav");
   SDL_AudioSpec spec{};spec.format=SDL_AUDIO_F32;spec.channels=2;spec.freq=48000;stream=SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK,&spec,callback,this);if(stream)SDL_ResumeAudioStreamDevice(stream);}
 ~RideSound(){if(stream)SDL_DestroyAudioStream(stream);}
 void update(float v,float d,float e,bool quiet,float rough=0,float wet=0,bool mute=false){muted.store(mute);paused.store(quiet);roughness.store(rough);water.store(wet);speed.store(quiet||mute?0:v);slide.store(d);scrape.store(e);}
 bool musicAvailable()const{return !music.menu.empty()&&!music.course.empty();}
 bool available()const{return stream!=nullptr;}
 void menu(bool enabled){menuMode.store(enabled);}
 void cue(int kind=1){cueRequest.store(kind);}
 void storm(float intensity){thunder.store(intensity);}
};
}

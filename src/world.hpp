#pragma once
#include "renderer.hpp"
#include "simulation.hpp"
#include "effects.hpp"
namespace dh {
struct StaticWorld { Mesh vertices;std::vector<Uint32> segmentOffsets; };
StaticWorld buildWorld(const Road& road);
float sceneryHeight(V3 p);
bool sceneryPlantClear(V3 p,float radius);
Mesh buildRider(const Rider& rider,const Road& road,V3 camera,bool tuck,bool slide);
void appendTrees(Mesh& mesh,const Road& road,const Rider& rider,V3 camera);
void appendEffects(Mesh& mesh,const RideEffects& effects,V3 camera,const Rider& rider,const Road& road);
Mesh buildSky(int width,float wet,float cameraYaw);
Mesh buildHud(int width,const Rider& rider,const Road& road,float wet,bool paused,bool demo,float fps,bool help);
}

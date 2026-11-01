#pragma once
#include "math.hpp"
namespace dh {
struct PilotTraits {const char* name;const char* style;float steering,drag,brake;V3 color;};
inline constexpr PilotTraits pilots[4]={
 {"SOL","ALL-ROUNDER",1,1,1,{1,.95f,.84f}},
 {"RIO","CARVING",1.06f,1.06f,1.08f,{.40f,.83f,.83f}},
 {"NOX","SPEED",.95f,.94f,.96f,{.97f,.51f,.35f}},
 {"MEI","FLOW",1.025f,1.025f,1.03f,{.95f,.48f,.68f}}
};
}

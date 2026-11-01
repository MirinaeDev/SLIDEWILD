#pragma once
#include "math.hpp"
namespace dh {
// Both the world projection and the sky rays are derived from this one frame.
// Screen Y points down; camera/view Y points up. Mixing those conventions made
// the panorama counter-roll while the road and buildings rolled together.
struct CameraProjection {
 Mat matrix;V3 rayX,rayY,forward;
 V3 ray(float u,float v)const{return norm(forward+rayX*(u*2-1)+rayY*(v*2-1));}
};
inline CameraProjection cameraProjection(V3 eye,V3 target,float fov,float aspect,float angle){
 float c=std::cos(angle),s=std::sin(angle),scale=std::tan(fov*.5f);
 V3 forward=norm(target-eye),right=norm(cross({0,1,0},forward)),up=cross(forward,right);
 Mat roll=identity();roll.m[0]=roll.m[5]=c;roll.m[1]=-s/aspect;roll.m[4]=s*aspect;
 return {roll*perspectiveReversed(fov,aspect,.1f,8000)*view(eye,target),
   (right*c-up*s)*(scale*aspect),(right*(-s)-up*c)*scale,forward};
}
}

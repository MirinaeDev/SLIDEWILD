#pragma once
#include <algorithm>
#include <cmath>
namespace dh {
constexpr float pi=3.14159265359f;
struct V3 { float x=0,y=0,z=0;
 V3 operator+(V3 b) const {return {x+b.x,y+b.y,z+b.z};}
 V3 operator-(V3 b) const {return {x-b.x,y-b.y,z-b.z};}
 V3 operator*(float s) const {return {x*s,y*s,z*s};}
};
inline float dot(V3 a,V3 b){return a.x*b.x+a.y*b.y+a.z*b.z;}
inline V3 cross(V3 a,V3 b){return {a.y*b.z-a.z*b.y,a.z*b.x-a.x*b.z,a.x*b.y-a.y*b.x};}
inline float length(V3 a){return std::sqrt(dot(a,a));}
inline V3 norm(V3 a){return a*(1.f/std::max(.00001f,length(a)));}
inline float mix(float a,float b,float t){return a+(b-a)*t;}
inline V3 mix(V3 a,V3 b,float t){return a+(b-a)*t;}
inline float clamp(float a,float lo,float hi){return std::clamp(a,lo,hi);}
inline float wrap(float a){return std::remainder(a,2*pi);}
inline float approach(float a,float b,float rate,float dt){return mix(a,b,1-std::exp(-rate*dt));}
struct Mat {float m[16]{};}; // Row-major, multiplied by column vectors in HLSL.
inline Mat identity(){Mat a; a.m[0]=a.m[5]=a.m[10]=a.m[15]=1;return a;}
inline Mat operator*(Mat a,Mat b){Mat c;for(int i=0;i<4;i++)for(int j=0;j<4;j++)for(int k=0;k<4;k++)c.m[i*4+j]+=a.m[i*4+k]*b.m[k*4+j];return c;}
inline Mat perspective(float fov,float aspect,float nearZ,float farZ){Mat a;float f=1/std::tan(fov*.5f);a.m[0]=f/aspect;a.m[5]=f;a.m[10]=farZ/(farZ-nearZ);a.m[11]=-nearZ*farZ/(farZ-nearZ);a.m[14]=1;return a;}
// D32 floating-point depth retains centimetre-scale facade separation at distance
// when near maps to 1 and far to 0. Sun shadows keep their orthographic convention.
inline Mat perspectiveReversed(float fov,float aspect,float nearZ,float farZ){
 Mat a;float f=1/std::tan(fov*.5f);a.m[0]=f/aspect;a.m[5]=f;
 a.m[10]=-nearZ/(farZ-nearZ);a.m[11]=nearZ*farZ/(farZ-nearZ);a.m[14]=1;return a;
}
inline Mat view(V3 eye,V3 target){V3 z=norm(target-eye),x=norm(cross({0,1,0},z)),y=cross(z,x);Mat m=identity();V3 rows[]={x,y,z};for(int i=0;i<3;i++){m.m[i*4]=rows[i].x;m.m[i*4+1]=rows[i].y;m.m[i*4+2]=rows[i].z;m.m[i*4+3]=-dot(rows[i],eye);}return m;}
inline float hash(float n){return std::fmod(std::sin(n*127.1f+311.7f)*43758.5453f+50000.f,1.f);}
}

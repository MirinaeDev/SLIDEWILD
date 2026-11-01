#pragma once
#include "math.hpp"
#include "pixel_grid.hpp"
#include "camera_projection.hpp"
#include <SDL3/SDL.h>
#include <vector>
#include <string>
#include <cstddef>
#include <type_traits>
namespace dh {
struct Vertex {V3 p,n,color;float u=0,v=0,material=0;};
using Mesh=std::vector<Vertex>;
struct Uniforms {Mat mvp;float eye[4];float weather[4];float camera[4];Mat sunMvp;float dynamics[4];float raster[4];float rayX[4];float rayY[4];float rayZ[4];float riderState[4][4];float cloth[4][4];};
static_assert(std::is_standard_layout_v<Vertex> && sizeof(Vertex)==48);
static_assert(offsetof(Vertex,u)==36 && offsetof(Vertex,material)==44);
static_assert(sizeof(Uniforms)==384); // 384 bytes, shared HLSL/MSL ABI.
static_assert(offsetof(Uniforms,sunMvp)==112 && offsetof(Uniforms,riderState)==256 && offsetof(Uniforms,cloth)==320);
class Renderer {
public:
 SDL_Window* window=nullptr;SDL_GPUDevice* gpu=nullptr;
 int width=960,height=540;std::string driver;
 bool integerPixels=true;
 int pixelWidth=960,pixelHeight=540;
 void configurePixels(int w,int h,bool fixedCapture=false){
   width=std::clamp(int(std::round(540.f*w/std::max(h,1))),540,1920);
   auto grid=fitPixelGrid(w,h);
   pixelWidth=integerPixels&&!fixedCapture?grid.width:width;
   pixelHeight=integerPixels&&!fixedCapture?grid.height:540;
 }
 Uint32 worldFirst=0,worldCount=0;
 Renderer();~Renderer();
 void uploadWorld(const Mesh& vertices);
 void draw(const Mesh& sky,const Mesh& moving,const Mesh& hud,const Uniforms& world,bool capture=false);
 size_t dynamicVertexCapacity()const{return dynamicCapacity;}
private:
 size_t dynamicCapacity=0;
 void ensureDynamicCapacity(size_t needed);
 SDL_GPUTexture *color=nullptr,*processed=nullptr,*depth=nullptr,*sunDepth=nullptr;
 SDL_GPUGraphicsPipeline *pipeline=nullptr,*flatPipeline=nullptr,*shadowPipeline=nullptr,*sunPipeline=nullptr;
 SDL_GPUBuffer *staticBuffer=nullptr,*dynamicBuffer=nullptr,*sunStaticBuffer=nullptr;
 SDL_GPUTransferBuffer* transfer=nullptr;
 SDL_GPUTexture* artwork[82]{};
 SDL_GPUSampler* artSampler=nullptr;
 SDL_GPUSampler* architectureSampler=nullptr;
 Uint32 staticCount=0,sunStaticCount=0;int previousWidth=0,previousHeight=0;
 void targets();SDL_GPUShader* shader(const char* stage);
 SDL_GPUTexture* loadArt(const char* name);
};
void triangle(Mesh&m,V3 a,V3 b,V3 c,V3 color,float material=0);
void quad(Mesh&m,V3 a,V3 b,V3 c,V3 d,V3 color,float material=0);
void box(Mesh&m,V3 p,V3 size,V3 color,float yaw=0,float material=0);
void text(Mesh&m,const std::string&str,float x,float y,float scale,V3 color,int width);
void rect(Mesh&m,float x,float y,float w,float h,V3 color,int width);
}

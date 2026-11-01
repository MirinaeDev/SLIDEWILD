#include "renderer.hpp"
#include "runtime_paths.hpp"
#include "gpu_backend.hpp"
#include "vertex_capacity.hpp"
#include "rider_pose.hpp"
#include "courses.hpp"
#include <cstring>
#include <fstream>
#include <stdexcept>
#include <filesystem>
namespace dh {
static void require(bool ok,const char*what){if(!ok)throw std::runtime_error(std::string(what)+": "+SDL_GetError());}
SDL_GPUShader* Renderer::shader(const char* stage){
 auto path=resourcePath(std::string("shaders/world.")+stage+gameShaderExtension);
 std::ifstream file(path,std::ios::binary);std::vector<Uint8> bytes((std::istreambuf_iterator<char>(file)),{});
 require(!bytes.empty(),pathUtf8(path).c_str());
 const auto codeSize=bytes.size();if(gameShaderFormat==SDL_GPU_SHADERFORMAT_MSL)bytes.push_back(0);
 SDL_GPUShaderCreateInfo info{};info.code=bytes.data();info.code_size=codeSize;info.entrypoint=stage[0]=='v'?"vs_main":(stage[0]=='s'?"shadow_ps":"ps_main");info.format=gameShaderFormat;info.stage=stage[0]=='v'?SDL_GPU_SHADERSTAGE_VERTEX:SDL_GPU_SHADERSTAGE_FRAGMENT;info.num_uniform_buffers=1;
 if(stage[0]=='p')info.num_samplers=16;
 if(stage[0]=='s'){info.num_samplers=11;info.num_uniform_buffers=0;}
 auto result=SDL_CreateGPUShader(gpu,&info);require(result,(std::string("Create ")+gameGPUDriver+" shader "+stage).c_str());return result;
}
SDL_GPUTexture* Renderer::loadArt(const char*name){
 std::ifstream f(resourcePath(std::string("assets/")+name+".rgba"),std::ios::binary);
 Uint32 w=0,h=0;f.read(reinterpret_cast<char*>(&w),4);f.read(reinterpret_cast<char*>(&h),4);
 require(f.good()&&w>0&&h>0&&w<=8192&&h<=8192,"Artwork header");
 std::vector<Uint8> bytes(size_t(w)*h*4);f.read(reinterpret_cast<char*>(bytes.data()),bytes.size());require(f.good(),"Artwork pixels");
 SDL_GPUTextureCreateInfo ci{};ci.type=SDL_GPU_TEXTURETYPE_2D;ci.format=SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM;ci.usage=SDL_GPU_TEXTUREUSAGE_SAMPLER|SDL_GPU_TEXTUREUSAGE_COLOR_TARGET;ci.width=w;ci.height=h;ci.layer_count_or_depth=1;ci.num_levels=1+Uint32(std::floor(std::log2(std::max(w,h))));ci.sample_count=SDL_GPU_SAMPLECOUNT_1;
 auto texture=SDL_CreateGPUTexture(gpu,&ci);require(texture,"Artwork texture");
 SDL_GPUTransferBufferCreateInfo ti{};ti.usage=SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;ti.size=Uint32(bytes.size());auto tb=SDL_CreateGPUTransferBuffer(gpu,&ti);require(tb,"Artwork upload");void*p=SDL_MapGPUTransferBuffer(gpu,tb,false);require(p,"Artwork map");std::memcpy(p,bytes.data(),bytes.size());SDL_UnmapGPUTransferBuffer(gpu,tb);
 auto cmd=SDL_AcquireGPUCommandBuffer(gpu);auto pass=SDL_BeginGPUCopyPass(cmd);SDL_GPUTextureTransferInfo src{};src.transfer_buffer=tb;src.pixels_per_row=w;src.rows_per_layer=h;SDL_GPUTextureRegion dst{};dst.texture=texture;dst.w=w;dst.h=h;dst.d=1;SDL_UploadToGPUTexture(pass,&src,&dst,false);SDL_EndGPUCopyPass(pass);SDL_GenerateMipmapsForGPUTexture(cmd,texture);require(SDL_SubmitGPUCommandBuffer(cmd),"Artwork submit");SDL_ReleaseGPUTransferBuffer(gpu,tb);return texture;
}
Renderer::Renderer(){
 require(SDL_Init(SDL_INIT_VIDEO|SDL_INIT_GAMEPAD),"SDL Init");
 window=SDL_CreateWindow("SLIDEWILD / Arcade longboarding",1440,810,SDL_WINDOW_RESIZABLE|SDL_WINDOW_HIGH_PIXEL_DENSITY);require(window,"Window");
 gpu=SDL_CreateGPUDevice(gameShaderFormat,false,gameGPUDriver);require(gpu,"GPU device");driver=SDL_GetGPUDeviceDriver(gpu);require(SDL_ClaimWindowForGPUDevice(gpu,window),"Claim window");
 // Mailbox avoids a queue of stale input frames on supported displays.
 if(SDL_WindowSupportsGPUPresentMode(gpu,window,SDL_GPU_PRESENTMODE_MAILBOX))SDL_SetGPUSwapchainParameters(gpu,window,SDL_GPU_SWAPCHAINCOMPOSITION_SDR,SDL_GPU_PRESENTMODE_MAILBOX);
 const char* names[]={"rig-sol-v30","pine-reference","panorama-full","vegetation-v2","asphalt-v2","grass-v3","flora-v4","masonry-v2","fall-v1","cliff-material-v1","valley-layer-v2","select-stage-v16","logo-slidewild","select-sol-v15","select-rio-v15","select-nox-v15","ui-font-v15","ride-sol-v15","ride-rio-v15","ride-nox-v15","aster-map-v15","select-mei-v16","rig-rio-v30","rig-nox-v30","rig-mei-v30","fall-mei-v16","intro-sky-v16","intro-peaks-v16","intro-valley-v16","intro-foreground-v16","arcade-badges-en","parking-props-v16","nameplates-v16","desert-panorama-v18","urban-panorama-v18","desert-props-v18","urban-props-v18","sandstone-v18","urban-facades-v18","desert-details-v18","urban-details-v18","desert-habitat-v18","urban-habitat-v18","scotland-panorama-v18","tropical-panorama-v18","tropical-storm-v18","scotland-props-v18","tropical-props-v18","scotland-details-v18","tropical-details-v18","scotland-habitat-v18","tropical-habitat-v18","biome-ground-v18","longboard-fall-v19","scotland-understory-v19","tropical-understory-v19","future-sky-v23","future-props-v23","future-facades-v23","caldeira-sky-v24","caldeira-props-v24","caldeira-geysers-v24","caldeira-materials-v24","future-arches-v25","future-facades-v25","future-sky-v26","future-surfaces-v26","future-rails-v26","future-props-v26","future-architecture-v28","future-sky-v28","future-road-v28","future-deck-v28","turn-sol-v30","turn-rio-v30","turn-nox-v30","turn-mei-v30","brake-cross-v31","wind-sol-v32","wind-rio-v32","wind-nox-v32","wind-mei-v32"};
 for(int i=0;i<82;i++)artwork[i]=loadArt(names[i]);
 SDL_GPUSamplerCreateInfo sampler{};sampler.min_filter=SDL_GPU_FILTER_NEAREST;sampler.mag_filter=SDL_GPU_FILTER_NEAREST;sampler.mipmap_mode=SDL_GPU_SAMPLERMIPMAPMODE_NEAREST;sampler.max_lod=16;sampler.address_mode_u=SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;sampler.address_mode_v=SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;sampler.address_mode_w=SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;artSampler=SDL_CreateGPUSampler(gpu,&sampler);require(artSampler,"Artwork sampler");
 // Keep magnified pixels crisp, but blend minification/mip transitions on
 // Nova's painted elevations. Riders, vegetation and UI retain nearest sampling.
 sampler.min_filter=SDL_GPU_FILTER_LINEAR;sampler.mipmap_mode=SDL_GPU_SAMPLERMIPMAPMODE_LINEAR;
 architectureSampler=SDL_CreateGPUSampler(gpu,&sampler);require(architectureSampler,"Architecture sampler");
 auto vs=shader("vs"),ps=shader("ps");
 SDL_GPUVertexBufferDescription vb{};vb.slot=0;vb.pitch=sizeof(Vertex);vb.input_rate=SDL_GPU_VERTEXINPUTRATE_VERTEX;
 SDL_GPUVertexAttribute attrs[4]{};for(int i=0;i<4;i++){attrs[i].location=i;attrs[i].buffer_slot=0;attrs[i].format=SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3;attrs[i].offset=i*12;}
 SDL_GPUColorTargetDescription target{};target.format=SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM;
 SDL_GPUGraphicsPipelineCreateInfo info{};info.vertex_shader=vs;info.fragment_shader=ps;info.vertex_input_state={&vb,1,attrs,4};info.primitive_type=SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;info.rasterizer_state.fill_mode=SDL_GPU_FILLMODE_FILL;info.rasterizer_state.cull_mode=SDL_GPU_CULLMODE_NONE;info.multisample_state.sample_count=SDL_GPU_SAMPLECOUNT_1;
 info.depth_stencil_state.compare_op=SDL_GPU_COMPAREOP_GREATER_OR_EQUAL;info.depth_stencil_state.enable_depth_test=true;info.depth_stencil_state.enable_depth_write=true;
 info.target_info.color_target_descriptions=&target;info.target_info.num_color_targets=1;info.target_info.depth_stencil_format=SDL_GPU_TEXTUREFORMAT_D32_FLOAT;info.target_info.has_depth_stencil_target=true;
 pipeline=SDL_CreateGPUGraphicsPipeline(gpu,&info);require(pipeline,"World pipeline");
 auto depthShader=shader("shadow");auto sunInfo=info;sunInfo.fragment_shader=depthShader;sunInfo.depth_stencil_state.compare_op=SDL_GPU_COMPAREOP_LESS_OR_EQUAL;
 sunInfo.target_info.num_color_targets=0;sunInfo.target_info.color_target_descriptions=nullptr;
 sunPipeline=SDL_CreateGPUGraphicsPipeline(gpu,&sunInfo);require(sunPipeline,"Sun depth pipeline");SDL_ReleaseGPUShader(gpu,depthShader);
 SDL_GPUTextureCreateInfo sunTexture{};sunTexture.type=SDL_GPU_TEXTURETYPE_2D;sunTexture.format=SDL_GPU_TEXTUREFORMAT_D32_FLOAT;
 sunTexture.usage=SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET|SDL_GPU_TEXTUREUSAGE_SAMPLER;sunTexture.width=sunTexture.height=1536;
 sunTexture.layer_count_or_depth=1;sunTexture.num_levels=1;sunTexture.sample_count=SDL_GPU_SAMPLECOUNT_1;
 sunDepth=SDL_CreateGPUTexture(gpu,&sunTexture);require(sunDepth,"Sun depth map");
 info.depth_stencil_state.enable_depth_write=false;
 target.blend_state.enable_blend=true;
 target.blend_state.src_color_blendfactor=SDL_GPU_BLENDFACTOR_SRC_ALPHA;
 target.blend_state.dst_color_blendfactor=SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
 target.blend_state.color_blend_op=SDL_GPU_BLENDOP_ADD;
 target.blend_state.src_alpha_blendfactor=SDL_GPU_BLENDFACTOR_ONE;
 target.blend_state.dst_alpha_blendfactor=SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
 target.blend_state.alpha_blend_op=SDL_GPU_BLENDOP_ADD;
 shadowPipeline=SDL_CreateGPUGraphicsPipeline(gpu,&info);require(shadowPipeline,"Shadow pipeline");
 target.blend_state.enable_blend=true;
 info.depth_stencil_state.enable_depth_test=false;info.depth_stencil_state.enable_depth_write=false;
 flatPipeline=SDL_CreateGPUGraphicsPipeline(gpu,&info);require(flatPipeline,"HUD pipeline");SDL_ReleaseGPUShader(gpu,vs);SDL_ReleaseGPUShader(gpu,ps);
 ensureDynamicCapacity(400000);targets();
}
void Renderer::ensureDynamicCapacity(size_t needed){
 if(needed<=dynamicCapacity)return;
 size_t capacity=growVertexCapacity(dynamicCapacity,needed,sizeof(Vertex));
 require(SDL_WaitForGPUIdle(gpu),"Wait for vertex resize");
 SDL_GPUBufferCreateInfo bi{};bi.usage=SDL_GPU_BUFFERUSAGE_VERTEX;bi.size=Uint32(capacity*sizeof(Vertex));
 auto replacement=SDL_CreateGPUBuffer(gpu,&bi);require(replacement,"Grow dynamic vertex buffer");
 SDL_GPUTransferBufferCreateInfo ti{};ti.usage=SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;ti.size=bi.size;
 auto upload=SDL_CreateGPUTransferBuffer(gpu,&ti);
 if(!upload){SDL_ReleaseGPUBuffer(gpu,replacement);require(false,"Grow vertex upload buffer");}
 if(dynamicBuffer)SDL_ReleaseGPUBuffer(gpu,dynamicBuffer);
 if(transfer)SDL_ReleaseGPUTransferBuffer(gpu,transfer);
 dynamicBuffer=replacement;transfer=upload;dynamicCapacity=capacity;
 SDL_Log("Dynamic vertex capacity: %u",unsigned(capacity));
}
void Renderer::targets(){
 SDL_WaitForGPUIdle(gpu);if(color)SDL_ReleaseGPUTexture(gpu,color);if(depth)SDL_ReleaseGPUTexture(gpu,depth);if(processed)SDL_ReleaseGPUTexture(gpu,processed);
 SDL_GPUTextureCreateInfo t{};t.type=SDL_GPU_TEXTURETYPE_2D;t.format=SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM;t.usage=SDL_GPU_TEXTUREUSAGE_COLOR_TARGET|SDL_GPU_TEXTUREUSAGE_SAMPLER;t.width=pixelWidth;t.height=pixelHeight;t.layer_count_or_depth=1;t.num_levels=1;t.sample_count=SDL_GPU_SAMPLECOUNT_1;color=SDL_CreateGPUTexture(gpu,&t);require(color,"Color target");processed=SDL_CreateGPUTexture(gpu,&t);require(processed,"Lens target");t.format=SDL_GPU_TEXTUREFORMAT_D32_FLOAT;t.usage=SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET;depth=SDL_CreateGPUTexture(gpu,&t);require(depth,"Depth target");previousWidth=pixelWidth;previousHeight=pixelHeight;
}
void Renderer::uploadWorld(const Mesh&v){
 SDL_WaitForGPUIdle(gpu);if(staticBuffer){SDL_ReleaseGPUBuffer(gpu,staticBuffer);staticBuffer=nullptr;}if(sunStaticBuffer){SDL_ReleaseGPUBuffer(gpu,sunStaticBuffer);sunStaticBuffer=nullptr;}
 staticCount=Uint32(v.size());SDL_GPUBufferCreateInfo b{};b.usage=SDL_GPU_BUFFERUSAGE_VERTEX;b.size=Uint32(v.size()*sizeof(Vertex));staticBuffer=SDL_CreateGPUBuffer(gpu,&b);require(staticBuffer,"World vertices");
 SDL_GPUTransferBufferCreateInfo ti{};ti.usage=SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;ti.size=b.size;auto tb=SDL_CreateGPUTransferBuffer(gpu,&ti);require(tb,"World staging");void*p=SDL_MapGPUTransferBuffer(gpu,tb,false);require(p,"Map world");std::memcpy(p,v.data(),b.size);SDL_UnmapGPUTransferBuffer(gpu,tb);
 auto cmd=SDL_AcquireGPUCommandBuffer(gpu);auto pass=SDL_BeginGPUCopyPass(cmd);SDL_GPUTransferBufferLocation src{tb,0};SDL_GPUBufferRegion dst{staticBuffer,0,b.size};SDL_UploadToGPUBuffer(pass,&src,&dst,false);SDL_EndGPUCopyPass(pass);SDL_SubmitGPUCommandBuffer(cmd);SDL_ReleaseGPUTransferBuffer(gpu,tb);
 // Only raised architecture casts here. Removing the underground heightfield
 // and road avoids thousands of irrelevant triangles and terrain self-acne.
 Mesh casters;
 for(size_t i=0;i<v.size();i+=3){float mat=v[i].material;if(mat==1||mat==3||mat==9||mat==51)continue;casters.insert(casters.end(),v.begin()+i,v.begin()+i+3);}
 sunStaticCount=Uint32(casters.size());b.size=Uint32(casters.size()*sizeof(Vertex));
 sunStaticBuffer=SDL_CreateGPUBuffer(gpu,&b);require(sunStaticBuffer,"Static sun casters");
 ti.size=b.size;tb=SDL_CreateGPUTransferBuffer(gpu,&ti);require(tb,"Sun caster staging");p=SDL_MapGPUTransferBuffer(gpu,tb,false);require(p,"Map sun casters");
 std::memcpy(p,casters.data(),b.size);SDL_UnmapGPUTransferBuffer(gpu,tb);
 cmd=SDL_AcquireGPUCommandBuffer(gpu);pass=SDL_BeginGPUCopyPass(cmd);src={tb,0};dst={sunStaticBuffer,0,b.size};
 SDL_UploadToGPUBuffer(pass,&src,&dst,false);SDL_EndGPUCopyPass(pass);SDL_SubmitGPUCommandBuffer(cmd);SDL_ReleaseGPUTransferBuffer(gpu,tb);

}
void Renderer::draw(const Mesh&sky,const Mesh&moving,const Mesh&hud,const Uniforms&world,bool capture){
 if(pixelWidth!=previousWidth||pixelHeight!=previousHeight)targets();
 Mesh ordered,shadows,sunCasters,npc[4],npcShadows[4];ordered.reserve(moving.size());std::vector<size_t> mist;
 for(size_t i=0;i<moving.size();i+=3){float mat=moving[i].material;
   if(mat>=36&&mat<48){
     int kind=(int(mat)-36)/4,who=(int(mat)-36)%4;auto&dst=kind==2?npcShadows[who]:npc[who];
     for(int k=0;k<3;k++){auto v=moving[i+k];v.material=(kind==0?5.f:kind==1?22.f:14.f)+who*.1f;dst.push_back(v);}continue;
   }
   if(mat>=11&&mat<12){mist.push_back(i);continue;}
   if((mat>=13&&mat<13.5f)||(mat>=21&&mat<21.5f)||(mat>=6&&mat<6.5f)||(mat>=29&&mat<29.5f)||(mat>=32&&mat<32.5f)||(mat>=33&&mat<33.5f)||(mat>=35&&mat<35.5f)){
     V3 eye{world.eye[0],world.eye[1],world.eye[2]};if(length(moving[i].p-eye)<160)sunCasters.insert(sunCasters.end(),moving.begin()+i,moving.begin()+i+3);
   }
   Mesh& dest=(mat==7||(mat>=14&&mat<14.5f)||(mat>=23&&mat<23.5f))?shadows:ordered;
   dest.insert(dest.end(),moving.begin()+i,moving.begin()+i+3);
 }
 V3 eye{world.eye[0],world.eye[1],world.eye[2]};
 auto distance=[&](size_t i){V3 p=(moving[i].p+moving[i+1].p+moving[i+2].p)*(1.f/3)-eye;return dot(p,p);};
 std::stable_sort(mist.begin(),mist.end(),[&](size_t a,size_t b){return distance(a)>distance(b);});
 for(size_t i:mist)shadows.insert(shadows.end(),moving.begin()+i,moving.begin()+i+3);
 size_t baseOpaqueCount=ordered.size(),npcOffset[4],npcShadowOffset[4];
 for(int j=0;j<4;j++){npcOffset[j]=ordered.size();ordered.insert(ordered.end(),npc[j].begin(),npc[j].end());}
 size_t opaqueCount=ordered.size();ordered.insert(ordered.end(),shadows.begin(),shadows.end());
 for(int j=0;j<4;j++){npcShadowOffset[j]=ordered.size();ordered.insert(ordered.end(),npcShadows[j].begin(),npcShadows[j].end());}
 Mesh post;quad(post,{-1,1,0},{1,1,0},{1,-1,0},{-1,-1,0},{1,1,1},-3);
 const float postUV[6][2]={{0,0},{1,0},{1,1},{0,0},{1,1},{0,1}};
 for(int i=0;i<6;i++){post[i].u=postUV[i][0];post[i].v=postUV[i][1];}
 size_t count=sky.size()+moving.size()+hud.size()+sunCasters.size()+post.size();ensureDynamicCapacity(count);
 auto data=static_cast<Vertex*>(SDL_MapGPUTransferBuffer(gpu,transfer,true));require(data,"Map dynamic");std::memcpy(data,sky.data(),sky.size()*sizeof(Vertex));std::memcpy(data+sky.size(),ordered.data(),ordered.size()*sizeof(Vertex));std::memcpy(data+sky.size()+moving.size(),hud.data(),hud.size()*sizeof(Vertex));std::memcpy(data+sky.size()+moving.size()+hud.size(),sunCasters.data(),sunCasters.size()*sizeof(Vertex));std::memcpy(data+count-post.size(),post.data(),post.size()*sizeof(Vertex));SDL_UnmapGPUTransferBuffer(gpu,transfer);
 auto cmd=SDL_AcquireGPUCommandBuffer(gpu);require(cmd,"Command buffer");SDL_GPUTexture* swap=nullptr;Uint32 sw=0,sh=0;require(SDL_WaitAndAcquireGPUSwapchainTexture(cmd,window,&swap,&sw,&sh),"Swapchain");if(!swap){SDL_CancelGPUCommandBuffer(cmd);return;}
 auto cp=SDL_BeginGPUCopyPass(cmd);SDL_GPUTransferBufferLocation src{transfer,0};SDL_GPUBufferRegion dst{dynamicBuffer,0,Uint32(count*sizeof(Vertex))};SDL_UploadToGPUBuffer(cp,&src,&dst,true);SDL_EndGPUCopyPass(cp);
 SDL_GPUTextureSamplerBinding textures[16]{};for(int i=0;i<11;i++)textures[i]={artwork[i],artSampler};textures[11]={sunDepth,artSampler};int pilot=std::clamp(int(world.raster[2]),0,3);
 int biome=std::clamp(int(world.raster[3]),0,courseCount-1);if(biome==5)biome=0;
 if(biome==6){
   textures[8]={artwork[72],artSampler};textures[1]={artwork[71],artSampler};textures[2]={artwork[70],artSampler};textures[4]={artwork[66],artSampler};textures[9]={artwork[67],artSampler};textures[3]={artwork[68],artSampler};textures[6]={artwork[69],architectureSampler};textures[7]={artwork[63],artSampler};textures[5]={artwork[58],artSampler};
 }else if(biome==7){
   textures[2]={artwork[59],artSampler};textures[3]={artwork[60],artSampler};textures[8]={artwork[61],artSampler};
   for(int slot:{4,5,6,7,9})textures[slot]={artwork[62],artSampler};
 }else if(biome){
   const int skies[]={0,33,34,43,44},props[]={0,35,36,46,47},details[]={0,39,40,48,49},habitats[]={0,41,42,50,51};
   textures[2]={artwork[skies[biome]],artSampler};textures[3]={artwork[props[biome]],artSampler};
   textures[1]={artwork[details[biome]],artSampler};textures[8]={artwork[habitats[biome]],artSampler};
   if(biome==1)textures[9]={artwork[37],artSampler};
   textures[6]={artwork[biome>=3?54+biome-3:38],artSampler};
   textures[5]={artwork[52],artSampler};
   if(biome==4)textures[10]={artwork[45],artSampler};
 }
 textures[0]={artwork[53],artSampler};
 int rigs[4]={0,22,23,24};const int falls[4]={8,18,19,25};
 for(int j=0;j<4;j++)if(usesTurnAtlas(int(world.riderState[j][0])))rigs[j]=73+j;
 for(int j=0;j<4;j++)if(world.riderState[j][0]>=11)rigs[j]=77;
 for(int j=0;j<4;j++)if(world.riderState[j][0]==0||world.riderState[j][0]==5)rigs[j]=78+j;
 textures[12]={artwork[rigs[pilot]],artSampler};textures[13]={artwork[falls[pilot]],artSampler};textures[14]={artwork[30],artSampler};textures[15]={artwork[31],artSampler};
 SDL_GPUDepthStencilTargetInfo sunTarget{};sunTarget.texture=sunDepth;sunTarget.clear_depth=1;
 sunTarget.load_op=SDL_GPU_LOADOP_CLEAR;sunTarget.store_op=SDL_GPU_STOREOP_STORE;sunTarget.stencil_load_op=SDL_GPU_LOADOP_DONT_CARE;sunTarget.stencil_store_op=SDL_GPU_STOREOP_DONT_CARE;
 auto sunPass=SDL_BeginGPURenderPass(cmd,nullptr,0,&sunTarget);SDL_BindGPUGraphicsPipeline(sunPass,sunPipeline);
 Uniforms light=world;light.mvp=world.sunMvp;SDL_PushGPUVertexUniformData(cmd,0,&light,sizeof(light));
 SDL_BindGPUFragmentSamplers(sunPass,0,textures,11);
 SDL_GPUBufferBinding sunBuffer{sunStaticBuffer,0};SDL_BindGPUVertexBuffers(sunPass,0,&sunBuffer,1);SDL_DrawGPUPrimitives(sunPass,sunStaticCount,1,0,0);
 sunBuffer.buffer=dynamicBuffer;SDL_BindGPUVertexBuffers(sunPass,0,&sunBuffer,1);SDL_DrawGPUPrimitives(sunPass,Uint32(sunCasters.size()),1,Uint32(sky.size()+moving.size()+hud.size()),0);
 SDL_EndGPURenderPass(sunPass);
 SDL_GPUColorTargetInfo ct{};ct.texture=color;ct.clear_color={.2f,.24f,.35f,1};ct.load_op=SDL_GPU_LOADOP_CLEAR;ct.store_op=SDL_GPU_STOREOP_STORE;
 SDL_GPUDepthStencilTargetInfo dt{};dt.texture=depth;dt.clear_depth=0;dt.load_op=SDL_GPU_LOADOP_CLEAR;dt.store_op=SDL_GPU_STOREOP_DONT_CARE;dt.stencil_load_op=SDL_GPU_LOADOP_DONT_CARE;dt.stencil_store_op=SDL_GPU_STOREOP_DONT_CARE;
 auto rp=SDL_BeginGPURenderPass(cmd,&ct,1,&dt);Uniforms flat=world;flat.mvp=identity();std::memcpy(flat.eye,world.eye,sizeof(flat.eye));flat.weather[0]=world.weather[0];flat.weather[1]=world.weather[1];flat.weather[3]=world.weather[3];flat.weather[2]=world.weather[2];std::memcpy(flat.camera,world.camera,sizeof(flat.camera));
 SDL_BindGPUFragmentSamplers(rp,0,textures,16);
 SDL_GPUBufferBinding db{dynamicBuffer,0};SDL_BindGPUVertexBuffers(rp,0,&db,1);SDL_BindGPUGraphicsPipeline(rp,flatPipeline);SDL_PushGPUVertexUniformData(cmd,0,&flat,sizeof(flat));SDL_PushGPUFragmentUniformData(cmd,0,&flat,sizeof(flat));SDL_DrawGPUPrimitives(rp,Uint32(sky.size()),1,0,0);
 SDL_BindGPUGraphicsPipeline(rp,pipeline);SDL_PushGPUVertexUniformData(cmd,0,&world,sizeof(world));SDL_PushGPUFragmentUniformData(cmd,0,&world,sizeof(world));SDL_GPUBufferBinding sb{staticBuffer,0};SDL_BindGPUVertexBuffers(rp,0,&sb,1);SDL_DrawGPUPrimitives(rp,worldCount?worldCount:staticCount,1,worldFirst,0);SDL_BindGPUVertexBuffers(rp,0,&db,1);SDL_DrawGPUPrimitives(rp,Uint32(baseOpaqueCount),1,Uint32(sky.size()),0);
 for(int j=0;j<4;j++)if(!npc[j].empty()){
   textures[12]={artwork[rigs[j]],artSampler};textures[13]={artwork[falls[j]],artSampler};SDL_BindGPUFragmentSamplers(rp,12,textures+12,2);
   SDL_DrawGPUPrimitives(rp,Uint32(npc[j].size()),1,Uint32(sky.size()+npcOffset[j]),0);
 }
 textures[12]={artwork[rigs[pilot]],artSampler};textures[13]={artwork[falls[pilot]],artSampler};SDL_BindGPUFragmentSamplers(rp,12,textures+12,2);
 SDL_BindGPUGraphicsPipeline(rp,shadowPipeline);
 SDL_DrawGPUPrimitives(rp,Uint32(shadows.size()),1,Uint32(sky.size()+opaqueCount),0);
 for(int j=0;j<4;j++)if(!npcShadows[j].empty()){
   textures[12]={artwork[rigs[j]],artSampler};SDL_BindGPUFragmentSamplers(rp,12,textures+12,1);
   SDL_DrawGPUPrimitives(rp,Uint32(npcShadows[j].size()),1,Uint32(sky.size()+npcShadowOffset[j]),0);
 }
 SDL_BindGPUGraphicsPipeline(rp,flatPipeline);SDL_PushGPUVertexUniformData(cmd,0,&flat,sizeof(flat));SDL_PushGPUFragmentUniformData(cmd,0,&flat,sizeof(flat));SDL_EndGPURenderPass(rp);
 // Apply the lens to the finished world at native pixel resolution. This
 // cannot drag geometry from behind the camera across the near clipping plane.
 SDL_GPUColorTargetInfo lensTarget{};lensTarget.texture=processed;lensTarget.load_op=SDL_GPU_LOADOP_DONT_CARE;lensTarget.store_op=SDL_GPU_STOREOP_STORE;
 auto lensPass=SDL_BeginGPURenderPass(cmd,&lensTarget,1,&dt);
 textures[0]={color,artSampler};SDL_BindGPUFragmentSamplers(lensPass,0,textures,16);
 SDL_BindGPUVertexBuffers(lensPass,0,&db,1);SDL_BindGPUGraphicsPipeline(lensPass,flatPipeline);
 flat.weather[2]=float(width)/height;SDL_PushGPUVertexUniformData(cmd,0,&flat,sizeof(flat));SDL_PushGPUFragmentUniformData(cmd,0,&flat,sizeof(flat));
 SDL_DrawGPUPrimitives(lensPass,6,1,Uint32(count-6),0);
 for(int i=0;i<6;i++)textures[i]={artwork[11+i],artSampler};
 textures[6]={artwork[20],artSampler};textures[7]={artwork[21],artSampler};
  textures[8]={artwork[26],artSampler};textures[9]={artwork[27],artSampler};textures[10]={artwork[28],artSampler};textures[12]={artwork[29],artSampler};textures[14]={artwork[30],artSampler};textures[15]={artwork[32],artSampler};
 SDL_BindGPUFragmentSamplers(lensPass,0,textures,16);
 SDL_DrawGPUPrimitives(lensPass,Uint32(hud.size()),1,Uint32(sky.size()+moving.size()),0);SDL_EndGPURenderPass(lensPass);
 SDL_GPUBlitInfo blit{};blit.source.texture=processed;blit.source.w=pixelWidth;blit.source.h=pixelHeight;blit.destination.texture=swap;
 Uint32 scale=std::min(sw/Uint32(pixelWidth),sh/Uint32(pixelHeight));
 if(integerPixels&&scale>0){blit.destination.w=pixelWidth*scale;blit.destination.h=pixelHeight*scale;blit.destination.x=(sw-blit.destination.w)/2;blit.destination.y=(sh-blit.destination.h)/2;}
 else{blit.destination.w=sw;blit.destination.h=sh;}
 blit.load_op=SDL_GPU_LOADOP_CLEAR;blit.clear_color={.025f,.028f,.035f,1};blit.filter=SDL_GPU_FILTER_NEAREST;SDL_BlitGPUTexture(cmd,&blit);
 SDL_GPUTransferBuffer* readback=nullptr;
 if(capture){SDL_GPUTransferBufferCreateInfo ci{};ci.usage=SDL_GPU_TRANSFERBUFFERUSAGE_DOWNLOAD;ci.size=pixelWidth*pixelHeight*4;readback=SDL_CreateGPUTransferBuffer(gpu,&ci);require(readback,"Capture buffer");auto copy=SDL_BeginGPUCopyPass(cmd);SDL_GPUTextureRegion region{};region.texture=processed;region.w=pixelWidth;region.h=pixelHeight;region.d=1;SDL_GPUTextureTransferInfo dest{};dest.transfer_buffer=readback;dest.pixels_per_row=pixelWidth;dest.rows_per_layer=pixelHeight;SDL_DownloadFromGPUTexture(copy,&region,&dest);SDL_EndGPUCopyPass(copy);}
 require(SDL_SubmitGPUCommandBuffer(cmd),"Submit");
 if(readback){SDL_WaitForGPUIdle(gpu);void*p=SDL_MapGPUTransferBuffer(gpu,readback,false);require(p,"Capture map");auto surface=SDL_CreateSurfaceFrom(pixelWidth,pixelHeight,SDL_PIXELFORMAT_RGBA32,p,pixelWidth*4);std::filesystem::create_directories(outputPath("captures"));require(SDL_SaveBMP(surface,pathUtf8(outputPath("captures/latest.bmp")).c_str()),"Save capture");SDL_DestroySurface(surface);SDL_UnmapGPUTransferBuffer(gpu,readback);SDL_ReleaseGPUTransferBuffer(gpu,readback);}
}
Renderer::~Renderer(){if(gpu){SDL_WaitForGPUIdle(gpu);SDL_ReleaseGPUBuffer(gpu,staticBuffer);SDL_ReleaseGPUBuffer(gpu,sunStaticBuffer);SDL_ReleaseGPUBuffer(gpu,dynamicBuffer);SDL_ReleaseGPUTransferBuffer(gpu,transfer);SDL_ReleaseGPUTexture(gpu,color);SDL_ReleaseGPUTexture(gpu,processed);SDL_ReleaseGPUTexture(gpu,depth);SDL_ReleaseGPUTexture(gpu,sunDepth);SDL_ReleaseGPUGraphicsPipeline(gpu,sunPipeline);for(auto* texture:artwork)SDL_ReleaseGPUTexture(gpu,texture);SDL_ReleaseGPUSampler(gpu,artSampler);SDL_ReleaseGPUSampler(gpu,architectureSampler);SDL_ReleaseGPUGraphicsPipeline(gpu,pipeline);SDL_ReleaseGPUGraphicsPipeline(gpu,flatPipeline);SDL_ReleaseGPUGraphicsPipeline(gpu,shadowPipeline);SDL_ReleaseWindowFromGPUDevice(gpu,window);SDL_DestroyGPUDevice(gpu);}if(window)SDL_DestroyWindow(window);SDL_Quit();}
void triangle(Mesh&m,V3 a,V3 b,V3 c,V3 color,float material){V3 n=norm(cross(b-a,c-a));m.push_back({a,n,color,a.x,a.z,material});m.push_back({b,n,color,b.x,b.z,material});m.push_back({c,n,color,c.x,c.z,material});}
void quad(Mesh&m,V3 a,V3 b,V3 c,V3 d,V3 color,float material){triangle(m,a,b,c,color,material);triangle(m,a,c,d,color,material);}
void box(Mesh&m,V3 p,V3 size,V3 color,float yaw,float mat){V3 v[8];for(int i=0;i<8;i++){float x=(i&1?.5f:-.5f)*size.x,z=(i&4?.5f:-.5f)*size.z;v[i]=p+V3{x*std::cos(yaw)+z*std::sin(yaw),(i&2?.5f:-.5f)*size.y,-x*std::sin(yaw)+z*std::cos(yaw)};}int faces[6][4]={{0,1,5,4},{2,6,7,3},{0,2,3,1},{4,5,7,6},{0,4,6,2},{1,3,7,5}};for(auto&f:faces)quad(m,v[f[0]],v[f[1]],v[f[2]],v[f[3]],color,mat);}
void rect(Mesh&m,float x,float y,float w,float h,V3 c,int width){auto p=[&](float a,float b){return V3{a/width*2-1,1-b/540*2,0};};quad(m,p(x,y),p(x+w,y),p(x+w,y+h),p(x,y+h),c,-1);}
void text(Mesh&m,const std::string&s,float x,float y,float scale,V3 c,int width){
 const std::string chars="ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789.-:/+ ";
 static const unsigned char rows[][7]={
 {14,17,17,31,17,17,17},{30,17,17,30,17,17,30},{14,17,16,16,16,17,14},{30,17,17,17,17,17,30},{31,16,16,30,16,16,31},{31,16,16,30,16,16,16},{14,17,16,23,17,17,15},{17,17,17,31,17,17,17},{31,4,4,4,4,4,31},{7,2,2,2,18,18,12},{17,18,20,24,20,18,17},{16,16,16,16,16,16,31},{17,27,21,21,17,17,17},{17,25,25,21,19,19,17},{14,17,17,17,17,17,14},{30,17,17,30,16,16,16},{14,17,17,17,21,18,13},{30,17,17,30,20,18,17},{15,16,16,14,1,1,30},{31,4,4,4,4,4,4},{17,17,17,17,17,17,14},{17,17,17,17,17,10,4},{17,17,17,21,21,27,17},{17,17,10,4,10,17,17},{17,17,10,4,4,4,4},{31,1,2,4,8,16,31},
 {14,17,19,21,25,17,14},{4,12,4,4,4,4,14},{14,17,1,2,4,8,31},{30,1,1,14,1,1,30},{2,6,10,18,31,2,2},{31,16,16,30,1,1,30},{14,16,16,30,17,17,14},{31,1,2,4,8,8,8},{14,17,17,14,17,17,14},{14,17,17,15,1,1,14},{0,0,0,0,0,12,12},{0,0,0,31,0,0,0},{0,12,12,0,12,12,0},{1,2,2,4,8,8,16},{0,4,4,31,4,4,0},{0,0,0,0,0,0,0}};
 for(char ch:s){auto index=chars.find(ch);if(index!=std::string::npos)for(int j=0;j<7;j++)for(int i=0;i<5;i++)if(rows[index][j]&(1<<(4-i)))rect(m,x+i*scale,y+j*scale,scale,scale,c,width);x+=6*scale;}
}
} // namespace dh

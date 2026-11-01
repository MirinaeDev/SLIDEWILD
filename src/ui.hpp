#pragma once
#include "renderer.hpp"
#include "ui_font_data.hpp"
namespace dh {
inline void uiImage(Mesh&m,int width,float x,float y,float w,float h,float material,float u0=0,float v0=0,float u1=1,float v1=1,V3 color={1,1,1}){
 x=std::round(x);y=std::round(y);w=std::round(w);h=std::round(h);
 auto p=[&](float a,float b){return V3{a/width*2-1,1-b/270,0};};
 size_t start=m.size();quad(m,p(x,y),p(x+w,y),p(x+w,y+h),p(x,y+h),color,material);
 const float uv[6][2]={{u0,v0},{u1,v0},{u1,v1},{u0,v0},{u1,v1},{u0,v1}};
 for(int k=0;k<6;k++){m[start+k].u=uv[k][0];m[start+k].v=uv[k][1];}
}
inline float uiTextWidth(const std::string&s,int font=0,float spacing=1){float w=0;for(unsigned char c:s)if(c>=32&&c<127)w+=uiGlyphs[font][c-32].advance+spacing;return w;}
inline void uiRawText(Mesh&m,int width,const std::string&s,float x,float y,int font,V3 color,float spacing=1){
 for(unsigned char c:s){if(c<32||c>126)continue;auto g=uiGlyphs[font][c-32];
   if(g.w&&g.h)uiImage(m,width,x,y+g.dy,float(g.w),float(g.h),-11,g.x/1024.f,g.y/1024.f,(g.x+g.w)/1024.f,(g.y+g.h)/1024.f,color);
   x+=g.advance+spacing;
 }
}
inline void uiText(Mesh&m,int width,const std::string&s,float x,float y,int font,V3 color,float spacing=1){
 if(font==0)uiRawText(m,width,s,x+1,y+1,font,{.02f,.025f,.04f},spacing);
 if(font>=1){
   for(auto d:std::initializer_list<std::pair<int,int>>{{-1,0},{1,0},{0,-1},{0,1},{2,3}})
     uiRawText(m,width,s,x+d.first,y+d.second,font,{.025f,.028f,.065f},spacing);
 }
 uiRawText(m,width,s,x,y,font,color,spacing);
}
inline void uiCentered(Mesh&m,int width,const std::string&s,float cx,float y,int font,V3 color){uiText(m,width,s,cx-uiTextWidth(s,font)*.5f,y,font,color);}
inline void uiPanel(Mesh&m,int width,float x,float y,float w,float h,V3 color,float alpha=.88f){uiImage(m,width,x,y,w,h,-13,alpha,0,alpha,1,color);}
// Small stepped corners, one-pixel highlights and a single shadow all live on
// the same render grid as the illustrated assets.
inline void uiCut(Mesh&m,int width,float x,float y,float w,float h,V3 color,float alpha=1,float cut=5){
 uiPanel(m,width,x+cut,y,w-cut*2,h,color,alpha);
 uiPanel(m,width,x,y+cut,cut,h-cut*2,color,alpha);
 uiPanel(m,width,x+w-cut,y+cut,cut,h-cut*2,color,alpha);
 for(int j=1;j<int(cut);j++){
   uiPanel(m,width,x+cut-j,y+j,j,1,color,alpha);uiPanel(m,width,x+w-cut,y+j,j,1,color,alpha);
   uiPanel(m,width,x+cut-j,y+h-j-1,j,1,color,alpha);uiPanel(m,width,x+w-cut,y+h-j-1,j,1,color,alpha);
 }
}
inline void uiFrame(Mesh&m,int width,float x,float y,float w,float h,V3 accent,float alpha=.88f){
 uiCut(m,width,x+3,y+4,w,h,{.015f,.023f,.04f},.35f);
 uiCut(m,width,x,y,w,h,{.035f,.055f,.08f},alpha);
 rect(m,x,y+5,1,h-10,accent*.38f,width);rect(m,x+w-1,y+5,1,h-10,accent*.38f,width);
 rect(m,x+5,y+h-1,w-10,1,accent*.30f,width);
 for(int j=0;j<5;j++){
   rect(m,x+5-j,y+j,1,1,accent*.55f,width);rect(m,x+w-6+j,y+j,1,1,accent*.55f,width);
   rect(m,x+5-j,y+h-1-j,1,1,accent*.30f,width);rect(m,x+w-6+j,y+h-1-j,1,1,accent*.30f,width);
 }
 rect(m,x+7,y,w-14,1,accent*.78f,width);
 rect(m,x+7,y,27,2,accent,width);
}
inline float uiKey(Mesh&m,int width,const std::string&key,float x,float y,V3 accent){
 float w=std::max(20.f,uiTextWidth(key,0,0)+12);
 uiCut(m,width,x,y,w,18,accent*.45f,1,2);
 uiCut(m,width,x+1,y+1,w-2,14,{.075f,.095f,.12f},1,1);
 uiRawText(m,width,key,x+(w-uiTextWidth(key,0,0))*.5f,y+2,0,{1,.96f,.84f},0);
 return w;
}
inline float uiHint(Mesh&m,int width,const std::string&key,const std::string&label,float x,float y,V3 accent){
 float k=uiKey(m,width,key,x,y,accent);uiText(m,width,label,x+k+7,y+2,0,{.79f,.83f,.83f});
 return k+uiTextWidth(label)+26;
}
inline void uiChevron(Mesh&m,int width,float x,float y,int direction,V3 color){
 for(int j=0;j<7;j++)rect(m,x+direction*(j<4?j:6-j),y+j,3,1,color,width);
}
inline void uiSteps(Mesh&m,int width,int active,float x,float y,V3 accent){
 const char* names[]={"RIDER","COURSE","START"};
 for(int j=0;j<3;j++){
   float px=x+j*82;V3 c=j==active?accent:V3{.42f,.46f,.50f};
   rect(m,px,y+22,70,2,c*.60f,width);
   uiText(m,width,std::to_string(j+1),px,y,1,c);uiText(m,width,names[j],px+18,y+3,0,c);
 }
}
inline void uiButton(Mesh&m,int width,const std::string&s,float x,float y,float w,V3 accent){
 // Cabinet-style lettering and a flying underline, not a desktop button.
 uiText(m,width,s,x+17,y+7,0,{1,.97f,.85f});
 uiChevron(m,width,x+3,y+9,1,accent);uiChevron(m,width,x+8,y+9,1,accent);
 for(int j=0;j<3;j++)rect(m,x+16+j*3,y+25+j,w-35-j*19,1,accent*(1-j*.23f),width);
}
}

#pragma once
#include <algorithm>
#include <cmath>
namespace dh {
struct PixelGrid{int width,height,scale,x,y;};
inline PixelGrid fitPixelGrid(int width,int height){
 width=std::max(1,width);height=std::max(1,height);
 int scale=std::max(1,int(std::round(height/540.f)));
 int w=std::max(1,width/scale),h=std::max(1,height/scale);
 return {w,h,scale,(width-w*scale)/2,(height-h*scale)/2};
}
}

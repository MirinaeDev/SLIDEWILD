#pragma once
#include "math.hpp"
#include "runtime_paths.hpp"
#include "sprite_data.hpp"
#include <SDL3/SDL.h>
#include <array>
#include <filesystem>
#include <fstream>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>
namespace dh {
struct SpriteContact {float aspect,u,v;};
// Read the runtime alpha, not the PNG's padded rectangle. No artistic pixels
// are changed. Cached once per drawing; all copies share the same contact.
inline SpriteContact spriteContact(const char* name,SpriteRect rect){
 struct Atlas {unsigned w=0,h=0;std::vector<unsigned char> pixels;std::map<std::array<int,4>,SpriteContact> contacts;};
 static std::map<std::string,Atlas> cache;
 auto& atlas=cache[name];
 if(atlas.pixels.empty()){
   auto path=resourcePath("assets/"+std::string(name)+".rgba");
   if(!std::filesystem::exists(path))path=std::filesystem::path("assets")/(std::string(name)+".rgba");
   std::ifstream file(path,std::ios::binary);file.read(reinterpret_cast<char*>(&atlas.w),4);file.read(reinterpret_cast<char*>(&atlas.h),4);
   if(!file||!atlas.w||!atlas.h)throw std::runtime_error("Missing sprite contact atlas: "+std::string(name));
   atlas.pixels.resize(size_t(atlas.w)*atlas.h*4);file.read(reinterpret_cast<char*>(atlas.pixels.data()),atlas.pixels.size());
   if(!file)throw std::runtime_error("Incomplete sprite contact atlas: "+std::string(name));
 }
 std::array<int,4> b={int(std::round(rect.x0*atlas.w)),int(std::round(rect.y0*atlas.h)),int(std::round(rect.x1*atlas.w)),int(std::round(rect.y1*atlas.h))};
 auto found=atlas.contacts.find(b);if(found!=atlas.contacts.end())return found->second;
 int foot=b[1];
 for(int y=b[3]-1;y>=b[1];y--){int count=0;for(int x=b[0];x<b[2];x++)count+=atlas.pixels[(size_t(y)*atlas.w+x)*4+3]>190;if(count>=2){foot=y;break;}}
 float xSum=0,count=0;int depth=std::max(2,int((b[3]-b[1])*.008f));
 for(int y=std::max(b[1],foot-depth);y<=foot;y++)for(int x=b[0];x<b[2];x++)if(atlas.pixels[(size_t(y)*atlas.w+x)*4+3]>190){xSum+=x+.5f;count++;}
 if(count==0)throw std::runtime_error("Empty sprite contact");
 SpriteContact contact{float(b[2]-b[0])/(b[3]-b[1]),(xSum/count-b[0])/(b[2]-b[0]),float(foot+1-b[1])/(b[3]-b[1])};
 atlas.contacts.emplace(b,contact);return contact;
}
}

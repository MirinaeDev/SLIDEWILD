#pragma once
#include <SDL3/SDL.h>
#include <filesystem>
#include <stdexcept>
#include <string>
#include <string_view>
namespace dh {
inline std::string pathUtf8(const std::filesystem::path& path){
 auto text=path.u8string();return {reinterpret_cast<const char*>(text.data()),text.size()};
}
inline std::filesystem::path pathFromUtf8(std::string_view text){return std::filesystem::path(std::u8string(reinterpret_cast<const char8_t*>(text.data()),text.size()));}
inline std::filesystem::path resourcePath(const std::string& relative){
 const char* base=SDL_GetBasePath();
 if(!base)throw std::runtime_error(std::string("Resource directory: ")+SDL_GetError());
 // SDL returns Contents/Resources for a macOS .app and the exe directory on Windows.
 return pathFromUtf8(base)/pathFromUtf8(relative);
}
inline std::filesystem::path outputDirectory=".";
inline void initializeOutputPaths(bool automated){
 if(automated)outputDirectory=std::filesystem::current_path();
 else {
   char* location=SDL_GetPrefPath("DamienMiri","Slidewild");
   if(!location)throw std::runtime_error(std::string("Writable user directory: ")+SDL_GetError());
   outputDirectory=pathFromUtf8(location);SDL_free(location);
 }
 std::filesystem::create_directories(outputDirectory);
}
inline std::filesystem::path outputPath(const std::string& relative){return outputDirectory/pathFromUtf8(relative);}
}

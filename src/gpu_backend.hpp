#pragma once
#include <SDL3/SDL.h>
namespace dh {
#if defined(__APPLE__)
#if !defined(__arm64__) && !defined(__aarch64__)
#error SLIDEWILD targets Apple Silicon only on macOS.
#endif
inline constexpr SDL_GPUShaderFormat gameShaderFormat=SDL_GPU_SHADERFORMAT_MSL;
inline constexpr const char* gameGPUDriver="metal";
inline constexpr const char* gameShaderExtension=".msl";
#elif defined(_WIN32)
inline constexpr SDL_GPUShaderFormat gameShaderFormat=SDL_GPU_SHADERFORMAT_DXIL;
inline constexpr const char* gameGPUDriver="direct3d12";
inline constexpr const char* gameShaderExtension=".dxil";
#else
#error Supported platforms: Windows x64 and macOS Apple Silicon.
#endif
}

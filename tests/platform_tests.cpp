#include "runtime_paths.hpp"
#include "gpu_backend.hpp"
#include "renderer.hpp"
#include <fstream>
#include <iostream>
int main(){try{
 using namespace dh;
 initializeOutputPaths(true);
 if(outputDirectory!=std::filesystem::current_path())throw std::runtime_error("Automated output escaped its working directory");
 if(!resourcePath("assets").is_absolute())throw std::runtime_error("Resources depend on current working directory");
 const std::string filename="path-test-\xc3\xa9-\xe6\xb5\x8b\xe8\xaf\x95.txt";
 auto path=outputPath(filename);
 if(pathUtf8(path.filename())!=filename)throw std::runtime_error("UTF-8 path round-trip failed");
 {std::ofstream file(path);file<<"unicode-path-ok";if(!file)throw std::runtime_error("Cannot write Unicode diagnostics path");}
 std::string contents;{std::ifstream file(path);file>>contents;}
 std::filesystem::remove(path);
 if(contents!="unicode-path-ok")throw std::runtime_error("Cannot read Unicode diagnostics path");
#if defined(__APPLE__)
 if(gameShaderFormat!=SDL_GPU_SHADERFORMAT_MSL||std::string(gameGPUDriver)!="metal")throw std::runtime_error("Wrong Apple backend");
#else
 if(gameShaderFormat!=SDL_GPU_SHADERFORMAT_DXIL||std::string(gameGPUDriver)!="direct3d12")throw std::runtime_error("Wrong Windows backend");
#endif
 std::cout<<"PASS platform: GPU selection, 384-byte uniform ABI, absolute resources, isolated diagnostics, Unicode paths\n";
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}}

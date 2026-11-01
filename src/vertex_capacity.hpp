#pragma once
#include <cstddef>
#include <cstdint>
#include <limits>
#include <stdexcept>
namespace dh {
inline size_t growVertexCapacity(size_t current,size_t needed,size_t stride){
 if(!stride||needed>std::numeric_limits<uint32_t>::max()/stride)throw std::runtime_error("Scene exceeds GPU buffer address space");
 if(needed<=current)return current;
 const size_t maximum=std::numeric_limits<uint32_t>::max()/stride;
 size_t result=current?current:400000;
 if(result>maximum)result=maximum;
 while(result<needed)result=result>maximum/2?maximum:result*2;
 return result;
}
}

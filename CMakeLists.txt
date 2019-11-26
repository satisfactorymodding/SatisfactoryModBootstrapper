cmake_minimum_required(VERSION 3.12)
project(xinput1_3)

set(CMAKE_CXX_STANDARD 14)
enable_language(ASM_MASM)

include_directories(typeinfo/include)
include_directories(pelib/include)
include_directories(MemoryModule)

file(GLOB source_list "src/*")
add_library(xinput1_3 SHARED ${source_list})

target_link_libraries(xinput1_3 ${CMAKE_CURRENT_SOURCE_DIR}/typeinfo/typeinfo.lib)
target_link_libraries(xinput1_3 ${CMAKE_CURRENT_SOURCE_DIR}/pelib/pelib.lib)
target_link_libraries(xinput1_3 ${CMAKE_CURRENT_SOURCE_DIR}/MemoryModule/MemoryModule.lib)
target_link_libraries(xinput1_3 dbghelp.lib)


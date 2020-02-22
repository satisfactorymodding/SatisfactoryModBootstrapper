cmake_minimum_required(VERSION 3.12)
project(xinput1_3)

set(CMAKE_CXX_STANDARD 17)
enable_language(ASM_MASM)

include_directories(MemoryModule)
include_directories("C:\\Program Files (x86)\\Microsoft Visual Studio\\2017\\Community\\VC\\Tools\\MSVC\\14.16.27023\\atlmfc\\include")
include_directories("C:\\Program Files (x86)\\Microsoft Visual Studio\\2017\\Community\\DIA SDK\\include")

file(GLOB source_list "src/*")
add_library(xinput1_3 SHARED ${source_list})
target_link_libraries(xinput1_3 ${CMAKE_CURRENT_SOURCE_DIR}/MemoryModule/MemoryModule.lib)
target_link_libraries(xinput1_3 "C:\\Program Files (x86)\\Microsoft Visual Studio\\2017\\Community\\DIA SDK\\lib\\amd64\\diaguids.lib")
target_link_libraries(xinput1_3 "C:\\Program Files (x86)\\Microsoft Visual Studio\\2017\\Community\\VC\\Tools\\MSVC\\14.16.27023\\atlmfc\\lib\\x64\\atls.lib")
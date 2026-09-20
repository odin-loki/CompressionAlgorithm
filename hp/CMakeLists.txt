cmake_minimum_required(VERSION 3.16)
project(hp CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

# NOTE: no -ffast-math needed or wanted. The codec contains no floating
# point at all, which is what makes archives bit-identical across
# toolchains. test/no_float.sh enforces this.

add_library(hp_headers INTERFACE)
target_include_directories(hp_headers INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/xsimd/include)
# SIMD mixer dots on by default (bit-identical to scalar). Disable with -DHP_XSIMD=0.

add_executable(hp src/main.cpp)
target_link_libraries(hp PRIVATE hp_headers)

add_executable(dump_experts tools/dump_experts.cpp)
target_link_libraries(dump_experts PRIVATE hp_headers)

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  set(_hp_gnu -O3 -flto -march=x86-64-v3 -mavx2 -fno-exceptions -fno-rtti
              -DHP_XSIMD=1 -msse4.1)
  target_compile_options(hp PRIVATE ${_hp_gnu})
  target_link_options(hp PRIVATE -flto)
  target_compile_options(dump_experts PRIVATE ${_hp_gnu})
  target_link_options(dump_experts PRIVATE -flto)
endif()

enable_testing()
add_test(NAME no_float COMMAND bash ${CMAKE_CURRENT_SOURCE_DIR}/test/no_float.sh)

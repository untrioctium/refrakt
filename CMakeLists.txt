cmake_minimum_required(VERSION 3.6...3.18)
include(FetchContent)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(BUILD_TESTING OFF CACHE BOOL "" FORCE)
set(BUILD_BENCHMARK OFF CACHE BOOL "" FORCE)

project(
  refrakt
  VERSION 0.1
  LANGUAGES CXX C)

set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
FetchContent_Declare(
  glfw
  GIT_REPOSITORY https://github.com/glfw/glfw.git
  GIT_TAG 3.3.2
)
FetchContent_MakeAvailable(glfw)

set(YAML_CPP_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(YAML_CPP_BUILD_TOOLS OFF CACHE BOOL "" FORCE)
FetchContent_Declare(
  yaml_cpp
  GIT_REPOSITORY https://github.com/jbeder/yaml-cpp.git
  GIT_TAG yaml-cpp-0.6.3
)
FetchContent_MakeAvailable(yaml_cpp)

FetchContent_Declare(
  glad
  GIT_REPOSITORY https://github.com/Dav1dde/glad.git
  GIT_TAG v0.1.34
)
FetchContent_MakeAvailable(glad)

FetchContent_Declare(
  pugixml
  GIT_REPOSITORY https://github.com/zeux/pugixml.git
  GIT_TAG v1.10
)
FetchContent_MakeAvailable(pugixml)

FetchContent_Declare(
  fmt
  GIT_REPOSITORY https://github.com/fmtlib/fmt.git
  GIT_TAG 7.1.2
)
FetchContent_MakeAvailable(fmt)

FetchContent_Declare(
  glm
  GIT_REPOSITORY https://github.com/g-truc/glm.git
  GIT_TAG 0.9.9.8
)
FetchContent_MakeAvailable(glm)

FetchContent_Declare(
  stb
  GIT_REPOSITORY https://github.com/nothings/stb.git
)
FetchContent_MakeAvailable(stb)

FetchContent_Declare(
  xxhash
  GIT_REPOSITORY https://github.com/Cyan4973/xxHash.git
  GIT_TAG v0.8.0
)
FetchContent_MakeAvailable(xxhash)
file(GLOB XHASH_SOURCES ${xxhash_SOURCE_DIR}/*.c)
list(REMOVE_ITEM XHASH_SOURCES ${xxhash_SOURCE_DIR}/xxhsum.c)

#FetchContent_Declare(
#  json
#  GIT_REPOSITORY https://github.com/ArthurSonzogni/nlohmann_json_cmake_fetchcontent
#  GIT_TAG v3.9.1
#)
#FetchContent_MakeAvailable(json)

FetchContent_Declare(
  inja
  GIT_REPOSITORY https://github.com/pantor/inja.git
  GIT_TAG v3.1.0
)
FetchContent_MakeAvailable(inja)

FetchContent_Declare(
  imgui
  GIT_REPOSITORY https://github.com/ocornut/imgui.git
  GIT_TAG docking
)
FetchContent_MakeAvailable(imgui)
file(GLOB IMGUI_SOURCES
  ${imgui_SOURCE_DIR}/*.cpp
  ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
  ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
)

file(GLOB REFRAKT_SOURCES src/*.cpp)

add_executable(refrakt ${REFRAKT_SOURCES} ${XHASH_SOURCES} ${IMGUI_SOURCES})
target_link_libraries(refrakt PRIVATE glfw)
target_link_libraries(refrakt PRIVATE yaml-cpp)
target_link_libraries(refrakt PRIVATE inja)
target_link_libraries(refrakt PRIVATE glad)
target_link_libraries(refrakt PRIVATE pugixml)
target_link_libraries(refrakt PRIVATE fmt)
target_link_libraries(refrakt PRIVATE glm)
target_include_directories(refrakt PRIVATE src)
target_include_directories(refrakt PRIVATE ${imgui_SOURCE_DIR})
target_include_directories(refrakt PRIVATE ${imgui_SOURCE_DIR}/backends)
target_include_directories(refrakt PRIVATE ${stb_SOURCE_DIR})
target_include_directories(refrakt PRIVATE ${xxhash_SOURCE_DIR})

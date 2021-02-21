cmake_minimum_required(VERSION 3.11...3.18)
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

file(GLOB REFRAKT_SOURCES src/*.cpp src/*.hpp)

# do nice things for IDEs
file(GLOB SHADER_FILES
       ${CMAKE_SOURCE_DIR}/shaders/*.glsl
       ${CMAKE_SOURCE_DIR}/shaders/include/*.glsl
       ${CMAKE_SOURCE_DIR}/shaders/templates/*.glsl)
source_group(TREE "${CMAKE_SOURCE_DIR}/shaders" PREFIX "Shaders" FILES ${SHADER_FILES})

add_executable(refrakt ${REFRAKT_SOURCES} ${SHADER_FILES})
set_target_properties(refrakt PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
target_include_directories(refrakt PRIVATE src)

# glfw
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
FetchContent_Declare(
  glfw
  GIT_REPOSITORY https://github.com/glfw/glfw.git
  GIT_TAG 3.3.2
)
FetchContent_MakeAvailable(glfw)
target_link_libraries(refrakt PRIVATE glfw)

# yaml-cpp
set(YAML_CPP_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(YAML_CPP_BUILD_TOOLS OFF CACHE BOOL "" FORCE)
FetchContent_Declare(
  yaml_cpp
  GIT_REPOSITORY https://github.com/jbeder/yaml-cpp.git
  GIT_TAG yaml-cpp-0.6.3
)
FetchContent_MakeAvailable(yaml_cpp)
target_link_libraries(refrakt PRIVATE yaml-cpp)

# glad
FetchContent_Declare(
  glad
  GIT_REPOSITORY https://github.com/Dav1dde/glad.git
  GIT_TAG v0.1.34
)
FetchContent_MakeAvailable(glad)
target_link_libraries(refrakt PRIVATE glad)

# pugixml
FetchContent_Declare(
  pugixml
  GIT_REPOSITORY https://github.com/zeux/pugixml.git
  GIT_TAG v1.10
)
FetchContent_MakeAvailable(pugixml)
target_link_libraries(refrakt PRIVATE pugixml)

# fmt
FetchContent_Declare(
  fmt
  GIT_REPOSITORY https://github.com/fmtlib/fmt.git
  GIT_TAG 7.1.2
)
FetchContent_MakeAvailable(fmt)
target_link_libraries(refrakt PRIVATE fmt)

# sqlitecpp
set(SQLITECPP_RUN_CPPLINT OFF CACHE BOOL "" FORCE)
set(SQLITECPP_RUN_CPPCHECK OFF CACHE BOOL "" FORCE)
FetchContent_Declare(
  sqlitecpp
  GIT_REPOSITORY https://github.com/cuberite/SQLiteCpp.git
)
FetchContent_MakeAvailable(sqlitecpp)
target_link_libraries(refrakt PRIVATE SQLiteCpp)

# glm
FetchContent_Declare(
  glm
  GIT_REPOSITORY https://github.com/g-truc/glm.git
  GIT_TAG 0.9.9.8
)
FetchContent_MakeAvailable(glm)
target_link_libraries(refrakt PRIVATE glm)

# stb
FetchContent_Declare(
  stb
  GIT_REPOSITORY https://github.com/nothings/stb.git
)
FetchContent_MakeAvailable(stb)
file(GLOB STB_SOURCES ${stb_SOURCE_DIR}/*.c)
add_library(stb STATIC ${STB_SOURCES})
target_include_directories(stb PUBLIC ${stb_SOURCE_DIR})
target_link_libraries(refrakt PRIVATE stb)

# xxhash
FetchContent_Declare(
  xxhash
  GIT_REPOSITORY https://github.com/Cyan4973/xxHash.git
  GIT_TAG v0.8.0
)
FetchContent_MakeAvailable(xxhash)
file(GLOB XXHASH_SOURCES ${xxhash_SOURCE_DIR}/*.c)
list(REMOVE_ITEM XXHASH_SOURCES ${xxhash_SOURCE_DIR}/xxhsum.c)
list(REMOVE_ITEM XXHASH_SOURCES ${xxhash_SOURCE_DIR}/xxh_x86dispatch.c)
add_library(xxhash STATIC ${XXHASH_SOURCES})
target_include_directories(xxhash PUBLIC ${xxhash_SOURCE_DIR})
target_link_libraries(refrakt PRIVATE xxhash)

# inja, note that this also includes nlohmann::json
FetchContent_Declare(
  inja
  GIT_REPOSITORY https://github.com/pantor/inja.git
  GIT_TAG v3.1.0
)
FetchContent_MakeAvailable(inja)
target_link_libraries(refrakt PRIVATE inja)

#imgui
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
add_library(imgui STATIC ${IMGUI_SOURCES})
target_include_directories(imgui PUBLIC ${imgui_SOURCE_DIR})
target_include_directories(imgui PUBLIC ${imgui_SOURCE_DIR}/backends)
target_link_libraries(imgui PUBLIC glfw)
target_link_libraries(imgui PUBLIC glad)
target_link_libraries(refrakt PRIVATE imgui)

# physfs
set(PHYSFS_BUILD_SHARED OFF CACHE BOOL "" FORCE)
set(PHYSFS_BUILD_TEST OFF CACHE BOOL "" FORCE)
FetchContent_Declare(
  physfs
  GIT_REPOSITORY https://github.com/criptych/physfs.git
)
FetchContent_MakeAvailable(physfs)
target_link_libraries(refrakt PRIVATE physfs-static)

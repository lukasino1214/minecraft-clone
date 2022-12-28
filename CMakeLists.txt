cmake_minimum_required(VERSION 3.12)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
project(minecraft-clone)
include(FetchContent)

FetchContent_Declare(FastNoise2 GIT_REPOSITORY https://github.com/lukasino1214/FastNoise2.git)
FetchContent_MakeAvailable(FastNoise2)

find_package(daxa CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(glfw3 CONFIG REQUIRED)

find_path(STB_INCLUDE_DIRS "stb_image.h" REQUIRED)
add_library(stb_interface INTERFACE)
target_include_directories(stb_interface INTERFACE ${STB_INCLUDE_DIRS})

add_executable(${PROJECT_NAME} "src/main.cpp" "src/camera.cpp" "src/camera.hpp" "src/chunk.hpp" "src/textures.hpp")
target_link_libraries(${PROJECT_NAME} PRIVATE daxa::daxa glm::glm glfw FastNoise2 stb_interface)
target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_20)

set(DAXA_INCLUDE_DIR "$<TARGET_FILE_DIR:${PROJECT_NAME}>/../vcpkg_installed/x64-$<LOWER_CASE:$<PLATFORM_ID>>/include")
target_compile_definitions(${PROJECT_NAME} PRIVATE DAXA_SHADER_INCLUDE_DIR="${DAXA_INCLUDE_DIR}")

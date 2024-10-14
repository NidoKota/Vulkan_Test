cmake_minimum_required(VERSION 3.22)

project(Vulkan_Test)

set(CMAKE_CXX_STANDARD 17)

add_executable(app ../src/Utility.hpp ../src/Main.cpp)

add_compile_definitions(VULKAN_TEST_MAC)

find_package(Vulkan REQUIRED)
target_include_directories(app PRIVATE ${Vulkan_INCLUDE_DIRS})
target_link_libraries(app PRIVATE ${Vulkan_LIBRARIES})
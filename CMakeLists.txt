cmake_minimum_required(VERSION 3.16)

project(raytracer VERSION 0.6.0 LANGUAGES CXX)

option(RAYTRACER_ENABLE_SANITIZERS
       "Enable address and undefined-behavior sanitizers" OFF)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Threads REQUIRED)

set(RAYTRACER_SOURCES
    raytracer/src/Math/Quaternion.cpp
    raytracer/src/Math/Sampler.cpp
    raytracer/src/Math/Vec3.cpp
    raytracer/src/Materials/Bsdf.cpp
    raytracer/src/Materials/Material.cpp
    raytracer/src/Renderer.cpp
    raytracer/src/Scene/Camera.cpp
    raytracer/src/Scene/Environment.cpp
    raytracer/src/Scene/Light/LightSource.cpp
    raytracer/src/Scene/Light/PointSource.cpp
    raytracer/src/Scene/Scene.cpp
    raytracer/src/Shapes/Cube.cpp
    raytracer/src/Shapes/Plane.cpp
    raytracer/src/Shapes/Pyramid.cpp
    raytracer/src/Shapes/Rectangle.cpp
    raytracer/src/Shapes/RectangularPrism.cpp
    raytracer/src/Shapes/Shape.cpp
    raytracer/src/Shapes/Sphere.cpp
    raytracer/src/Writer/ImageWriter.cpp
)

add_library(raytracer_lib ${RAYTRACER_SOURCES})
target_include_directories(raytracer_lib PUBLIC raytracer/src)
target_link_libraries(raytracer_lib PUBLIC Threads::Threads)

add_executable(raytracer raytracer/src/main.cpp)
target_link_libraries(raytracer PRIVATE raytracer_lib)

enable_testing()
add_executable(raytracer_tests tests/RayTracerTests.cpp)
target_link_libraries(raytracer_tests PRIVATE raytracer_lib)

foreach(target raytracer_lib raytracer raytracer_tests)
    if(MSVC)
        target_compile_options(${target} PRIVATE /W4 /permissive-)
    else()
        target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic)
    endif()
endforeach()

if(RAYTRACER_ENABLE_SANITIZERS AND
   CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(raytracer_lib PRIVATE
        -fsanitize=address,undefined -fno-omit-frame-pointer)
    foreach(target raytracer raytracer_tests)
        target_compile_options(${target} PRIVATE
            -fsanitize=address,undefined -fno-omit-frame-pointer)
        target_link_options(${target} PRIVATE -fsanitize=address,undefined)
    endforeach()
endif()

if(MINGW AND NOT RAYTRACER_ENABLE_SANITIZERS)
    target_link_options(raytracer PRIVATE -static)
    target_link_options(raytracer_tests PRIVATE -static)
endif()

add_test(NAME raytracer_tests COMMAND raytracer_tests)

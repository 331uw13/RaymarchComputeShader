cmake_minimum_required(VERSION 3.18.2)
project(RaymarchingWithComputeShaders C)

set(CMAKE_BUILD_TYPE Release)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -Wunused-function -lm -lglfw -lGL -ldl -O3 -floop-parallelize-all")

file(GLOB_RECURSE SOURCE_FILES "./src/*.c")

add_executable(
	raymarch
	${SOURCE_FILES}
)

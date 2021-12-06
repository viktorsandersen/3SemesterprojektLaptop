cmake_minimum_required(VERSION 3.20)
project(3SemesterprojektLaptop)

set(CMAKE_CXX_STANDARD 17)

add_executable(3SemesterprojektLaptop main.cpp definitioner.h decoder.cpp decoder.h sampler.cpp sampler.h processor.cpp processor.h toolbox.cpp toolbox.h)

set(SFML_STATIC_LIBRARIES TRUE)
set(SFML_DIR C:/SFML/lib/cmake/SFML)
find_package(SFML COMPONENTS system window graphics audio network REQUIRED)

include_directories(C:/SFML/include)
target_link_libraries(3SemesterprojektLaptop sfml-system sfml-window sfml-graphics sfml-audio)
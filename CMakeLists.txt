cmake_minimum_required(VERSION 3.10)

project(Advanced_Internship)

add_executable(main 
    src/main.cpp 
    src/ParseCoordinates.cpp
)

target_include_directories(main 
    PUBLIC
    ${CMAKE_SOURCE_DIR}/include/
)

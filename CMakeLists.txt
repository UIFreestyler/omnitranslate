cmake_minimum_required(VERSION 3.10)
project(omnitranslate)

add_executable(omnitranslate main.cpp)

install(TARGETS omnitranslate DESTINATION bin)

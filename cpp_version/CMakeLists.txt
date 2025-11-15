cmake_minimum_required(VERSION 3.10)
project(ZeeCpp)

set(CMAKE_CXX_STANDARD 17)

find_package(cpr REQUIRED)
find_package(nlohmann_json REQUIRED)

add_executable(zee_cpp main.cpp base64.cpp)
target_link_libraries(zee_cpp PRIVATE cpr::cpr nlohmann_json::nlohmann_json)

cmake_minimum_required(VERSION 3.28)
project(PackageTest CXX)

find_package(app CONFIG REQUIRED)

add_executable(app_test src/app/AppTest.cpp)
target_link_libraries(app_test app::app)

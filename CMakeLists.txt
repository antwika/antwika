cmake_minimum_required(VERSION 3.28)

project(antwika CXX)

include(CTest)

# Enable folders in IDEs
#set_property(GLOBAL PROPERTY USE_FOLDERS ON)

enable_testing()

add_subdirectory(libs/time)
add_subdirectory(libs/log)
add_subdirectory(libs/engine)

add_subdirectory(apps/app)

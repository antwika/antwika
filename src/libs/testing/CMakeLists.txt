add_library(antwika_testing INTERFACE)

add_library(antwika::testing ALIAS antwika_testing)

target_include_directories(antwika_testing
    INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

find_package(GTest REQUIRED)

target_link_libraries(antwika_testing
    INTERFACE
        gtest::gtest
)

install(
    TARGETS antwika_testing
    EXPORT antwika_testingTargets
)

install(
    DIRECTORY include/
    DESTINATION include
)

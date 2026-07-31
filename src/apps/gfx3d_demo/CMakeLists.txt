add_executable(antwika_gfx3d_demo
    src/CubeMesh.cpp
    src/SpinLoop.cpp
    src/SpinScene.cpp
    src/main.cpp
)

target_include_directories(antwika_gfx3d_demo
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(antwika_gfx3d_demo
    PRIVATE
        antwika::app
        antwika::gfx
        antwika::gfx_backend
        antwika::log
)

if(BUILD_TESTING AND NOT CMAKE_CROSSCOMPILING)
    add_subdirectory(tests)
endif()

antwika_bundle_app(
    TARGET antwika_gfx3d_demo
)

install(TARGETS antwika_gfx3d_demo
    RUNTIME DESTINATION bin
)

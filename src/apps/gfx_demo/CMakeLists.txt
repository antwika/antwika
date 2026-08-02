add_executable(antwika_gfx_demo
    src/DemoLoop.cpp
    src/DemoScene.cpp
    src/main.cpp
)

target_include_directories(antwika_gfx_demo
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(antwika_gfx_demo
    PRIVATE
        antwika::app
        antwika::cli
        antwika::gfx
        antwika::gfx_backend
        antwika::input
        antwika::input_backend
        antwika::log
        antwika::time
        antwika::ui
)

if(BUILD_TESTING AND NOT CMAKE_CROSSCOMPILING)
    add_subdirectory(tests)
endif()

antwika_bundle_app(
    TARGET antwika_gfx_demo
    ASSETS
        assets/antwika.png
)

install(TARGETS antwika_gfx_demo
    RUNTIME DESTINATION bin
)

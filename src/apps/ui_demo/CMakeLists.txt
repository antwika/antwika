add_executable(antwika_ui_demo
    src/DemoOverlay.cpp
    src/DemoScene.cpp
    src/DemoSink.cpp
    src/DemoState.cpp
    src/KeyMapping.cpp
    src/RenderSink.cpp
    src/TickBudgetSource.cpp
    src/UiDemo.cpp
    src/main.cpp
)

target_include_directories(antwika_ui_demo
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(antwika_ui_demo
    PRIVATE
        antwika::app
        antwika::engine
        antwika::event
        antwika::gfx
        antwika::gfx_backend
        antwika::i18n
        antwika::input
        antwika::input_backend
        antwika::log
        antwika::replay
        antwika::simulation
        antwika::time
        antwika::ui
)

if(BUILD_TESTING AND NOT CMAKE_CROSSCOMPILING)
    add_subdirectory(tests)
endif()

antwika_bundle_app(
    TARGET antwika_ui_demo
)

install(TARGETS antwika_ui_demo
    RUNTIME DESTINATION bin
)

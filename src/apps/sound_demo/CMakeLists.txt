add_executable(antwika_sound_demo
    src/DemoLoop.cpp
    src/DemoTrack.cpp
    src/main.cpp
)

target_include_directories(antwika_sound_demo
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(antwika_sound_demo
    PRIVATE
        antwika::app
        antwika::log
        antwika::sound
        antwika::sound_backend
        antwika::time
)

if(BUILD_TESTING AND NOT CMAKE_CROSSCOMPILING)
    add_subdirectory(tests)
endif()

antwika_bundle_app(
    TARGET antwika_sound_demo
)

install(TARGETS antwika_sound_demo
    RUNTIME DESTINATION bin
)

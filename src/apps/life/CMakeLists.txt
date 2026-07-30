add_executable(antwika_life
    src/Board.cpp
    src/BoardScene.cpp
    src/BoardSink.cpp
    src/Grid.cpp
    src/Life.cpp
    src/LifeSystem.cpp
    src/PrintSystem.cpp
    src/main.cpp
)

target_include_directories(antwika_life
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

set(ANTWIKA_LIFE_DEMO_REPLAY_PATH
    "${CMAKE_CURRENT_SOURCE_DIR}/replays/demo.json")

target_compile_definitions(antwika_life
    PRIVATE
        ANTWIKA_LIFE_DEMO_REPLAY_PATH="${ANTWIKA_LIFE_DEMO_REPLAY_PATH}"
)

target_link_libraries(antwika_life
    PRIVATE
        antwika::ecs
        antwika::engine
        antwika::gfx
        antwika::log
        antwika::replay
        antwika::time
)

if(BUILD_TESTING AND NOT CMAKE_CROSSCOMPILING)
    add_subdirectory(tests)
endif()

if(MINGW)
    execute_process(
        COMMAND ${CMAKE_CXX_COMPILER} -print-file-name=libgcc_s_seh-1.dll
        OUTPUT_VARIABLE LIBGCC_DLL
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )

    execute_process(
        COMMAND ${CMAKE_CXX_COMPILER} -print-file-name=libstdc++-6.dll
        OUTPUT_VARIABLE LIBSTDCPP_DLL
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )

    execute_process(
        COMMAND ${CMAKE_CXX_COMPILER} -print-file-name=libwinpthread-1.dll
        OUTPUT_VARIABLE LIBWINPTHREAD_DLL
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )

    foreach(DLL
        ${LIBGCC_DLL}
        ${LIBSTDCPP_DLL}
        ${LIBWINPTHREAD_DLL}
    )
        if(EXISTS ${DLL})
            add_custom_command(TARGET antwika_life POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    ${DLL}
                    $<TARGET_FILE_DIR:antwika_life>
            )
        endif()
    endforeach()
endif()

install(TARGETS antwika_life
    RUNTIME DESTINATION bin
)

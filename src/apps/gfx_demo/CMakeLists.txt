add_executable(antwika_gfx_demo
    src/DemoLoop.cpp
    src/DemoScene.cpp
    src/main.cpp
)

# Baked in at configure time, as the poker app does with its demo
# replay, so the binary needs no install step and no working directory.
# Only the app gets this: every test draws a mock texture instead.
set(ANTWIKA_GFX_DEMO_TEXTURE_PATH
    "${CMAKE_CURRENT_SOURCE_DIR}/assets/antwika.png")

target_compile_definitions(antwika_gfx_demo
    PRIVATE
        ANTWIKA_GFX_DEMO_TEXTURE_PATH="${ANTWIKA_GFX_DEMO_TEXTURE_PATH}"
)

target_include_directories(antwika_gfx_demo
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(antwika_gfx_demo
    PRIVATE
        antwika::app
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
            add_custom_command(TARGET antwika_gfx_demo POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    ${DLL}
                    $<TARGET_FILE_DIR:antwika_gfx_demo>
            )
        endif()
    endforeach()
endif()

install(TARGETS antwika_gfx_demo
    RUNTIME DESTINATION bin
)

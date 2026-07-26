cmake_minimum_required(VERSION 3.28)

project(game CXX)

if(NOT TARGET antwika::time)
    find_package(time REQUIRED)
endif()

if(NOT TARGET antwika::log)
    find_package(log REQUIRED)
endif()

if(NOT TARGET antwika::event)
    find_package(event REQUIRED)
endif()

if(NOT TARGET antwika::engine)
    find_package(engine REQUIRED)
endif()

add_executable(game src/Game.cpp src/main.cpp)

target_include_directories(game
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_link_libraries(game PRIVATE antwika::time)
target_link_libraries(game PRIVATE antwika::log)
target_link_libraries(game PRIVATE antwika::event)
target_link_libraries(game PRIVATE antwika::engine)

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
            add_custom_command(TARGET game POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    ${DLL}
                    $<TARGET_FILE_DIR:game>
            )
        endif()
    endforeach()
endif()

install(TARGETS game DESTINATION "."
        RUNTIME DESTINATION bin
        ARCHIVE DESTINATION lib
        LIBRARY DESTINATION lib
)

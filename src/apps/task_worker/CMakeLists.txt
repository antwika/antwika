add_executable(antwika_task_worker
    src/TaskDispatchSystem.cpp
    src/TaskJob.cpp
    src/TaskRegistry.cpp
    src/TaskStatusPrintSystem.cpp
    src/TaskSubmissionSink.cpp
    src/TaskWorker.cpp
    src/WorkerCompletionSystem.cpp
    src/WorkerLookup.cpp
    src/WorkerStatusPrintSystem.cpp
    src/main.cpp
)

target_include_directories(antwika_task_worker
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(antwika_task_worker
    PRIVATE
        antwika::ecs
        antwika::engine
        antwika::event
        antwika::log
        antwika::replay
        antwika::scheduler
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
            add_custom_command(TARGET antwika_task_worker POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    ${DLL}
                    $<TARGET_FILE_DIR:antwika_task_worker>
            )
        endif()
    endforeach()
endif()

install(TARGETS antwika_task_worker
    RUNTIME DESTINATION bin
)

add_executable(antwika_task_worker
    src/StatusPrintSystem.cpp
    src/TaskDispatchSystem.cpp
    src/TaskJob.cpp
    src/TaskRegistry.cpp
    src/TaskSubmissionSink.cpp
    src/TaskWorker.cpp
    src/Worker.cpp
    src/WorkerCompletionSystem.cpp
    src/WorkerLookup.cpp
    src/main.cpp
)

target_include_directories(antwika_task_worker
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(antwika_task_worker
    PRIVATE
        antwika::app
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

antwika_bundle_app(
    TARGET antwika_task_worker
    ASSETS
        replays/demo.json
)

install(TARGETS antwika_task_worker
    RUNTIME DESTINATION bin
)

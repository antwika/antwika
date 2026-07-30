find_package(GTest REQUIRED)

add_executable(antwika_task_worker_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/StatusPrintSystem.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/TaskDispatchSystem.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/TaskJob.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/TaskRegistry.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/TaskSubmissionSink.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/TaskWorker.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/Worker.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/WorkerCompletionSystem.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/WorkerLookup.cpp
    BootstrapTest.cpp
    ReplayIntegrationTest.cpp
    StatusPrintSystemTest.cpp
    TaskDispatchSystemTest.cpp
    TaskJobTest.cpp
    TaskRegistryTest.cpp
    TaskSubmissionSinkTest.cpp
    WorkerCompletionSystemTest.cpp
    WorkerTest.cpp
)

target_include_directories(antwika_task_worker_tests
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../include
)

target_link_libraries(antwika_task_worker_tests
    PRIVATE
        antwika::ecs
        antwika::engine
        antwika::log::tests::mocks
        antwika::replay
        antwika::scheduler
        antwika::scheduler::tests::mocks
        antwika::time::tests::fakes
        antwika::event::tests::mocks
        gtest::gtest
)

include(GoogleTest)
gtest_discover_tests(antwika_task_worker_tests)

find_package(GTest REQUIRED)

add_executable(antwika_task_worker_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/TaskDispatchSystem.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/TaskJob.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/TaskRegistry.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/TaskStatusPrintSystem.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/TaskSubmissionSink.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/TaskWorker.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/WorkerCompletionSystem.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/WorkerLookup.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/WorkerStatusPrintSystem.cpp
    BootstrapTest.cpp
    ReplayIntegrationTest.cpp
    TaskDispatchSystemTest.cpp
    TaskJobTest.cpp
    TaskRegistryTest.cpp
    TaskStatusPrintSystemTest.cpp
    TaskSubmissionSinkTest.cpp
    WorkerCompletionSystemTest.cpp
    WorkerStatusPrintSystemTest.cpp
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
        gtest::gtest
)

include(GoogleTest)
gtest_discover_tests(antwika_task_worker_tests)

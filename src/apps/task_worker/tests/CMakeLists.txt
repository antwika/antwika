find_package(GTest REQUIRED)

add_executable(antwika_task_worker_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/Messages.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/PoolScene.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/PoolSnapshot.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/RenderSystem.cpp
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
    MessagesTest.cpp
    PoolSceneTest.cpp
    PoolSnapshotTest.cpp
    RenderSystemTest.cpp
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
        antwika::ecs_commons
        antwika::engine
        antwika::gfx
        antwika::gfx::tests::mocks
        antwika::i18n
        antwika::i18n::tests::conformance
        antwika::log::tests::mocks
        antwika::replay
        antwika::scheduler
        antwika::scheduler::tests::mocks
        antwika::simulation
        antwika::event::tests::mocks
        gtest::gtest
)

antwika_bundle_test(TARGET antwika_task_worker_tests)

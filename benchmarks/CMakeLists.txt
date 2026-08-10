add_executable(antwika_ecs_benchmark EcsBenchmark.cpp)

target_link_libraries(antwika_ecs_benchmark
    PRIVATE
        antwika::ecs
        antwika::engine
        antwika::event
        antwika::gfx
        antwika::log
        antwika::time
)

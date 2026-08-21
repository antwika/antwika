add_executable(antwika_ecs_benchmark EcsBenchmark.cpp)

target_link_libraries(antwika_ecs_benchmark
    PRIVATE
        antwika::ecs
        antwika::engine
        antwika::event
        antwika::log
        antwika::time
)

add_executable(antwika_worldgen_benchmark WorldgenBenchmark.cpp)

target_link_libraries(antwika_worldgen_benchmark
    PRIVATE
        antwika::voxel
        antwika::worldgen
)

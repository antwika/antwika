add_library(antwika_ecs_commons INTERFACE)

add_library(antwika::ecs_commons ALIAS antwika_ecs_commons)

target_include_directories(antwika_ecs_commons
    INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_link_libraries(antwika_ecs_commons
    INTERFACE
        antwika::ecs
        antwika::time
)

if(BUILD_TESTING AND NOT CMAKE_CROSSCOMPILING)
    add_subdirectory(tests)
endif()

install(
    TARGETS antwika_ecs_commons
    EXPORT antwika_ecs_commonsTargets
)

install(
    DIRECTORY include/
    DESTINATION include
)

install(
    EXPORT antwika_ecs_commonsTargets
    NAMESPACE antwika::
    DESTINATION lib/cmake/antwika
)

# The vocabulary half of the ECS: what an app names its entities with,
# and the gate it runs a system behind.
#
# Header-only, and that is a consequence rather than a design: the
# three systems that had .cpp files -- MovementSystem, LifetimeSystem,
# PeriodicSystem -- along with Velocity, Lifetime, Tag, GridPosition
# and the error type only PeriodicSystem threw, were pruned once it
# was clear no application in the tree had ever called one.
# They were an offering nobody took up, and a library is not a place
# to keep code warm on the chance somebody wants it: git remembers.
#
# What is left is what is actually used -- Name, by twelve files, and
# GatedSystem, by the three per-app gating decorators -- so there is
# nothing to compile and the elaborate "compile the sources in rather
# than link them" arrangement this file used to carry went with them.
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

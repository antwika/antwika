antwika_add_library(
    NAME ecs
    SOURCES
        src/ComponentKey.cpp
        src/EntityManager.cpp
        src/OpenPhase.cpp
        src/SystemScheduler.cpp
        src/World.cpp
    DEPENDS
        antwika::time
        antwika::log
)

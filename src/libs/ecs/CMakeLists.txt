antwika_add_library(
    NAME ecs
    SOURCES
        src/ComponentId.cpp
        src/EntityManager.cpp
        src/SystemScheduler.cpp
        src/World.cpp
    DEPENDS
        antwika::time
        antwika::log
)

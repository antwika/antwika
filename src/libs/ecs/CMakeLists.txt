antwika_add_library(
    NAME ecs
    SOURCES
        src/EntityManager.cpp
        src/SystemScheduler.cpp
        src/World.cpp
    DEPENDS
        antwika::time
        antwika::log
)

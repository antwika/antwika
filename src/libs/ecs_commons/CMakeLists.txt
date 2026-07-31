antwika_add_library(
    NAME ecs_commons
    SOURCES
        src/LifetimeSystem.cpp
        src/MovementSystem.cpp
        src/PeriodicSystem.cpp
    DEPENDS
        antwika::ecs
        antwika::time
)

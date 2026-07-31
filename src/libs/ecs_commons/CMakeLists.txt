# Named once, because tests/CMakeLists.txt compiles this same list in
# rather than linking the library -- see the comment there for why.
# A source added to one list and not the other would still build, link
# and pass, while being neither exercised nor counted, which is exactly
# the gap compiling the sources in was adopted to close.
set(ANTWIKA_ECS_COMMONS_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/LifetimeSystem.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/MovementSystem.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/PeriodicSystem.cpp
)

antwika_add_library(
    NAME ecs_commons
    SOURCES
        ${ANTWIKA_ECS_COMMONS_SOURCES}
    DEPENDS
        antwika::ecs
        antwika::time
)

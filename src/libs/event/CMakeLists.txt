antwika_add_library(
    NAME event
    SOURCES
        src/EventDispatcher.cpp
        src/TickEventRecorder.cpp
        src/TickedEventDispatcher.cpp
    DEPENDS
        antwika::time
)

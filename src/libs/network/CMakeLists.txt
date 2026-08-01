antwika_add_library(
    NAME network
    SOURCES
        src/LoopbackBackend.cpp
        src/LoopbackHost.cpp
        src/LoopbackNetwork.cpp
        src/NullHost.cpp
        src/NullNetworkBackend.cpp
    DEPENDS
        antwika::log
)

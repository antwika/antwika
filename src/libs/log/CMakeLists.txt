antwika_add_library(
    NAME log
    SOURCES
        src/Level.cpp
        src/Logger.cpp
        src/MinimumLevelLogPolicy.cpp
        src/NullAppender.cpp
        src/PlainFormatter.cpp
        src/StreamAppender.cpp
    DEPENDS
        antwika::time
)

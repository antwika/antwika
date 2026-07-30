# The first module on cmake/AntwikaModule.cmake, as a worked example.
# The other 61 still spell the seven blocks out, and converting them is
# its own change, since it touches every module at once.
antwika_add_library(
    NAME time
    SOURCES
        src/SystemClock.cpp
        src/SystemSleeper.cpp
)

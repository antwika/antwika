antwika_add_library(
    NAME pattern
    SOURCES
        src/Combinators.cpp
        src/Controls.cpp
        src/Cycle.cpp
        src/Hap.cpp
        src/HapBuffer.cpp
        src/ParamValue.cpp
        src/Pattern.cpp
        src/Patterns.cpp
        src/Span.cpp
    DEPENDS
        antwika::rng
)

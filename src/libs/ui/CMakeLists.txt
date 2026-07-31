antwika_add_library(
    NAME ui
    SOURCES
        src/Context.cpp
        src/Dropdown.cpp
        src/Flatten.cpp
        src/Layout.cpp
        src/LayoutTree.cpp
        src/Painter.cpp
        src/Resolve.cpp
        src/Scope.cpp
        src/TextField.cpp
        src/Theme.cpp
    DEPENDS
        antwika::gfx
)

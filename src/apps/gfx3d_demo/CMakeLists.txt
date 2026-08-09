antwika_add_app(
    NAME gfx3d_demo
    SOURCES
        src/CubeMesh.cpp
        src/SpinLoop.cpp
        src/SpinScene.cpp
        src/main.cpp
    LIBS
        antwika::app
        antwika::cli
        antwika::gfx
        antwika::gfx_backend
        antwika::log
)

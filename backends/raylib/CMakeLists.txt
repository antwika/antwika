find_package(raylib REQUIRED)

add_library(antwika_gfx_backend
    src/RaylibBackend.cpp
    src/RaylibRenderer.cpp
    src/RaylibTexture.cpp
    src/RaylibWindow.cpp
    src/SelectedBackend.cpp
)

add_library(antwika::gfx_backend ALIAS antwika_gfx_backend)

set_target_properties(antwika_gfx_backend PROPERTIES
    WINDOWS_EXPORT_ALL_SYMBOLS ON
)

target_link_libraries(antwika_gfx_backend
    PUBLIC
        antwika::gfx
    PRIVATE
        raylib
)

if(BUILD_TESTING AND NOT CMAKE_CROSSCOMPILING)
    add_subdirectory(tests)
endif()

install(
    TARGETS antwika_gfx_backend
    EXPORT antwika_gfx_backendTargets
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
)

install(
    EXPORT antwika_gfx_backendTargets
    NAMESPACE antwika::
    DESTINATION lib/cmake/antwika
)

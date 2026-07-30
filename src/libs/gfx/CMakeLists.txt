add_library(antwika_gfx
    src/Glyphs.cpp
    src/NullBackend.cpp
    src/NullRenderer.cpp
    src/NullWindow.cpp
    src/TextLayout.cpp
)

add_library(antwika::gfx ALIAS antwika_gfx)

set_target_properties(antwika_gfx PROPERTIES
    WINDOWS_EXPORT_ALL_SYMBOLS ON
)

target_include_directories(antwika_gfx
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_link_libraries(antwika_gfx
    PUBLIC
        antwika::log
)

if(BUILD_TESTING AND NOT CMAKE_CROSSCOMPILING)
    add_subdirectory(tests)
endif()

install(
    TARGETS antwika_gfx
    EXPORT antwika_gfxTargets
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
)

install(
    DIRECTORY include/
    DESTINATION include
)

install(
    EXPORT antwika_gfxTargets
    NAMESPACE antwika::
    DESTINATION lib/cmake/antwika
)

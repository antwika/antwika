find_package(stb REQUIRED)

add_library(antwika_font
    src/Coverage.cpp
    src/Font.cpp
    src/FontDirectory.cpp
    src/GlyphAtlas.cpp
    src/StbTrueType.cpp
    src/TtfReader.cpp
)

add_library(antwika::font ALIAS antwika_font)

set_source_files_properties(src/StbTrueType.cpp PROPERTIES
    COMPILE_OPTIONS "-w"
)

set_target_properties(antwika_font PROPERTIES
    WINDOWS_EXPORT_ALL_SYMBOLS ON
)

target_include_directories(antwika_font
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_link_libraries(antwika_font
    PRIVATE
        stb::stb
)

if(BUILD_TESTING AND NOT CMAKE_CROSSCOMPILING)
    add_subdirectory(tests)
endif()

install(
    TARGETS antwika_font
    EXPORT antwika_fontTargets
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
)

install(
    DIRECTORY include/
    DESTINATION include
)

install(
    EXPORT antwika_fontTargets
    NAMESPACE antwika::
    DESTINATION lib/cmake/antwika
)

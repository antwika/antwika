# Not on antwika_add_library, for antwika::gfx's reason: stb arrives as
# a private dependency and one source file is compiled with warnings
# off.
# The function writes the blocks a module has nothing to say about, and
# ttf has two things to say.
find_package(stb REQUIRED)

add_library(antwika_ttf
    src/Coverage.cpp
    src/Font.cpp
    src/FontDirectory.cpp
    src/GlyphAtlas.cpp
    src/StbTrueType.cpp
    src/TtfReader.cpp
)

add_library(antwika::ttf ALIAS antwika_ttf)

# -w wins over the project's -Werror whichever order they land in.
# A warning that was never emitted cannot be promoted to an error.
#
# It is needed here for a reason stb_image did not have: STBTT_STATIC
# makes every entry point static, and the ones this library never calls
# are then unused functions, which -Wunused-function reports on every
# build.
# Silencing a whole file is why StbTrueType.cpp holds nothing but
# one-call wrappers -- every decision this library makes is in a source
# the compiler is still allowed to complain about.
set_source_files_properties(src/StbTrueType.cpp PROPERTIES
    COMPILE_OPTIONS "-w"
)

set_target_properties(antwika_ttf PROPERTIES
    WINDOWS_EXPORT_ALL_SYMBOLS ON
)

target_include_directories(antwika_ttf
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

# No PUBLIC link at all, which is the library's shape rather than an
# omission: antwika::ttf names no other module of this project, exactly
# as antwika::animation and antwika::wfc name none.
target_link_libraries(antwika_ttf
    PRIVATE
        stb::stb
)

if(BUILD_TESTING AND NOT CMAKE_CROSSCOMPILING)
    add_subdirectory(tests)
endif()

install(
    TARGETS antwika_ttf
    EXPORT antwika_ttfTargets
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
)

install(
    DIRECTORY include/
    DESTINATION include
)

install(
    EXPORT antwika_ttfTargets
    NAMESPACE antwika::
    DESTINATION lib/cmake/antwika
)

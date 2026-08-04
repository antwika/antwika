# Not on antwika_add_library: stb arrives as a private dependency with
# its own compile definitions, and one source file is compiled with
# warnings off.
# The function writes the blocks a module has nothing to say about, and
# gfx has three things to say.
find_package(stb REQUIRED)

# glm is header-only and PUBLIC: Math3D.hpp aliases its vector and
# matrix types rather than wrapping them, so anything including a gfx
# 3D header includes glm's.
find_package(glm REQUIRED)

# The one font every backend draws text from, compiled into the library.
# antwika::gfx opens no files and cannot ask antwika::app where one
# lives, since app depends on gfx and not the other way round, so the
# bytes arrive the only way that leaves nothing to find at run time.
include(AntwikaEmbedBinary)

set(ANTWIKA_GFX_FONT_SOURCE
    ${CMAKE_CURRENT_BINARY_DIR}/generated/BuiltInFontBytes.cpp
)

antwika_embed_binary(
    INPUT ${CMAKE_SOURCE_DIR}/assets/fonts/RobotoMono-Regular.ttf
    OUTPUT ${ANTWIKA_GFX_FONT_SOURCE}
    NAMESPACE antwika::gfx::detail
    SYMBOL kBuiltInFontBytes
)

add_library(antwika_gfx
    ${ANTWIKA_GFX_FONT_SOURCE}
    src/AtlasText.cpp
    src/Bitmap.cpp
    src/Blit.cpp
    src/BuiltInFont.cpp
    src/Camera3D.cpp
    src/GlyphCells.cpp
    src/MeshData.cpp
    src/NullBackend.cpp
    src/NullMesh.cpp
    src/NullRenderer.cpp
    src/NullTexture.cpp
    src/NullWindow.cpp
    src/PngReader.cpp
    src/PngWriter.cpp
    src/StbImage.cpp
    src/StbImageWrite.cpp
    src/TextLayout.cpp
    src/Transform.cpp
    src/Viewport.cpp
    src/ViewportRenderer.cpp
)

add_library(antwika::gfx ALIAS antwika_gfx)

# -w wins over the project's -Werror whichever order they land in.
# A warning that was never emitted cannot be promoted to an error.
set_source_files_properties(src/StbImage.cpp src/StbImageWrite.cpp
    PROPERTIES
        COMPILE_OPTIONS "-w"
)

set_target_properties(antwika_gfx PROPERTIES
    WINDOWS_EXPORT_ALL_SYMBOLS ON
)

target_include_directories(antwika_gfx
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

# antwika::font is PUBLIC because AtlasText.hpp names font::GlyphAtlas
# in its signatures, so anything including it includes that library's
# headers.
# The direction is the only one available: antwika::font names no
# module of this project at all, and has no DEPENDS line to name one
# with, so gfx -> font is acyclic by construction rather than by
# agreement.
target_link_libraries(antwika_gfx
    PUBLIC
        antwika::font
        antwika::geometry
        antwika::log
        glm::glm
    PRIVATE
        stb::stb
)

# STBI_ONLY_PNG makes "textures come from PNG files" true at the linker
# rather than by convention.
# STBI_NO_STDIO removes the path-taking entry points, so nothing in
# antwika::gfx can open a file even by accident.
target_compile_definitions(antwika_gfx
    PRIVATE
        STBI_ONLY_PNG
        STBI_NO_STDIO
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

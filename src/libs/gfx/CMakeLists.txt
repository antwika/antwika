find_package(stb REQUIRED)

find_package(glm REQUIRED)

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
    src/GlyphSheet.cpp
    src/GlyphSheetTextures.cpp
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

target_link_libraries(antwika_gfx
    PUBLIC
        antwika::font
        antwika::geometry
        antwika::log
        glm::glm
    PRIVATE
        stb::stb
)

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

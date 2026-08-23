include(AntwikaEmbedBinary)

set(ANTWIKA_TEXT_FONT_SOURCE
    ${CMAKE_CURRENT_BINARY_DIR}/generated/BuiltInFontBytes.cpp
)

antwika_embed_binary(
    INPUT ${CMAKE_SOURCE_DIR}/assets/fonts/RobotoMono-Regular.ttf
    OUTPUT ${ANTWIKA_TEXT_FONT_SOURCE}
    NAMESPACE antwika::text::detail
    SYMBOL kBuiltInFontBytes
)

antwika_add_library(
    NAME text
    SOURCES
        ${ANTWIKA_TEXT_FONT_SOURCE}
        src/AtlasText.cpp
        src/BuiltInFont.cpp
        src/GlyphAtlas.cpp
        src/GlyphAtlasTextures.cpp
        src/GlyphCells.cpp
        src/TextLayout.cpp
    DEPENDS
        antwika::font
        antwika::gfx
)

target_include_directories(antwika_text
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

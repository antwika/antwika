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

add_library(antwika_gfx
    src/Bitmap.cpp
    src/Blit.cpp
    src/Camera3D.cpp
    src/Glyphs.cpp
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

target_link_libraries(antwika_gfx
    PUBLIC
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

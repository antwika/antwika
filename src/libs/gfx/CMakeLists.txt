find_package(glm REQUIRED)

add_library(antwika_gfx
    src/Bitmap.cpp
    src/Blit.cpp
    src/Camera3D.cpp
    src/ClipScope.cpp
    src/ColorSpace.cpp
    src/CubeFace.cpp
    src/ForwardingRenderer.cpp
    src/MeshBox.cpp
    src/MeshData.cpp
    src/NullBackend.cpp
    src/NullMesh.cpp
    src/NullRenderTarget.cpp
    src/NullRenderer.cpp
    src/NullShader.cpp
    src/NullTexture.cpp
    src/NullWindow.cpp
    src/ShaderReader.cpp
    src/ShaderSource.cpp
    src/TargetScope.cpp
    src/Transform.cpp
    src/TransformScope.cpp
    src/Viewport.cpp
    src/ViewportRenderer.cpp
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
        antwika::geometry
        antwika::log
        glm::glm
    PRIVATE
        antwika::io
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

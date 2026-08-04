# Points, sizes and rectangles, and nothing else.
#
# They lived in antwika::gfx, which meant that every module wanting a
# width and a height linked stb, glm and an embedded TrueType font to
# get one -- antwika::replay records a canvas size, and antwika::config
# inherited the whole graph through it.
#
# Header-only and dependency-free, on antwika::time's terms: a value
# type with no behaviour beyond comparing equal has nothing to compile.
add_library(antwika_geometry INTERFACE)

add_library(antwika::geometry ALIAS antwika_geometry)

target_include_directories(antwika_geometry
    INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

install(
    TARGETS antwika_geometry
    EXPORT antwika_geometryTargets
)

install(
    DIRECTORY include/
    DESTINATION include
)

install(
    EXPORT antwika_geometryTargets
    NAMESPACE antwika::
    DESTINATION lib/cmake/antwika
)

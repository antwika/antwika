find_package(stb REQUIRED)

antwika_add_library(
    NAME image
    SOURCES
        src/PngFile.cpp
        src/PngReader.cpp
        src/PngWriter.cpp
        src/StbImage.cpp
        src/StbImageWrite.cpp
    DEPENDS
        antwika::gfx
        antwika::io
)

set_source_files_properties(src/StbImage.cpp src/StbImageWrite.cpp
    PROPERTIES
        COMPILE_OPTIONS "-w"
)

target_link_libraries(antwika_image
    PRIVATE
        stb::stb
)

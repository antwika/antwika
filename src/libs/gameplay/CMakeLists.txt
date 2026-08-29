set(ANTWIKA_GAMEPLAY_SEAM_STAMP
    ${CMAKE_CURRENT_BINARY_DIR}/generated/antwika/gameplay/SeamStamp.hpp
)

file(GLOB_RECURSE ANTWIKA_GAMEPLAY_SEAM_HEADERS
    CONFIGURE_DEPENDS
    ${CMAKE_SOURCE_DIR}/src/libs/*/include/*.hpp
)

add_custom_command(
    OUTPUT ${ANTWIKA_GAMEPLAY_SEAM_STAMP}
    COMMAND ${CMAKE_COMMAND} -E env python3
        ${CMAKE_SOURCE_DIR}/scripts/stamp_seam.py
        --root ${CMAKE_SOURCE_DIR}
        --header
            ${CMAKE_CURRENT_SOURCE_DIR}/include/antwika/gameplay/IGame.hpp
        --output ${ANTWIKA_GAMEPLAY_SEAM_STAMP}
    DEPENDS
        ${CMAKE_SOURCE_DIR}/scripts/stamp_seam.py
        ${ANTWIKA_GAMEPLAY_SEAM_HEADERS}
)

add_custom_target(antwika_gameplay_seam_stamp
    DEPENDS ${ANTWIKA_GAMEPLAY_SEAM_STAMP}
)

antwika_add_library(
    NAME gameplay
    SOURCES
        src/Characters.cpp
        src/ComponentNames.cpp
        src/Game.cpp
        src/GameLoop.cpp
        src/Module.cpp
        src/PadReports.cpp
        src/SpawnSystem.cpp
    DEPENDS
        antwika::camera
        antwika::character
        antwika::component
        antwika::ecs
        antwika::enums
        antwika::gfx
        antwika::light
        antwika::loadout
        antwika::log
        antwika::map
        antwika::rules
        antwika::system
        antwika::time
        antwika::voxel
        antwika::collision
)

if(ANTWIKA_GAME_SHARED)
    add_library(antwika_gameplay_module MODULE src/Module.cpp)

    target_link_libraries(antwika_gameplay_module
        PRIVATE
            antwika::gameplay
    )

    set_target_properties(antwika_gameplay_module PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY
            "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/antwika_editor"
        WINDOWS_EXPORT_ALL_SYMBOLS ON
    )

    install(TARGETS antwika_gameplay_module
        LIBRARY DESTINATION "bin/antwika_editor"
    )
endif()

add_dependencies(antwika_gameplay antwika_gameplay_seam_stamp)

target_include_directories(antwika_gameplay
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/generated>
)

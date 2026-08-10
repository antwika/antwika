antwika_add_app(
    NAME mapcheck_cli
    SOURCES
        src/CheckMaps.cpp
        src/main.cpp
    LIBS
        antwika::app
        antwika::cli
        antwika::log
        antwika::mapcheck
        antwika::tilemap
)

if(BUILD_TESTING AND NOT CMAKE_CROSSCOMPILING)
    file(GLOB antwika_map_assets
        "${CMAKE_SOURCE_DIR}/assets/maps/*.json")

    if(antwika_map_assets)
        add_test(
            NAME map_assets_validate
            COMMAND antwika_mapcheck_cli ${antwika_map_assets})
    endif()
endif()

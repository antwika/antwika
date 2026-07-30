# The seven blocks every module's CMakeLists.txt repeats.
# 62 of them differ only in a name, a source list and a dependency list,
# so anything cross-cutting is 62 identical edits with nothing checking
# that none was missed.
# A module that genuinely differs -- gfx's stb, a backend's framework
# link, an app's baked-in asset path -- keeps writing its own rules, and
# then the difference is visible for the right reason.

# Defines antwika_<NAME>, aliased to antwika::<NAME>, from SOURCES.
# DEPENDS is linked PUBLIC, since a module's headers are what its
# dependants include.
# tests/ is added when there is one and the build is testing.
function(antwika_add_library)
    cmake_parse_arguments(PARSE_ARGV 0 ARG "" "NAME" "SOURCES;DEPENDS")

    if(NOT ARG_NAME)
        message(FATAL_ERROR "antwika_add_library: NAME is required")
    endif()

    if(NOT ARG_SOURCES)
        message(FATAL_ERROR
            "antwika_add_library(${ARG_NAME}): SOURCES is required")
    endif()

    if(ARG_UNPARSED_ARGUMENTS)
        message(FATAL_ERROR
            "antwika_add_library(${ARG_NAME}): unrecognised arguments "
            "'${ARG_UNPARSED_ARGUMENTS}'")
    endif()

    set(target antwika_${ARG_NAME})

    add_library(${target} ${ARG_SOURCES})
    add_library(antwika::${ARG_NAME} ALIAS ${target})

    set_target_properties(${target} PROPERTIES
        WINDOWS_EXPORT_ALL_SYMBOLS ON
    )

    target_include_directories(${target}
        PUBLIC
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
            $<INSTALL_INTERFACE:include>
    )

    if(ARG_DEPENDS)
        target_link_libraries(${target}
            PUBLIC
                ${ARG_DEPENDS}
        )
    endif()

    if(BUILD_TESTING AND NOT CMAKE_CROSSCOMPILING
            AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/tests/CMakeLists.txt")
        add_subdirectory(tests)
    endif()

    install(
        TARGETS ${target}
        EXPORT ${target}Targets
        ARCHIVE DESTINATION lib
        LIBRARY DESTINATION lib
        RUNTIME DESTINATION bin
    )

    install(
        DIRECTORY include/
        DESTINATION include
    )

    install(
        EXPORT ${target}Targets
        NAMESPACE antwika::
        DESTINATION lib/cmake/antwika
    )
endfunction()

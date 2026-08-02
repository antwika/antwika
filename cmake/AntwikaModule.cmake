# The seven blocks every module's CMakeLists.txt repeats.
# 62 of them differ only in a name, a source list and a dependency list,
# so anything cross-cutting is 62 identical edits with nothing checking
# that none was missed.
# A module that genuinely differs -- gfx's stb, a backend's framework
# link -- keeps writing its own rules, and then the difference is
# visible for the right reason.

# The runtime a MinGW build's executables need standing beside them.
# Found once here rather than by each application repeating the same
# three lookups, which is what nine of them were doing.
if(MINGW)
    set(ANTWIKA_MINGW_RUNTIME_DLLS "")

    foreach(name
        libgcc_s_seh-1.dll
        libstdc++-6.dll
        libwinpthread-1.dll
    )
        execute_process(
            COMMAND ${CMAKE_CXX_COMPILER} -print-file-name=${name}
            OUTPUT_VARIABLE found
            OUTPUT_STRIP_TRAILING_WHITESPACE
        )

        if(EXISTS "${found}")
            list(APPEND ANTWIKA_MINGW_RUNTIME_DLLS "${found}")
        endif()
    endforeach()
endif()

# Gives an application a directory of its own under bin/, holding the
# executable, everything it opens and everything it needs to start.
#
# One shared bin/ was fine while an application was a single file and
# stopped being fine the moment one had to open something beside it:
# nine applications in one directory is nine applications sharing one
# atlas.png, and two of them do have one.
#
# ASSETS are copied there rather than named at configure time, which is
# the substantive change.  A path baked into the binary is the building
# machine's, and that is the running machine's path right up until it is
# not -- a cross build's never was, so every MinGW executable that
# opened anything died on its first line looking for a directory that
# exists only inside the container that built it.
# The counterpart is antwika::app's assetPath(), which asks where the
# executable is rather than where the working directory is, so starting
# one from somewhere else still works.
function(antwika_bundle_app)
    cmake_parse_arguments(PARSE_ARGV 0 ARG "" "TARGET" "ASSETS")

    if(NOT ARG_TARGET)
        message(FATAL_ERROR "antwika_bundle_app: TARGET is required")
    endif()

    if(ARG_UNPARSED_ARGUMENTS)
        message(FATAL_ERROR
            "antwika_bundle_app(${ARG_TARGET}): unrecognised arguments "
            "'${ARG_UNPARSED_ARGUMENTS}'")
    endif()

    set_target_properties(${ARG_TARGET} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY
            "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${ARG_TARGET}"
    )

    # An asset named here and missing is a build that fails now rather
    # than an application that starts and cannot draw.
    #
    # Each copy is a rule that DEPENDS on its source, gathered into a
    # target the application depends on -- never a POST_BUILD step.
    # POST_BUILD runs only when the executable relinks, and editing an
    # asset touches no source: the documented repaint-the-atlas
    # workflow rebuilt everything and ran against the previous sheet,
    # silently, through app::assetPath().
    set(copies "")

    foreach(asset IN LISTS ARG_ASSETS)
        set(source "${CMAKE_CURRENT_SOURCE_DIR}/${asset}")

        if(NOT EXISTS "${source}")
            message(FATAL_ERROR
                "antwika_bundle_app(${ARG_TARGET}): no such asset "
                "'${source}'")
        endif()

        get_filename_component(name "${asset}" NAME)
        set(copied
            "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${ARG_TARGET}/${name}")

        add_custom_command(OUTPUT "${copied}"
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${source}"
                "${copied}"
            DEPENDS "${source}"
        )

        list(APPEND copies "${copied}")
    endforeach()

    if(copies)
        add_custom_target(${ARG_TARGET}_assets DEPENDS ${copies})
        add_dependencies(${ARG_TARGET} ${ARG_TARGET}_assets)
    endif()

    foreach(dll IN LISTS ANTWIKA_MINGW_RUNTIME_DLLS)
        add_custom_command(TARGET ${ARG_TARGET} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${dll}"
                "$<TARGET_FILE_DIR:${ARG_TARGET}>"
        )
    endforeach()
endfunction()

# A test executable lands in the directory of the module that owns it,
# and that directory is the target's own name with the trailing _tests
# taken off.
# So an application's suite sits beside the executable
# antwika_bundle_app() put there, and a library's gets a directory named
# after the library -- one rule, applied to every suite in the tree,
# rather than applications in directories and every test binary loose in
# bin/ beside them.
#
# The directory is derived from the target rather than named as a second
# argument, which is what keeps the two from ever disagreeing.
#
# Registering the cases with CTest happens here as well, because moving
# a binary and telling CTest where to find it are one decision: leaving
# the second in every tests/CMakeLists.txt is exactly the drift that
# having one home for the rule prevents.
function(antwika_bundle_test)
    cmake_parse_arguments(PARSE_ARGV 0 ARG "" "TARGET" "")

    if(NOT ARG_TARGET)
        message(FATAL_ERROR "antwika_bundle_test: TARGET is required")
    endif()

    if(ARG_UNPARSED_ARGUMENTS)
        message(FATAL_ERROR
            "antwika_bundle_test(${ARG_TARGET}): unrecognised arguments "
            "'${ARG_UNPARSED_ARGUMENTS}'")
    endif()

    # The name is what says which module owns the suite, so a target
    # that does not follow the convention has no directory to go to and
    # is refused now rather than landing somewhere surprising.
    if(NOT ARG_TARGET MATCHES "^(.+)_tests$")
        message(FATAL_ERROR
            "antwika_bundle_test(${ARG_TARGET}): a test target's name "
            "must end in '_tests'")
    endif()

    set_target_properties(${ARG_TARGET} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY
            "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${CMAKE_MATCH_1}"
    )

    include(GoogleTest)
    gtest_discover_tests(${ARG_TARGET})
endfunction()

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

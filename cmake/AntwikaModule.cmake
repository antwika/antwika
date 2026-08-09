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

set(ANTWIKA_NOTICE_SOURCES
    "${CMAKE_SOURCE_DIR}/LICENSE"
    "${CMAKE_SOURCE_DIR}/NOTICE"
    "${CMAKE_SOURCE_DIR}/THIRD_PARTY.txt"
    "${CMAKE_SOURCE_DIR}/assets/fonts/LICENSE.txt"
)

set(ANTWIKA_NOTICE_NAMES
    LICENSE
    NOTICE
    THIRD_PARTY.txt
    RobotoMono-OFL.txt
)

function(antwika_install_notices destination)
    list(LENGTH ANTWIKA_NOTICE_SOURCES count)
    math(EXPR last "${count} - 1")

    foreach(index RANGE ${last})
        list(GET ANTWIKA_NOTICE_SOURCES ${index} source)
        list(GET ANTWIKA_NOTICE_NAMES ${index} name)

        install(FILES "${source}"
            DESTINATION "${destination}"
            RENAME "${name}"
        )
    endforeach()
endfunction()

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

    set(destination "bin/${ARG_TARGET}")

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

        install(FILES "${source}" DESTINATION "${destination}")

        list(APPEND copies "${copied}")
    endforeach()

    list(LENGTH ANTWIKA_NOTICE_SOURCES notice_count)
    math(EXPR last_notice "${notice_count} - 1")

    foreach(index RANGE ${last_notice})
        list(GET ANTWIKA_NOTICE_SOURCES ${index} source)
        list(GET ANTWIKA_NOTICE_NAMES ${index} name)

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

        install(FILES "${dll}" DESTINATION "${destination}")
    endforeach()

    antwika_install_notices("${destination}")

    install(TARGETS ${ARG_TARGET}
        RUNTIME DESTINATION "${destination}"
    )
endfunction()

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

function(antwika_add_app)
    cmake_parse_arguments(PARSE_ARGV 0 ARG "" "NAME" "SOURCES;LIBS;ASSETS")

    if(NOT ARG_NAME)
        message(FATAL_ERROR "antwika_add_app: NAME is required")
    endif()

    if(NOT ARG_SOURCES)
        message(FATAL_ERROR
            "antwika_add_app(${ARG_NAME}): SOURCES is required")
    endif()

    if(ARG_UNPARSED_ARGUMENTS)
        message(FATAL_ERROR
            "antwika_add_app(${ARG_NAME}): unrecognised arguments "
            "'${ARG_UNPARSED_ARGUMENTS}'")
    endif()

    set(target antwika_${ARG_NAME})

    add_executable(${target} ${ARG_SOURCES})

    set(own ${ARG_SOURCES})
    list(REMOVE_ITEM own src/main.cpp)
    set_property(TARGET ${target} PROPERTY ANTWIKA_APP_SOURCES ${own})
    set_property(TARGET ${target} PROPERTY ANTWIKA_APP_LIBS ${ARG_LIBS})

    target_include_directories(${target}
        PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/include
    )

    if(ARG_LIBS)
        target_link_libraries(${target}
            PRIVATE
                ${ARG_LIBS}
        )
    endif()

    if(BUILD_TESTING AND NOT CMAKE_CROSSCOMPILING
            AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/tests/CMakeLists.txt")
        add_subdirectory(tests)
    endif()

    if(ARG_ASSETS)
        antwika_bundle_app(TARGET ${target} ASSETS ${ARG_ASSETS})
    else()
        antwika_bundle_app(TARGET ${target})
    endif()
endfunction()

function(antwika_add_app_tests)
    cmake_parse_arguments(PARSE_ARGV 0 ARG "" "APP"
        "TESTS;LIBS;EXTRA_SOURCES;EXTRA_INCLUDES")

    if(NOT ARG_APP)
        message(FATAL_ERROR "antwika_add_app_tests: APP is required")
    endif()

    if(NOT ARG_TESTS)
        message(FATAL_ERROR
            "antwika_add_app_tests(${ARG_APP}): TESTS is required")
    endif()

    if(ARG_UNPARSED_ARGUMENTS)
        message(FATAL_ERROR
            "antwika_add_app_tests(${ARG_APP}): unrecognised arguments "
            "'${ARG_UNPARSED_ARGUMENTS}'")
    endif()

    find_package(GTest REQUIRED)

    set(app antwika_${ARG_APP})
    set(target ${app}_tests)

    get_target_property(app_sources ${app} ANTWIKA_APP_SOURCES)

    set(sources "")
    foreach(one IN LISTS app_sources)
        list(APPEND sources "${CMAKE_CURRENT_SOURCE_DIR}/../${one}")
    endforeach()

    add_executable(${target} ${sources} ${ARG_EXTRA_SOURCES} ${ARG_TESTS})

    target_include_directories(${target}
        PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/../include
            ${CMAKE_CURRENT_SOURCE_DIR}/../src
            ${CMAKE_CURRENT_SOURCE_DIR}
            ${ARG_EXTRA_INCLUDES}
    )

    if(ARG_LIBS)
        target_link_libraries(${target}
            PRIVATE
                ${ARG_LIBS}
        )
    endif()

    antwika_bundle_test(TARGET ${target})
endfunction()

function(antwika_add_test_support)
    cmake_parse_arguments(PARSE_ARGV 0 ARG "" "MODULE;KIND" "DEPENDS")

    if(NOT ARG_MODULE OR NOT ARG_KIND)
        message(FATAL_ERROR
            "antwika_add_test_support: MODULE and KIND are required")
    endif()

    if(ARG_UNPARSED_ARGUMENTS)
        message(FATAL_ERROR
            "antwika_add_test_support(${ARG_MODULE}): unrecognised "
            "arguments '${ARG_UNPARSED_ARGUMENTS}'")
    endif()

    set(target antwika_${ARG_MODULE}_tests_${ARG_KIND})

    add_library(${target} INTERFACE)
    add_library(antwika::${ARG_MODULE}::tests::${ARG_KIND} ALIAS ${target})

    target_include_directories(${target}
        INTERFACE
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
            $<INSTALL_INTERFACE:include>
    )

    target_link_libraries(${target}
        INTERFACE
            antwika::${ARG_MODULE}
            ${ARG_DEPENDS}
    )

    install(
        TARGETS ${target}
        EXPORT antwika_${ARG_MODULE}Targets
    )

    install(
        DIRECTORY include/
        DESTINATION include
    )
endfunction()

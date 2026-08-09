file(GLOB ANTWIKA_BACKEND_FILES
    RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/*/CMakeLists.txt")

set(ANTWIKA_BACKENDS "")

foreach(BACKEND_FILE ${ANTWIKA_BACKEND_FILES})
    get_filename_component(BACKEND_DIR ${BACKEND_FILE} DIRECTORY)
    list(APPEND ANTWIKA_BACKENDS ${BACKEND_DIR})
endforeach()

list(JOIN ANTWIKA_BACKENDS ", " ANTWIKA_BACKENDS_TEXT)

foreach(SELECTION
        ANTWIKA_GFX_BACKEND
        ANTWIKA_INPUT_BACKEND
        ANTWIKA_SOUND_BACKEND
        ANTWIKA_NETWORK_BACKEND)
    if(NOT IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/${${SELECTION}}")
        message(FATAL_ERROR
            "Unknown ${SELECTION} '${${SELECTION}}'. "
            "Available: ${ANTWIKA_BACKENDS_TEXT}")
    endif()
endforeach()

set(ANTWIKA_REAL_BACKENDS "")

foreach(SELECTION
        ANTWIKA_GFX_BACKEND
        ANTWIKA_INPUT_BACKEND
        ANTWIKA_SOUND_BACKEND
        ANTWIKA_NETWORK_BACKEND)
    if(NOT ${SELECTION} STREQUAL "null"
       AND NOT ${SELECTION} STREQUAL "sockets")
        list(APPEND ANTWIKA_REAL_BACKENDS ${${SELECTION}})
    endif()
endforeach()

list(REMOVE_DUPLICATES ANTWIKA_REAL_BACKENDS)
list(LENGTH ANTWIKA_REAL_BACKENDS ANTWIKA_REAL_BACKEND_COUNT)

if(ANTWIKA_REAL_BACKEND_COUNT GREATER 1)
    list(JOIN ANTWIKA_REAL_BACKENDS ", " ANTWIKA_REAL_BACKENDS_TEXT)

    message(FATAL_ERROR
        "ANTWIKA_GFX_BACKEND '${ANTWIKA_GFX_BACKEND}', "
        "ANTWIKA_INPUT_BACKEND '${ANTWIKA_INPUT_BACKEND}', "
        "ANTWIKA_SOUND_BACKEND '${ANTWIKA_SOUND_BACKEND}' and "
        "ANTWIKA_NETWORK_BACKEND '${ANTWIKA_NETWORK_BACKEND}' name more "
        "than one framework (${ANTWIKA_REAL_BACKENDS_TEXT}). "
        "Use one framework per build, or 'null' for the subsystems "
        "that should opt out.")
endif()

set(ANTWIKA_SELECTED_BACKENDS
    ${ANTWIKA_GFX_BACKEND}
    ${ANTWIKA_INPUT_BACKEND}
    ${ANTWIKA_SOUND_BACKEND}
    ${ANTWIKA_NETWORK_BACKEND})

list(REMOVE_DUPLICATES ANTWIKA_SELECTED_BACKENDS)

foreach(BACKEND ${ANTWIKA_SELECTED_BACKENDS})
    add_subdirectory(${BACKEND})
endforeach()

foreach(SUBSYSTEM gfx input sound network)
    string(TOUPPER ${SUBSYSTEM} SUBSYSTEM_UPPER)

    if(NOT TARGET antwika_${SUBSYSTEM}_backend)
        message(FATAL_ERROR
            "ANTWIKA_${SUBSYSTEM_UPPER}_BACKEND names "
            "'${ANTWIKA_${SUBSYSTEM_UPPER}_BACKEND}', which builds no "
            "${SUBSYSTEM} backend. That directory does not implement "
            "this subsystem.")
    endif()
endforeach()

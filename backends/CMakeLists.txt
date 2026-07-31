# Exactly one backend is compiled per subsystem, chosen by
# ANTWIKA_GFX_BACKEND, ANTWIKA_INPUT_BACKEND and ANTWIKA_SOUND_BACKEND.
# Each subdirectory defines the same three target names,
# antwika_gfx_backend, antwika_input_backend and antwika_sound_backend,
# so apps link antwika::gfx_backend, antwika::input_backend and
# antwika::sound_backend without knowing which framework they were built
# against.
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
        ANTWIKA_SOUND_BACKEND)
    if(NOT IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/${${SELECTION}}")
        message(FATAL_ERROR
            "Unknown ${SELECTION} '${${SELECTION}}'. "
            "Available: ${ANTWIKA_BACKENDS_TEXT}")
    endif()
endforeach()

# Two real frameworks are refused for two separate reasons, and both
# matter.
#
# Graphics and input share one process-global event queue, so two
# frameworks there would compete for it and whichever polled second
# would silently lose what the other took -- a failure that is
# framework-dependent and invisible at runtime.
#
# Sound has no such queue, but naming a second framework anywhere still
# links two of them into one process, which doubles the dependency graph
# and gives two libraries a claim on the same global initialisation.
# One framework per build is the rule; 'null' is how a subsystem opts
# out of it.
set(ANTWIKA_REAL_BACKENDS "")

foreach(SELECTION
        ANTWIKA_GFX_BACKEND
        ANTWIKA_INPUT_BACKEND
        ANTWIKA_SOUND_BACKEND)
    if(NOT ${SELECTION} STREQUAL "null")
        list(APPEND ANTWIKA_REAL_BACKENDS ${${SELECTION}})
    endif()
endforeach()

list(REMOVE_DUPLICATES ANTWIKA_REAL_BACKENDS)
list(LENGTH ANTWIKA_REAL_BACKENDS ANTWIKA_REAL_BACKEND_COUNT)

if(ANTWIKA_REAL_BACKEND_COUNT GREATER 1)
    list(JOIN ANTWIKA_REAL_BACKENDS ", " ANTWIKA_REAL_BACKENDS_TEXT)

    message(FATAL_ERROR
        "ANTWIKA_GFX_BACKEND '${ANTWIKA_GFX_BACKEND}', "
        "ANTWIKA_INPUT_BACKEND '${ANTWIKA_INPUT_BACKEND}' and "
        "ANTWIKA_SOUND_BACKEND '${ANTWIKA_SOUND_BACKEND}' name more "
        "than one framework (${ANTWIKA_REAL_BACKENDS_TEXT}). "
        "Use one framework per build, or 'null' for the subsystems "
        "that should opt out.")
endif()

# A directory selected for more than one subsystem is added once, and
# guards each of its targets on which subsystem selected it.
set(ANTWIKA_SELECTED_BACKENDS
    ${ANTWIKA_GFX_BACKEND}
    ${ANTWIKA_INPUT_BACKEND}
    ${ANTWIKA_SOUND_BACKEND})

list(REMOVE_DUPLICATES ANTWIKA_SELECTED_BACKENDS)

foreach(BACKEND ${ANTWIKA_SELECTED_BACKENDS})
    add_subdirectory(${BACKEND})
endforeach()

# A selection naming a directory that does not implement that subsystem
# configures cleanly today and fails much later, at link, with an
# undefined makeSelected*Backend.  Saying so here names the actual
# mistake instead.
foreach(SUBSYSTEM gfx input sound)
    string(TOUPPER ${SUBSYSTEM} SUBSYSTEM_UPPER)

    if(NOT TARGET antwika_${SUBSYSTEM}_backend)
        message(FATAL_ERROR
            "ANTWIKA_${SUBSYSTEM_UPPER}_BACKEND names "
            "'${ANTWIKA_${SUBSYSTEM_UPPER}_BACKEND}', which builds no "
            "${SUBSYSTEM} backend. That directory does not implement "
            "this subsystem.")
    endif()
endforeach()

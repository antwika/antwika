# Exactly one backend is compiled per subsystem, chosen by
# ANTWIKA_GFX_BACKEND and ANTWIKA_INPUT_BACKEND. Each subdirectory
# defines the same two target names, antwika_gfx_backend and
# antwika_input_backend, so apps link antwika::gfx_backend and
# antwika::input_backend without knowing which framework they were built
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

foreach(SELECTION ANTWIKA_GFX_BACKEND ANTWIKA_INPUT_BACKEND)
    if(NOT IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/${${SELECTION}}")
        message(FATAL_ERROR
            "Unknown ${SELECTION} '${${SELECTION}}'. "
            "Available: ${ANTWIKA_BACKENDS_TEXT}")
    endif()
endforeach()

# Two real frameworks would compete for one process-global event queue,
# and whichever polled second would silently lose what the other took --
# see docs/history/input-plan.md. That failure is framework-dependent and
# invisible at runtime, so it is worth refusing rather than trusting.
if(NOT ANTWIKA_GFX_BACKEND STREQUAL ANTWIKA_INPUT_BACKEND
        AND NOT ANTWIKA_GFX_BACKEND STREQUAL "null"
        AND NOT ANTWIKA_INPUT_BACKEND STREQUAL "null")
    message(FATAL_ERROR
        "ANTWIKA_GFX_BACKEND '${ANTWIKA_GFX_BACKEND}' and "
        "ANTWIKA_INPUT_BACKEND '${ANTWIKA_INPUT_BACKEND}' name two "
        "different frameworks, which would fight over one event queue. "
        "Use the same backend for both, or 'null' for one of them.")
endif()

# A directory selected for both subsystems is added once, and guards its
# own targets on which subsystem selected it.
set(ANTWIKA_SELECTED_BACKENDS
    ${ANTWIKA_GFX_BACKEND}
    ${ANTWIKA_INPUT_BACKEND})

list(REMOVE_DUPLICATES ANTWIKA_SELECTED_BACKENDS)

foreach(BACKEND ${ANTWIKA_SELECTED_BACKENDS})
    add_subdirectory(${BACKEND})
endforeach()

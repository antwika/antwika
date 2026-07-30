# Exactly one backend is compiled per build, chosen by
# ANTWIKA_GFX_BACKEND. Each subdirectory defines the same target name,
# antwika_gfx_backend, so apps link antwika::gfx_backend without knowing
# which graphics framework they were built against.
if(NOT IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/${ANTWIKA_GFX_BACKEND}")
    file(GLOB ANTWIKA_GFX_BACKEND_FILES
        RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}"
        "${CMAKE_CURRENT_SOURCE_DIR}/*/CMakeLists.txt")

    set(ANTWIKA_GFX_BACKENDS "")

    foreach(BACKEND_FILE ${ANTWIKA_GFX_BACKEND_FILES})
        get_filename_component(BACKEND_DIR ${BACKEND_FILE} DIRECTORY)
        list(APPEND ANTWIKA_GFX_BACKENDS ${BACKEND_DIR})
    endforeach()

    list(JOIN ANTWIKA_GFX_BACKENDS ", " ANTWIKA_GFX_BACKENDS_TEXT)

    message(FATAL_ERROR
        "Unknown ANTWIKA_GFX_BACKEND '${ANTWIKA_GFX_BACKEND}'. "
        "Available: ${ANTWIKA_GFX_BACKENDS_TEXT}")
endif()

add_subdirectory(${ANTWIKA_GFX_BACKEND})

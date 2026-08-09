if(ANTWIKA_NETWORK_BACKEND STREQUAL "sockets")
    add_library(antwika_network_backend
        src/SelectedNetworkBackend.cpp
        src/SocketApi.cpp
        src/SocketsBackend.cpp
        src/SocketsHost.cpp
    )

    add_library(antwika::network_backend ALIAS antwika_network_backend)

    set_target_properties(antwika_network_backend PROPERTIES
        WINDOWS_EXPORT_ALL_SYMBOLS ON
    )

    target_include_directories(antwika_network_backend
        PUBLIC
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
    )

    target_link_libraries(antwika_network_backend
        PUBLIC
            antwika::network
    )

    if(WIN32)
        target_link_libraries(antwika_network_backend
            PUBLIC
                ws2_32
        )
    endif()

    install(
        TARGETS antwika_network_backend
        EXPORT antwika_network_backendTargets
        ARCHIVE DESTINATION lib
        LIBRARY DESTINATION lib
        RUNTIME DESTINATION bin
    )

    install(
        EXPORT antwika_network_backendTargets
        NAMESPACE antwika::
        DESTINATION lib/cmake/antwika
    )

    if(BUILD_TESTING AND NOT CMAKE_CROSSCOMPILING)
        add_subdirectory(tests)
    endif()
endif()

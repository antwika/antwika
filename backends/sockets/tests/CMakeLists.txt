find_package(GTest REQUIRED)

add_executable(antwika_network_backend_sockets_tests
    SocketsBackendConformanceTest.cpp
    SocketsHostTest.cpp
)

target_link_libraries(antwika_network_backend_sockets_tests
    PRIVATE
        antwika::network
        antwika::network::tests::conformance
        antwika::log::tests::mocks
        antwika_network_backend
        gtest::gtest
)

antwika_bundle_test(TARGET antwika_network_backend_sockets_tests)

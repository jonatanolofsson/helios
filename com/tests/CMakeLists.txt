make_test(DispatcherTests)
target_link_libraries(DispatcherTests os)
make_test(LocalSerialCommunicationTests)
target_link_libraries(LocalSerialCommunicationTests
    utils
    os
    ${Boost_LIBRARIES}
)
make_test(LocalNetworkCommunicationTests)
target_link_libraries(LocalNetworkCommunicationTests
    utils
    os
    ${Boost_LIBRARIES}
)

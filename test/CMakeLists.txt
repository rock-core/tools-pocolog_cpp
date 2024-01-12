if (HANDLE_OROGEN_OPAQUES)
    if (NOT OROCOS_TARGET)
        set(OROCOS_TARGET gnulinux)
    endif()

    list(APPEND OPTIONAL_TESTS test_SequentialReadDispatcher.cpp)
endif()

rock_gtest(
    pocolog_cpp_test
    suite.cpp test_LogFile.cpp test_StreamDescription.cpp
    ${OPTIONAL_TESTS}
    DEPS pocolog_cpp
)

if (HANDLE_OROGEN_OPAQUES)
    if (NOT OROCOS_TARGET)
        set(OROCOS_TARGET gnulinux)
    endif()

    list(APPEND OPTIONAL_SOURCES SequentialReadDispatcher.cpp)
    list(APPEND OPTIONAL_HEADERS SequentialReadDispatcher.hpp)
    list(APPEND OPTIONAL_DEPS_PKGCONFIG
                orocos-rtt-${OROCOS_TARGET} rtt_typelib-${OROCOS_TARGET} utilmm)
endif()

find_package( Boost COMPONENTS system filesystem program_options)
rock_library(pocolog_cpp
    SOURCES
        Format.cpp
        Read.cpp
        Write.cpp
        Index.cpp
        InputDataStream.cpp
        StreamDescription.cpp
        Stream.cpp
        LogFile.cpp
        IndexFile.cpp
        FileStream.cpp
        MultiFileIndex.cpp
        named_vector_helpers.cpp
        OwnedValue.cpp
        ${OPTIONAL_SOURCES}
    HEADERS
        FileStream.hpp
        Format.hpp
        Index.hpp
        InputDataStream.hpp
        LogFile.hpp
        MultiFileIndex.hpp
        Read.hpp
        Stream.hpp
        StreamDescription.hpp
        Write.hpp
        named_vector_helpers.hpp
        OwnedValue.hpp
        ${OPTIONAL_HEADERS}
    DEPS_PKGCONFIG
        base-types
        base-logging
        typelib
        yaml-cpp
        ${OPTIONAL_DEPS_PKGCONFIG}
    DEPS_PLAIN
        Boost_SYSTEM Boost_FILESYSTEM)

rock_executable(indexer NOINSTALL
    SOURCES indexer.cpp
    DEPS pocolog_cpp
    DEPS_PKGCONFIG base-types typelib
)

rock_executable(multiIndexer NOINSTALL
    SOURCES multiIndexTest.cpp
    DEPS pocolog_cpp
    DEPS_PKGCONFIG base-types typelib
)
rock_executable(speedTest NOINSTALL
    SOURCES speedTest.cpp
)

rock_executable(example_old NOINSTALL
    SOURCES example_old.cpp
    DEPS pocolog_cpp
    DEPS_PKGCONFIG base-types typelib
)

rock_executable(pocolog-extract
    SOURCES pocolog-extract_main.cpp named_vector_helpers.cpp csv_output.cpp
    HEADERS named_vector_helpers.hpp csv_output.hpp
    DEPS_PKGCONFIG yaml-cpp
    DEPS pocolog_cpp
    DEPS_PLAIN
        Boost_PROGRAM_OPTIONS
)

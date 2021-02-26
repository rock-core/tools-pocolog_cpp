find_package( Boost COMPONENTS system filesystem)
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
    DEPS_PKGCONFIG 
        base-types 
        base-logging
        typelib
    DEPS_PLAIN 
        Boost_SYSTEM Boost_FILESYSTEM)

rock_executable(indexer
    SOURCES indexer.cpp
    DEPS pocolog_cpp
    DEPS_PKGCONFIG base-types typelib
)

rock_executable(multiIndexer
    SOURCES multiIndexTest.cpp
    DEPS pocolog_cpp
    DEPS_PKGCONFIG base-types typelib
)
rock_executable(speedTest
    SOURCES speedTest.cpp
)

rock_executable(example_old
    SOURCES example_old.cpp
    DEPS pocolog_cpp
    DEPS_PKGCONFIG base-types typelib
)

#include "MappedStream.hpp"
#include <base-logging/Logging.hpp>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <cassert>
#include <iostream>
#include <stdexcept>
#include <unistd.h>

#include <sys/mman.h>
#include <cstring>

pocolog_cpp::MappedStream::MappedStream() : fd(-1), goodFlag(false), fileName("")
{

}

pocolog_cpp::MappedStream::MappedStream(const char* __s, std::ios_base::openmode mode)
{
    if(!open(__s, mode))
    {
        throw std::runtime_error(std::string("Error opening file") + __s);
    }
}

pocolog_cpp::MappedStream::~MappedStream()
{
    if(fd != -1)
        close();
}


bool pocolog_cpp::MappedStream::open(const char* fileName, std::ios_base::openmode mode)
{
    fd = ::open(fileName, O_NONBLOCK);
    if(fd < 0)
    {
        goodFlag = false;
        return false;
    }
    
    struct stat stats;
    int ret = ::fstat(fd, &stats);
    if(ret < 0)
    {
        goodFlag = false;
        return false;
    }
    
    fileSize = stats.st_size;
    blockSize = stats.st_blksize;
    
    goodFlag = true;
    mem_map = static_cast<const char*>(mmap(0, fileSize, PROT_READ, MAP_PRIVATE, fd, 0));
    readPos = 0;
    writePos = 0;
    this->fileName = fileName;
    
    LOG_DEBUG_S << "File opened " << (fd > 0) << " file size " << fileSize  << " blk size " << blockSize;
    
    return true;
}


void pocolog_cpp::MappedStream::read(char* buffer, size_t size)
{
    LOG_DEBUG_S << "Reading Bytes from pos " << readPos;

    std::memcpy(buffer, mem_map + readPos, size);
    readPos += size;
}

bool pocolog_cpp::MappedStream::good() const
{
    return goodFlag;
}

bool pocolog_cpp::MappedStream::fail() const
{
    return !goodFlag;
}

std::streampos pocolog_cpp::MappedStream::seekg(std::streampos pos)
{
    readPos = pos;
    goodFlag = true;
    return readPos;
}

std::streampos pocolog_cpp::MappedStream::seekp(std::streampos pos)
{
    writePos = pos;
    goodFlag = true;
    return writePos;
}

std::streampos pocolog_cpp::MappedStream::tellg()
{
    return readPos;
}

std::streampos pocolog_cpp::MappedStream::tellp()
{
    return writePos;
}

off_t pocolog_cpp::MappedStream::size() const
{
    return fileSize;
}

void pocolog_cpp::MappedStream::close()
{
    goodFlag = false;
    if(fd > 0)
        ::close(fd);
    fd = -1;
}


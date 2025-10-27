#include "FileStream.hpp"
#include <base-logging/Logging.hpp>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <cassert>
#include <iostream>
#include <stdexcept>
#include <unistd.h>
#include <limits>

pocolog_cpp::FileStream::FileStream() : fileName("")
{

}

pocolog_cpp::FileStream::FileStream(const char* __s, std::ios_base::openmode mode)
{
    if(!open(__s, mode))
    {
        throw std::runtime_error(std::string("Error opening file") + __s);
    }
}

pocolog_cpp::FileStream::~FileStream()
{
    // if(fd != -1)
    bufferedFileReader.close();
}


bool pocolog_cpp::FileStream::open(const char* fileName, std::ios_base::openmode mode)
{

    bufferReadPos = 0;
    bufferStartPos = 0;
    
    bufferedFileReader = std::ifstream(fileName, std::ifstream::in | mode);

    //fd = ::open(fileName, O_NONBLOCK);
    if(!bufferedFileReader.is_open())
    {
        return false;
    }
    
    // struct stat stats;
    // int ret = ::fstat(fd, &stats);
    // if(ret < 0)
    // {
    //     goodFlag = false;
    //     return false;
    // }
    
    // fileSize = stats.st_size;
    blockSize = 10;
    
    // goodFlag = true;
    // readBufferPosition = -1;
    // readBufferEndPosition = -1;
    // readBuffer.resize(blockSize);
    // readPos = 0;
    // writePos = 0;
    this->fileName = fileName;

    readerThreadRunning = true;
    fileIsEof = false;
    
    readThread = std::make_shared<std::thread>([&](){
        while (readerThreadRunning) {
            // printf("%s:%i %li\n", __PRETTY_FUNCTION__, __LINE__,blockSize.load());

            this->seekMutex.lock();
            

            this->readBufferMutex.lock();
            size_t availabeBytes = readBuffer.size() - bufferReadPos;
            this->readBufferMutex.unlock();


            // printf("%s:%i %li\n", __PRETTY_FUNCTION__, __LINE__, availabeBytes);
            
            if (availabeBytes < blockSize) {                
                std::vector<char> buf;
                buf.reserve(blockSize);

                for (int i = 0; i < blockSize;++i) {
                    char val;
                    if (!bufferedFileReader.eof()){
                        bufferedFileReader.get(val);
                        buf.push_back(val);
                    }else{
                        printf("%s:%i EOF\n", __PRETTY_FUNCTION__, __LINE__);
                        fileIsEof = true;
                    }
                }

                this->readBufferMutex.lock();
                    readBuffer.insert(readBuffer.end(),buf.begin(),buf.end());
                    printf("%s:%i %li\n", __PRETTY_FUNCTION__, __LINE__, readBuffer.size() - bufferReadPos);
                this->readBufferMutex.unlock();
                
                
                

                // //int read = bufferedFileReader.readsome(buf.data(), blockSize);
                // size_t readsize = blockSize;
                // // if (bufferedFileReader.rdbuf()->in_avail() < blockSize) {
                // //     readsize = bufferedFileReader.rdbuf()->in_avail();
                // // }

                // bufferedFileReader.get(buf.data(), readsize);
                // // int read = ;
                
                // printf("%s:%i %li\n", __PRETTY_FUNCTION__, __LINE__,readsize);
                // this->readBufferMutex.lock();
                // readBuffer.insert(readBuffer.end(),buf.begin(),buf.begin()+readsize);
                // // printf("%s:%i %li\n", __PRETTY_FUNCTION__, __LINE__, readBuffer.size());
                // this->readBufferMutex.unlock();
                // return;
            }else{
                // this->readBufferMutex.unlock();
                // printf("%s:%i %li\n", __PRETTY_FUNCTION__, __LINE__, availabeBytes);
                usleep(100);
            }
            this->seekMutex.unlock();
            // printf("%s:%i\n", __PRETTY_FUNCTION__, __LINE__);
        }
    });
    
    LOG_DEBUG_S << "File opened " << " file size "  << " blk size " << blockSize;
    
    return true;
}


void pocolog_cpp::FileStream::read(char* buffer, size_t size)
{
    LOG_DEBUG_S << "Reading Bytes " << size;

    

    // printf("%s:%i %li\n", __PRETTY_FUNCTION__, __LINE__, size);
    
    // readBufferMutex.lock();

    if (blockSize < size) {
        // increase read size, to fill the readbuffer to an appropirate level
        blockSize = size;
    }

    readBufferMutex.lock();
    size_t availabeBytes = readBuffer.size() - bufferReadPos;

    printf("%s:%i %li of %li\n", __PRETTY_FUNCTION__, __LINE__, availabeBytes, size);

    // need to free lock to allow reader thread to add contents
    // readBufferMutex.unlock();
    // while(availabeBytes < size) {
    //     usleep(100);
    //     //keep buffer unlocked while waiting for data
    //     readBufferMutex.lock();
    //     availabeBytes = readBuffer.size() - bufferReadPos;
    //     readBufferMutex.unlock();

    //     if (fileeof) {
    //         seekMutex.unlock();
    //         return;
    //     }
    //     printf("%s:%i %li of %li\n", __PRETTY_FUNCTION__, __LINE__, availabeBytes, size);
    // }


    while (readBuffer.size() < size) {
        readBufferMutex.unlock();
        usleep(1000);

        readBufferMutex.lock();

        if (bufferedFileReader.eof()){
            // not more to ecpect
            return;
        }

        //printf("%s:%i %li\n", __PRETTY_FUNCTION__, __LINE__,readBuffer.size());
    }

    printf("%s:%i %li\n", __PRETTY_FUNCTION__, __LINE__,readBuffer.size());
    readBufferMutex.unlock();


    readBufferMutex.lock();
    for(size_t i = 0; i < size; i++)
    {
            // buffer[i] = readBuffer[bufferReadPos++];
            buffer[i] = readBuffer.front();
            readBuffer.pop_front();
        
    }
    readBufferMutex.unlock();
    
    printf("%s:%i\n", __PRETTY_FUNCTION__, __LINE__);
    
}

bool pocolog_cpp::FileStream::good() const
{
    return bufferedFileReader.good();
}

bool pocolog_cpp::FileStream::fail() const
{
    return bufferedFileReader.fail();
}

std::streampos pocolog_cpp::FileStream::seekg(std::streampos pos)
{
    seekMutex.lock();
        bufferedFileReader.seekg(pos);
        readBufferMutex.lock();
            std::streampos bufferEnd = bufferStartPos + readBuffer.size();
            // if (pos < bufferStartPos || pos > bufferEnd) {
                // relocate read
                bufferReadPos = 0;
                bufferStartPos = pos;
                fileIsEof=false;
                readBuffer.clear(); // reload data in othe rthread
                printf("%s:%i\n", __PRETTY_FUNCTION__, __LINE__);
            // } else {
            //     // pos is already in buffer
            //     // relocate reead index
            //     bufferReadPos = pos - bufferStartPos;
            //     // size_t todelete = pos - bufferpos;
            //     // readBuffer.erase(readBuffer.begin(), readBuffer.begin()+todelete);
            //     printf("%s:%i\n", __PRETTY_FUNCTION__, __LINE__);
            // }
        readBufferMutex.unlock();
    seekMutex.unlock();

    //todo: error handling
    return pos;
}

// std::streampos pocolog_cpp::FileStream::seekp(std::streampos pos)
// {
//     return bufferedFileReader.seekp(pos);
// }

// std::streampos pocolog_cpp::FileStream::tellg()
// {
//     printf("%s:%i\n", __PRETTY_FUNCTION__, __LINE__);
//     readBufferMutex.lock();
//     // std::streampos pos = bufferedFileReader.tellg() - readBuffer.size();
//     std::streampos pos = bufferStartPos + bufferReadPos;
//     readBufferMutex.unlock();
//     return pos;
// }

// std::streampos pocolog_cpp::FileStream::tellp()
// {
//     return bufferedFileReader.tellp();
// }

// off_t pocolog_cpp::FileStream::size() const
// {
//     return 0;
// }

void pocolog_cpp::FileStream::close()
{
    readBufferMutex.lock();
    readerThreadRunning = false;
    readThread->join();
    readBufferMutex.unlock();
    bufferedFileReader.close();
}


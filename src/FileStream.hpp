#ifndef FILESTREAM_H
#define FILESTREAM_H

#include <fstream>
#include <vector>
#include <iostream>

#include <thread>
#include <iostream>
#include <mutex>
#include <deque>
#include <memory>
#include <atomic>

namespace pocolog_cpp
{
    
/**
 * Reimplementation of the std::fstream class.
 * 
 * This implementation is buffered, meanding that the class
 * will keep a read and a writebuffer, and only perform disc
 * accesses if neccessary. 
 *
 * This reimplementation performs way better in the usecase,
 * that seekp/seekg is called often. The std::fstream implementation
 * discards its buffer every time seek is called, resulting in a
 * horrible runtime performance in our usecase.
 * */
class FileStream
{
private:
    FileStream(const FileStream&){};
    std::deque<char> readBuffer;
    std::mutex readBufferMutex;
    std::mutex seekMutex;
    std::mutex readmutex;

    off_t bufferReadPos;
    off_t bufferStartPos;
    size_t eofPos;

    
    // off_t readBufferPosition;
    // off_t readBufferEndPosition;
    // off_t readPos;
    // off_t writePos;
    // off_t fileSize;
    std::atomic<size_t> blockSize;
    std::ifstream bufferedFileReader; 
    
    // bool posInBuffer(off_t pos) const
    // {
    //     return pos >= readBufferPosition && pos < readBufferEndPosition;
    // }
    
    // bool reloadBuffer(off_t position);
    // bool goodFlag;
    std::string fileName;

    std::shared_ptr<std::thread> readThread;
    std::atomic<bool> readerThreadRunning, fileIsEof;

public:
    FileStream();
    
    FileStream(const char* __s, std::ios_base::openmode mode);
    virtual ~FileStream();
    
    bool open(const char* fileName, std::ios_base::openmode mode);

    void setBlocksize(const size_t& size) {
        blockSize = size;
    }
    
    void read(char* buffer, size_t size);
    
    // std::streampos tellg();
    // std::streampos tellp();
    
    std::streampos seekg(std::streampos pos);
    // std::streampos seekp(std::streampos pos);
    
    bool good() const;
    bool eof() const
    {
        return bufferedFileReader.eof();
    }
    
    const std::string getFileName() const
    {
        return fileName;
    }

    bool fail() const;
    // off_t size() const;
    void close();
    
};
}
#endif // FILESTREAM_H

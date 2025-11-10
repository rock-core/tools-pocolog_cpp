#ifndef MAPPEDSTREAM_H
#define MAPPEDSTREAM_H

#include <fstream>
#include <vector>

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
class MappedStream
{
private:
//    std::vector<char> readBuffer;
    char const* mem_map; // mmap'ed file contents
    int fd;
    off_t readPos;
    off_t writePos;
    off_t fileSize;
    off_t blockSize;
    
    bool reloadBuffer(off_t position);
    bool goodFlag;
    std::string fileName;
    
public:
    MappedStream();
    
    MappedStream(const char* __s,
           std::ios_base::openmode mode);
    ~MappedStream();
    
    bool open(const char* fileName, std::ios_base::openmode mode);
    
    void read(char* buffer, size_t size);
    
    std::streampos tellg();
    std::streampos tellp();
    
    std::streampos seekg(std::streampos pos);
    std::streampos seekp(std::streampos pos);
    
    bool good() const;
    bool eof() const
    {
        return readPos >= fileSize;
    }
    
    const std::string getFileName() const
    {
        return fileName;
    }

    bool fail() const;
    off_t size() const;
    void close();
    
};
}
#endif // MAPPEDSTREAM_H

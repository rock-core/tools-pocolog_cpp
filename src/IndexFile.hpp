#ifndef INDEXFILE_H
#define INDEXFILE_H
#include <string>
#include <vector>
#include "LogFile.hpp"

namespace pocolog_cpp
{
class Index;
class LogFile;
    
class IndexFile
{
    std::vector<Index *> indices;
    std::vector<StreamDescription> streams;

public:
    
    struct IndexFileHeader
    {
        IndexFileHeader();
        char magic[8];
        uint32_t numStreams;
        static std::string getMagic();
    } __attribute__((packed));
    
    IndexFile(std::string indexFileName, pocolog_cpp::LogFile& logFile, bool verbose = true);
    
    IndexFile(LogFile& logFile, bool verbose = true);
    ~IndexFile();
    
    bool loadIndexFile(std::string indexFileName, LogFile& logFile);
    bool createIndexFile(std::string indexFileName, LogFile& logFile);
    
    Index &getIndexForStream(const StreamDescription &desc);
    
    const std::vector< StreamDescription >& getStreamDescriptions() const;
};
}
#endif // INDEXFILE_H

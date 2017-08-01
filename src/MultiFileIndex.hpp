#ifndef MULTIFILEINDEX_H
#define MULTIFILEINDEX_H
#include <vector>
#include <string>
#include <stdint.h>
#include <stdexcept>
#include <typelib/registry.hh>
#include <boost/function.hpp>

namespace pocolog_cpp
{
class LogFile;
class Stream;
class InputDataStream;

class MultiFileIndex
{
    struct IndexEntry
    {
        IndexEntry(): sampleNrInStream(0), stream(NULL) {};
        uint64_t sampleNrInStream;
        Stream *stream;
        size_t globalStreamIdx;
    } __attribute__((packed));
    
    bool verbose;
    std::vector<IndexEntry> index;
    std::vector<LogFile *> logFiles;
    std::vector<Stream *> streams;
    std::map<Stream *, size_t> streamToGlobalIdx;
    size_t globalSampleCount;
    Typelib::Registry combinedRegistry;
//     base::Time firstSampleTime;
//     base::Time lastSampleTime;
public:
    MultiFileIndex(const std::vector<std::string> &fileNames, bool verbose = true);
    MultiFileIndex(bool verbose = true);
    ~MultiFileIndex();
    
    const std::vector<Stream *> getAllStreams() const
    {
        return streams;
    };
    
    size_t getSize() const
    {
        return globalSampleCount;
    }

    size_t getGlobalStreamIdx(size_t globalSamplePos) const
    {
        if(globalSamplePos > globalSampleCount - 1)
            throw std::runtime_error("Error, Sample out of index requested");
        
        return index[globalSamplePos].globalStreamIdx;
    }

    size_t getGlobalStreamIdx(Stream *stream) const;

    Stream *getSampleStream(size_t globalSamplePos) const
    {
        if(globalSamplePos > globalSampleCount - 1)
            throw std::runtime_error("Error, Sample out of index requested");
        
        return index[globalSamplePos].stream;
    }
    
    size_t getPosInStream(size_t globalSamplePos) const
    {
        if(globalSamplePos > globalSampleCount - 1)
            throw std::runtime_error("Error, Sample out of index requested");
        
        return index[globalSamplePos].sampleNrInStream;
    }

    /***
     * Registers a callback, that evaluates every given stream if it
     * should be included in the MultiIndex
     * */
    void registerStreamCheck(boost::function<bool (Stream *stream)> test);
    
    Typelib::Registry &getCombinedRegistry()
    {
        return combinedRegistry;
    };

    bool createIndex(const std::vector<LogFile *> &logfiles);
    bool createIndex(const std::vector<std::string> &fileNames);
    
private:
    boost::function<bool (Stream *stream)> streamCheck;
};
}
#endif // MULTIFILEINDEX_H
